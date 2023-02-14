
// Copyright 2022 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/core/types.hpp>

#include <memoria/core/tools/span.hpp>

#include <boost/variant2/variant.hpp>

#include <type_traits>
#include <iostream>

namespace memoria {

namespace pool {
template <typename> class SharedPtr;
template <typename> class UniquePtr;
template <typename> class WeakPtr;

template <typename> class enable_shared_from_this;
}

namespace hermes {
class HermesCtrView;
}

namespace arena {
class ArenaAllocator;
}

class MemoryObject;

#ifdef MMA_NO_REACTOR
class MemoryObject {
public:
    virtual ~MemoryObject() noexcept = default;
    virtual void finalize_memory_object() {};
    constexpr bool is_put_to_list() const { return false;}
    void put_to_list() {}
};

#else

class MemoryObjectList {
    MemoryObject* head_;
    size_t size_;
public:
    MemoryObjectList():
        head_(), size_()
    {}

    MemoryObject* head() const {return head_;}
    size_t size() const {return size_;}

    void link(MemoryObject* msg);

    MemoryObject* detach_all() {
        MemoryObject* tmp = head_;
        head_ = nullptr;
        size_ = 0;
        return tmp;
    }

    static MemoryObjectList& list(int cpu) {
        return object_lists()[cpu];
    }

private:
    static std::vector<MemoryObjectList>& object_lists();
};

class MemoryObject {
    MemoryObjectList* list_;
    MemoryObject* next_;

protected:
    int owner_cpu_;

    friend class MemoryObjectList;
public:
    MemoryObject():
        list_(), next_(), owner_cpu_()
    {}

    ~MemoryObject() noexcept = default;
    virtual void finalize_memory_object() {}

    bool is_put_to_list() const {
        return list_ != nullptr;
    }

    void put_to_list() {
        list_->link(this);
    }

    int owner_cpu() const {
        return owner_cpu_;
    }

    void set_list(MemoryObjectList* list) {
        list_ = list;
    }

    void run_finalizers()
    {
        MemoryObject* obj = this;
        while (obj) {
            MemoryObject* next = obj->next_;
            obj->finalize_memory_object();
            obj = next;
        }
    }
};


#endif


class SharedPtrHolder: public MemoryObject {
    long use_count_{1};
    long weak_count_{1};

    template <typename> friend class memoria::pool::SharedPtr;
    template <typename> friend class memoria::pool::UniquePtr;
    template <typename> friend class memoria::pool::WeakPtr;

    template <typename, bool>
    friend struct SharedFromThisHelper;

public:
    SharedPtrHolder() {}
    virtual ~SharedPtrHolder() noexcept = default;


protected:
    // Release the holder
    virtual void dispose() noexcept = 0;

    // Run the object's destructor
    virtual void destroy() noexcept = 0;

public:
    void ref_copy() noexcept {
        use_count_++;
    }

    bool ref_lock() noexcept {
        if (use_count_ == 0) return false;
        use_count_++;
        return true;
    }

    void ref_weak() noexcept {
        weak_count_++;
    }

    void unref() noexcept {
        if (MMA_UNLIKELY(--use_count_ == 0)) {
            destroy();
            unref_weak();
        }
    }

    void unref_weak() noexcept {
        if (MMA_UNLIKELY(--weak_count_ == 0)) {
            dispose();
        }
    }

public:
    long refs() const {
        return use_count_;
    }

    long weak_refs() const {
        return weak_count_;
    }

    void init_ref() noexcept {
        use_count_ = 1;
        weak_count_ = 1;
    }
};




class LWMemHolder {
    using HolderT = SharedPtrHolder;

public:
    using MemData = boost::variant2::variant<
        arena::ArenaAllocator*,
        Span<uint8_t>,
        EmptyType
    >;

private:
    MemData mem_data_;

    mutable HolderT* owner_;
    int64_t references_{};
public:
    LWMemHolder():
        mem_data_(EmptyType{}),
        owner_(),
        references_()
    {}

    LWMemHolder(HolderT* owner):
        mem_data_(EmptyType{}),
        owner_(owner),
        references_()
    {}

    LWMemHolder(HolderT* owner, MemData mem_data):
        mem_data_(std::move(mem_data)),
        owner_(owner),
        references_()
    {}

    MemData& mem_data() {return mem_data_;}
    const MemData& mem_data() const {return mem_data_;}

    bool is_mem_mutable() const {
        return mem_data_.index() == 0;
    }

    void set_owner(HolderT* owner) {
        owner_ = owner;
    }

    HolderT* owner() const {
        return owner_;
    }

    void ref_copy() {
        if (MMA_UNLIKELY((bool)(references_++ == 0))) {
            owner_->ref_copy();
        }
    }

    void unref() noexcept {
        if (MMA_UNLIKELY((bool)(--references_ == 0))) {
            owner_->unref();
        }
    }

    int64_t refs() const {
        return references_;
    }

    bool is_in_use() const {
        return references_ != 0;
    }
};

class MemHolderHandle {
    mutable LWMemHolder* holder_;
public:
    MemHolderHandle(): holder_(nullptr) {}

    MemHolderHandle(LWMemHolder* holder):
        holder_(holder)
    {
        holder->ref_copy();
    }

    MemHolderHandle(MemHolderHandle&& other) noexcept :
        holder_(other.holder_)
    {
        other.holder_ = nullptr;
    }

    MemHolderHandle(const MemHolderHandle& other) noexcept :
        holder_(other.holder_)
    {
        if (holder_) {
            holder_->ref_copy();
        }
    }

    ~MemHolderHandle() noexcept
    {
        if (holder_) {
            holder_->unref();
        }
    }

    MemHolderHandle& operator=(const MemHolderHandle& other)
    {
        if (holder_) {
            holder_->unref();
        }

        holder_ = other.holder_;

        if (holder_) {
            holder_->ref_copy();
        }

        return *this;
    }

    MemHolderHandle& operator=(MemHolderHandle&& other)
    {
        if (holder_) {
            holder_->unref();
        }

        holder_ = other.holder_;
        other.holder_ = nullptr;

        return *this;
    }

    bool operator==(const MemHolderHandle& other) const noexcept {
        return holder_ == other.holder_;
    }

    LWMemHolder* holder() const {
        return holder_;
    }

    LWMemHolder* release() {
        auto tmp = holder_;
        holder_ = nullptr;
        return tmp;
    }
};

enum class OwningKind: size_t {
    EMBEDDED = 0,
    RESERVED = 1,
    WRAPPING = 2,
    HOLDING  = 3
};

template <typename, OwningKind> class Own;

namespace detail {

template <typename T>
class HasGetMemHolderMethod
{
    typedef char one;
    struct two { char x[2]; };

    template <typename C> static one test( decltype(&C::get_mem_holder) ) ;
    template <typename C> static two test(...);

public:
    static constexpr bool Value = sizeof(test<T>(0)) == sizeof(char);
};

}

template <typename T>
struct IsWrappingView: HasValue<bool, false> {};

struct MoveOwnershipTag{};

template <typename>
class HoldingView {
protected:
    mutable LWMemHolder* mem_holder_;

    template <typename, OwningKind>
    friend class Own;

    template <typename>
    friend class detail::HasGetMemHolderMethod;

public:
    HoldingView() noexcept:
        mem_holder_()
    {
    }

    HoldingView(LWMemHolder* holder) noexcept:
        mem_holder_(holder)
    {}

    HoldingView& operator=(const HoldingView& other) noexcept {
        mem_holder_ = other.mem_holder_;
        return *this;
    }

    bool is_mem_mutable() {
        return mem_holder_->mem_data().index() == 0;
    }

protected:
    LWMemHolder* get_mem_holder() const noexcept {
        return mem_holder_;
    }

    void reset_mem_holder() noexcept {
        mem_holder_ = nullptr;
    }

    LWMemHolder* release_mem_holder() noexcept {
        auto tmp = mem_holder_;
        mem_holder_ = nullptr;
        return tmp;
    }

    void set_mem_holder(LWMemHolder* mem_holder) noexcept {
        mem_holder_ = mem_holder;
    }
};



template <typename ViewT>
constexpr OwningKind OwningKindOf = detail::HasGetMemHolderMethod<ViewT>::Value ?
        OwningKind::HOLDING :
            IsWrappingView<ViewT>::Value ?
                OwningKind::WRAPPING :
        OwningKind::EMBEDDED;

template <typename ViewT, OwningKind IsHoldingView = OwningKindOf<ViewT>>
class Own;


template <typename ViewT>
class Own<ViewT, OwningKind::EMBEDDED> {
    using MemHolder = LWMemHolder;

    mutable ViewT view_;
    MemHolder* mem_holder_;

    template <typename, OwningKind>
    friend class Own;

public:
    using element_type = ViewT;

    Own() noexcept : view_(), mem_holder_() {}

    Own(ViewT view, MemHolder* holder) noexcept :
        view_(view), mem_holder_(holder)
    {
        if (holder) {
            holder->ref_copy();
        }
    }

    template<typename U>
    Own(const Own<U, OwningKind::EMBEDDED>& other) noexcept:
        view_(other.view_),
        mem_holder_(other.mem_holder_)
    {
        if (MMA_LIKELY((bool)mem_holder_)) {
            mem_holder_->ref_copy();
        }
    }


    Own(const Own& other) noexcept:
        view_(other.view_),
        mem_holder_(other.mem_holder_)
    {
        if (MMA_LIKELY((bool)mem_holder_)) {
            mem_holder_->ref_copy();
        }
    }


    Own(Own&& other) noexcept:
        view_(other.view_), mem_holder_(other.mem_holder_)
    {
        other.mem_holder_ = nullptr;
    }

    template<typename U>
    Own(Own<U, OwningKind::EMBEDDED>&& other) noexcept:
        view_(other.view_), mem_holder_(other.mem_holder_)
    {
        other.mem_holder_ = nullptr;
    }

    ~Own() noexcept {
        if (mem_holder_) {
            mem_holder_->unref();
        }
    }

    Own& operator=(const Own& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (mem_holder_) {
                mem_holder_->unref();
            }

            view_ = other.view_;
            mem_holder_ = other.mem_holder_;

            if (mem_holder_){
                mem_holder_->ref_copy();
            }
        }

        return *this;
    }

    Own& operator=(Own&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(mem_holder_ != nullptr)) {
                mem_holder_->unref();
            }

            view_ = other.view_;
            mem_holder_ = other.mem_holder_;
            other.mem_holder_ = nullptr;
        }

        return *this;
    }

    void reset() noexcept {
        if (mem_holder_) {
            mem_holder_->unref();
            mem_holder_ = nullptr;
        }
    }

    MemHolder* release_holder()
    {
        MemHolder* tmp = mem_holder_;
        mem_holder_ = nullptr;
        view_ = ViewT{};
        return tmp;
    }

    friend void swap(Own& lhs, Own& rhs) {
        std::swap(lhs.view_, rhs.view_);
        std::swap(lhs.mem_holder_, rhs.mem_holder_);
    }

    auto use_count() const {
        if (mem_holder_) {
            return mem_holder_->refs();
        }
        else {
            return 0;
        }
    }

    ViewT* operator->() {return &view_;}
    ViewT* operator->() const {return &view_;}

    ViewT& operator*() {return view_;}
    ViewT& operator*() const {return view_;}

    ViewT* get() {return &view_;}
    const ViewT* get() const {return &view_;}

    bool is_empty() const {
        return mem_holder_ == nullptr;
    }

    bool is_not_empty() const {
        return mem_holder_ != nullptr;
    }
};


template <typename ViewT>
class Own<ViewT, OwningKind::WRAPPING>: public ViewT {

    using MemHolder = LWMemHolder;
    using Base = ViewT;

    template <typename, OwningKind>
    friend class Own;

public:
    using element_type = ViewT;

    Own() noexcept : Base() {}

    template <typename... Args>
    Own(MemHolder* holder, Args&&... args):
        Base(std::forward<Args>(args)...)
    {}

    template <typename... Args>
    Own(MemHolderHandle&& holder, Args&&... args):
        Base(std::forward<Args>(args)...)
    {}

    template <typename... Args>
    Own(Args&&... args):
        Base(std::forward<Args>(args)...)
    {}

    template<typename U>
    Own(const Own<U, OwningKind::WRAPPING>& other):
        Base(*static_cast<const U*>(&other))
    {}


    Own(const Own& other):
        Base(*static_cast<const ViewT*>(&other))
    {}

    Own(Own&& other):
        Base(std::move(*static_cast<const ViewT*>(&other)))
    {}

    template<typename U>
    Own(Own<U, OwningKind::WRAPPING>&& other) noexcept:
        Base(std::move(other))
    {}

    template <typename U>
    Own& operator=(const U& other){
        Base::operator=(other);
        return *this;
    }

    Own& operator=(const Own& other){
        Base::operator=(*static_cast<const ViewT*>(&other));
        return *this;
    }

    Own& operator=(Own&& other){
        Base::operator=(std::move(*static_cast<const ViewT*>(&other)));
        return *this;
    }

    template <typename U>
    bool operator==(const U& other) {
        return Base::operator==(other);
    }

    bool operator==(const Own& other) {
        return Base::operator==(*static_cast<const ViewT*>(&other));
    }

    void reset() noexcept {
        *this = {};
    }

    MemHolder* release_holder() {
        return {};
    }

    friend void swap(Own& lhs, Own& rhs)
    {
        swap(
            *static_cast<ViewT*>(&lhs),
            *static_cast<ViewT*>(&rhs)
        );
    }

    auto use_count() const {
        return 1;
    }
};


template <typename ViewT>
class Own<ViewT, OwningKind::HOLDING>: public ViewT {
    using MemHolder = LWMemHolder;
    using Base = ViewT;

    template <typename, OwningKind>
    friend class Own;

public:
    using element_type = ViewT;

    Own() noexcept : Base() {}

    template <typename... Args>
    Own(MemHolder* holder, Args&&... args):
        Base(holder, std::forward<Args>(args)...)
    {
        if (holder) {
            holder->ref_copy();
        }
    }

    template <typename... Args>
    Own(MemHolderHandle&& holder, Args&&... args):
        Base(std::move(holder), std::forward<Args>(args)...)
    {}

    Own(ViewT view) noexcept :
        Base(view)
    {
        MemHolder* holder = this->get_mem_holder();
        if (holder) {
            holder->ref_copy();
        }
    }

    template<typename U>
    Own(const Own<U, OwningKind::HOLDING>& other) noexcept:
        Base(*static_cast<const U*>(&other))
    {
        MemHolder* holder = this->get_mem_holder();
        if (MMA_LIKELY((bool)holder)) {
            holder->ref_copy();
        }
    }


    Own(const Own& other) noexcept:
        Base(*static_cast<const ViewT*>(&other))
    {
        MemHolder* holder = this->get_mem_holder();
        if (MMA_LIKELY((bool)holder)) {
            holder->ref_copy();
        }
    }


    Own(Own&& other) noexcept:
        Base(std::move(*static_cast<ViewT*>(&other)))
    {
        other.reset_mem_holder();
    }

    template<typename U>
    Own(Own<U, OwningKind::HOLDING>&& other) noexcept:
        Base(*static_cast<U*>(&other))
    {
        other.reset_mem_holder();
    }

    ~Own() noexcept
    {
        MemHolder* holder = this->get_mem_holder();
        if (holder) {
            holder->unref();
        }
    }

    Own& operator=(const Own& other) noexcept
    {
        if (MMA_LIKELY(&other != this))
        {
            MemHolder* holder = this->get_mem_holder();

            if (holder) {
                holder->unref();
            }

            *static_cast<ViewT*>(this) = *static_cast<const ViewT*>(&other);

            holder = this->get_mem_holder();
            if (holder){
                holder->ref_copy();
            }
        }

        return *this;
    }

    Own& operator=(Own&& other) noexcept
    {
        if (MMA_LIKELY(&other != this))
        {
            MemHolder* holder = this->get_mem_holder();

            if (MMA_UNLIKELY(holder != nullptr)) {
                holder->unref();
            }

            *static_cast<ViewT*>(this) = std::move(*static_cast<const ViewT*>(&other));

            other.reset_mem_holder();
        }

        return *this;
    }

    void reset() noexcept
    {
        MemHolder* holder = this->get_mem_holder();
        if (holder) {
            holder->unref();
            this->reset_mem_holder();
        }
    }

    MemHolder* release_holder()
    {
        MemHolder* tmp = this->get_mem_holder();
        *static_cast<ViewT*>(this) = ViewT{};
        return tmp;
    }

    friend void swap(Own& lhs, Own& rhs) {
        std::swap(
            *static_cast<ViewT*>(&lhs),
            *static_cast<ViewT*>(&rhs)
        );
    }

    auto use_count() const {
        MemHolder* holder = this->get_mem_holder();
        if (holder) {
            return holder->refs();
        }
        else {
            return 0;
        }
    }

    bool is_empty() const {
        return this->get_mem_holder() == nullptr;
    }

    bool is_not_empty() const {
        return this->get_mem_holder() != nullptr;
    }
};

}
