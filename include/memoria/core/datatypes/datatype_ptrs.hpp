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
#include <memoria/core/memory/object_pool.hpp>
#include <memoria/core/tools/span.hpp>

#include <memoria/core/datatypes/traits.hpp>

namespace memoria {

class DTViewHolder {
    using HolderT = pool::detail::ObjectPoolRefHolder;
    HolderT* owner_;
    int64_t references_{};
public:
    DTViewHolder():
        owner_(), references_()
    {}

    void set_owner(HolderT* owner) {
        owner_ = owner;
    }

    void ref_copy() {
        if (MMA_UNLIKELY((bool)(references_++ == 0))) {
            owner_->ref_copy();
        }
    }

    void unref() {
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


// SFINAE test
template <typename T>
class OwningViewCfg
{
    typedef char one;
    struct two { char x[2]; };

    template <typename C> static one test( decltype(&C::configure_resource_owner) ) ;
    template <typename C> static two test(...);

public:
    static constexpr bool Value = sizeof(test<T>(0)) == sizeof(char);

    static void configure_resource_owner(T& view, DTViewHolder* owner)
    {
        view.configure_resource_owner(owner);
    }
};

template <typename T, bool HasOwner = OwningViewCfg<T>::Value>
struct OwningViewSpanHelper;

template <typename T>
struct OwningViewSpanHelper<T, true> {
    static void configure_resource_owner(T& view, DTViewHolder* owner)
    {
        OwningViewCfg<T>::configure_resource_owner(view, owner);
    }

    static void configure_resource_owner(Span<T> span, DTViewHolder* owner)
    {
        for (auto& view: span) {
            configure_resource_owner(view, owner);
        }
    }
};

template <typename T>
struct OwningViewSpanHelper<T, false> {
    static void configure_resource_owner(T& view, DTViewHolder* owner) {}
    static void configure_resource_owner(Span<T> span, DTViewHolder* owner) {}
    static void configure_resource_owner(Span<const T> span, DTViewHolder* owner) {}
};



template <typename ViewT, typename PtrT>
class DTViewSpan {
    using HolderT = DTViewHolder;

    HolderT* owner_;
    ViewT* views_;
    size_t size_;

public:
    DTViewSpan():
        owner_(), views_(), size_()
    {}

    DTViewSpan(Span<ViewT> span, HolderT* holder) noexcept :
        owner_(holder),
        views_(span.data()),
        size_(span.size())
    {
        if (MMA_LIKELY((bool)owner_)) {
            owner_->ref_copy();
        }
    }

    DTViewSpan(ViewT* views, size_t size, HolderT* holder) noexcept :
        owner_(holder),
        views_(views),
        size_(size)
    {
        if (MMA_LIKELY((bool)owner_)) {
            owner_->ref_copy();
        }
    }


    DTViewSpan(const DTViewSpan& other) noexcept:
        owner_(other.owner_),
        views_(other.views_),
        size_(other.size_)
    {
        if (MMA_LIKELY((bool)owner_)) {
            owner_->ref_copy();
        }
    }

    DTViewSpan(DTViewSpan&& other) noexcept:
        owner_(other.owner_),
        views_(other.views_),
        size_(other.size_)
    {
        other.owner_ = nullptr;
    }

    virtual ~DTViewSpan() noexcept {
        if (owner_) {
            owner_->unref();
        }
    }

    size_t size() const {return size_;}

    ViewT* operator[](size_t idx) const {
        return &views_[idx];
    }

    PtrT get(size_t idx) {
        return PtrT(views_[idx], owner_);
    }

    Span<ViewT> raw_span() const {
        return Span<ViewT>(views_, size_);
    }

    DTViewSpan& operator=(const DTViewSpan& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (owner_) {
                owner_->unref();
            }

            owner_ = other.owner_;
            views_ = other.views_;
            size_  = other.size_;

            if (owner_) {
                owner_->ref_copy();
            }
        }

        return *this;
    }

    DTViewSpan& operator=(DTViewSpan&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(owner_ != nullptr)) {
                owner_->unref();
            }

            owner_ = other.owner_;
            views_ = other.views_;
            size_  = other.size_;

            other.owner_ = nullptr;
        }

        return *this;
    }

    void reset() noexcept {
        if (owner_) {
            owner_->unref();
            owner_ = nullptr;
        }
    }

    HolderT* release_holder()
    {
        HolderT* tmp = owner_;
        owner_ = nullptr;
        views_ = nullptr;
        return tmp;
    }

    friend void swap(DTViewSpan& lhs, DTViewSpan& rhs) {
        std::swap(lhs.views_, rhs.views_);
        std::swap(lhs.owner_, rhs.owner_);
    }

    DTViewSpan first(size_t size) const {
        return DTViewSpan(views_, size, owner_);
    }
};


template <typename ViewT, typename PtrT>
class DTConstViewSpan {
    using HolderT = DTViewHolder;

    HolderT* owner_;
    const ViewT* views_;
    size_t size_;

public:
    DTConstViewSpan():
        owner_(), views_(), size_()
    {}

    DTConstViewSpan(Span<const ViewT> span, HolderT* holder) noexcept :
        owner_(holder),
        views_(span.data()),
        size_(span.size())
    {
        if (MMA_LIKELY((bool)owner_)) {
            owner_->ref_copy();
        }
    }

    DTConstViewSpan(const ViewT* views, size_t size, HolderT* holder) noexcept :
        owner_(holder),
        views_(views),
        size_(size)
    {
        if (MMA_LIKELY((bool)owner_)) {
            owner_->ref_copy();
        }
    }


    DTConstViewSpan(const DTConstViewSpan& other) noexcept:
        owner_(other.owner_),
        views_(other.views_),
        size_(other.size_)
    {
        if (MMA_LIKELY((bool)owner_)) {
            owner_->ref_copy();
        }
    }

    DTConstViewSpan(const DTViewSpan<ViewT, PtrT>& other) noexcept:
        owner_(other.owner_),
        views_(other.views_),
        size_(other.size_)
    {
        if (MMA_LIKELY((bool)owner_)) {
            owner_->ref_copy();
        }
    }

    DTConstViewSpan(DTConstViewSpan&& other) noexcept:
        owner_(other.owner_),
        views_(other.views_),
        size_(other.size_)
    {
        other.owner_ = nullptr;
    }

    DTConstViewSpan(DTViewSpan<ViewT, PtrT>&& other) noexcept:
        owner_(other.owner_),
        views_(other.views_),
        size_(other.size_)
    {
        other.owner_ = nullptr;
    }

    virtual ~DTConstViewSpan() noexcept {
        if (owner_) {
            owner_->unref();
        }
    }

    size_t size() const {return size_;}

    const ViewT* operator[](size_t idx) const {
        return &views_[idx];
    }

    PtrT get(size_t idx) const {
        return PtrT(views_[idx], owner_);
    }

    Span<const ViewT> raw_span() const {
        return Span<const ViewT>(views_, size_);
    }

    DTConstViewSpan& operator=(const DTConstViewSpan& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (owner_) {
                owner_->unref();
            }

            owner_ = other.owner_;
            views_ = other.views_;
            size_  = other.size_;

            if (owner_) {
                owner_->ref_copy();
            }
        }

        return *this;
    }

    DTConstViewSpan& operator=(const DTViewSpan<ViewT, PtrT>& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (owner_) {
                owner_->unref();
            }

            owner_ = other.owner_;
            views_ = other.views_;
            size_  = other.size_;

            if (owner_) {
                owner_->ref_copy();
            }
        }

        return *this;
    }

    DTConstViewSpan& operator=(DTConstViewSpan&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(owner_ != nullptr)) {
                owner_->unref();
            }

            owner_ = other.owner_;
            views_ = other.views_;
            size_  = other.size_;

            other.owner_ = nullptr;
        }

        return *this;
    }

    DTConstViewSpan& operator=(DTViewSpan<ViewT, PtrT>&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(owner_ != nullptr)) {
                owner_->unref();
            }

            owner_ = other.owner_;
            views_ = other.views_;
            size_  = other.size_;

            other.owner_ = nullptr;
        }

        return *this;
    }

    void reset() noexcept {
        if (owner_) {
            owner_->unref();
            owner_ = nullptr;
        }
    }

    HolderT* release_holder()
    {
        HolderT* tmp = owner_;
        owner_ = nullptr;
        views_ = nullptr;
        return tmp;
    }

    friend void swap(DTConstViewSpan& lhs, DTConstViewSpan& rhs) {
        std::swap(lhs.views_, rhs.views_);
        std::swap(lhs.owner_, rhs.owner_);
    }

    DTConstViewSpan first(size_t size) const {
        return DTConstViewSpan(views_, size, owner_);
    }
};




template <typename ViewT>
class DTSharedPtr {
    using RefHolder = DTViewHolder;

    ViewT view_;
    RefHolder* ref_holder_;

public:
    using element_type = ViewT;

    DTSharedPtr() noexcept : view_(), ref_holder_() {}

    DTSharedPtr(ViewT view, RefHolder* holder) noexcept :
        view_(view), ref_holder_(holder)
    {
        if (holder) {
            holder->ref_copy();
        }
    }

    template<typename U>
    DTSharedPtr(const DTSharedPtr<U>& other) noexcept:
        view_(other.view_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }


    DTSharedPtr(const DTSharedPtr& other) noexcept:
        view_(other.view_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }


    DTSharedPtr(DTSharedPtr&& other) noexcept:
        view_(other.view_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    template<typename U>
    DTSharedPtr(DTSharedPtr<U>&& other) noexcept:
        view_(other.view_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    ~DTSharedPtr() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
        }
    }

    DTSharedPtr& operator=(const DTSharedPtr& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (ref_holder_) {
                ref_holder_->unref();
            }

            view_ = other.view_;
            ref_holder_ = other.ref_holder_;

            if (ref_holder_){
                ref_holder_->ref_copy();
            }
        }

        return *this;
    }

    DTSharedPtr& operator=(DTSharedPtr&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(ref_holder_ != nullptr)) {
                ref_holder_->unref();
            }

            view_ = other.view_;
            ref_holder_ = other.ref_holder_;
            other.ref_holder_ = nullptr;
        }

        return *this;
    }

    void reset() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
            ref_holder_ = nullptr;
        }
    }

    RefHolder* release_holder()
    {
        RefHolder* tmp = ref_holder_;
        ref_holder_ = nullptr;
        view_ = ViewT{};
        return tmp;
    }

    friend void swap(DTSharedPtr& lhs, DTSharedPtr& rhs) {
        std::swap(lhs.view_, rhs.view_);
        std::swap(lhs.ref_holder_, rhs.ref_holder_);
    }

    auto use_count() const {
        if (ref_holder_) {
            return ref_holder_->refs();
        }
        else {
            return 0;
        }
    }

    ViewT* operator->() {return &view_;}
    const ViewT* operator->() const {return &view_;}

    ViewT& operator*() {return view_;}
    const ViewT& operator*() const {return view_;}

    ViewT* get() {return &view_;}
    const ViewT* get() const {return &view_;}

    operator bool() const {
        return ref_holder_ != nullptr;
    }
};



template <typename ViewT>
class DTConstSharedPtr {
    using RefHolder = DTViewHolder;

    ViewT view_;
    RefHolder* ref_holder_;

public:
    using element_type = ViewT;

    DTConstSharedPtr() noexcept : view_(), ref_holder_() {}

    DTConstSharedPtr(ViewT view, RefHolder* holder) noexcept :
        view_(view), ref_holder_(holder)
    {
        if (holder) {
            holder->ref_copy();
        }
    }

    template<typename U>
    DTConstSharedPtr(const DTConstSharedPtr<U>& other) noexcept:
        view_(other.view_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }

    template<typename U>
    DTConstSharedPtr(const DTSharedPtr<U>& other) noexcept:
        view_(other.view_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }


    DTConstSharedPtr(const DTConstSharedPtr& other) noexcept:
        view_(other.view_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }

    DTConstSharedPtr(const DTSharedPtr<ViewT>& other) noexcept:
        view_(other.view_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }


    DTConstSharedPtr(DTConstSharedPtr&& other) noexcept:
        view_(other.view_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }


    DTConstSharedPtr(DTSharedPtr<ViewT>&& other) noexcept:
        view_(other.view_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    template<typename U>
    DTConstSharedPtr(DTConstSharedPtr<U>&& other) noexcept:
        view_(other.view_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    template<typename U>
    DTConstSharedPtr(DTSharedPtr<U>&& other) noexcept:
        view_(other.view_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    ~DTConstSharedPtr() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
        }
    }

    DTConstSharedPtr& operator=(const DTConstSharedPtr& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (ref_holder_) {
                ref_holder_->unref();
            }

            view_ = other.view_;
            ref_holder_ = other.ref_holder_;

            if (ref_holder_){
                ref_holder_->ref_copy();
            }
        }

        return *this;
    }

    DTConstSharedPtr& operator=(DTConstSharedPtr&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(ref_holder_ != nullptr)) {
                ref_holder_->unref();
            }

            view_ = other.view_;
            ref_holder_ = other.ref_holder_;
            other.ref_holder_ = nullptr;
        }

        return *this;
    }

    void reset() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
            ref_holder_ = nullptr;
        }
    }

    RefHolder* release_holder()
    {
        RefHolder* tmp = ref_holder_;
        ref_holder_ = nullptr;
        view_ = ViewT{};
        return tmp;
    }

    friend void swap(DTConstSharedPtr& lhs, DTConstSharedPtr& rhs) {
        std::swap(lhs.view_, rhs.view_);
        std::swap(lhs.ref_holder_, rhs.ref_holder_);
    }

    auto use_count() const {
        if (ref_holder_) {
            return ref_holder_->refs();
        }
        else {
            return 0;
        }
    }

    const ViewT* operator->() const {return &view_;}

    const ViewT& operator*() const {return view_;}

    const ViewT* get() const {return &view_;}

    operator bool() const {
        return ref_holder_ != nullptr;
    }
};




template <typename ViewT>
class DTFxdValueWrapper {
    using RefHolder = DTViewHolder;

    ViewT view_;

    template <typename> friend class DTFxdValueWrapper;

public:
    using element_type = ViewT;

    DTFxdValueWrapper() noexcept : view_() {}

    DTFxdValueWrapper(ViewT view) noexcept :
        view_(view)
    {}

    DTFxdValueWrapper(ViewT view, RefHolder*) noexcept :
        view_(view)
    {}

    template<typename U>
    DTFxdValueWrapper(const DTFxdValueWrapper<U>& other) noexcept:
        view_(other.view_)
    {}

    DTFxdValueWrapper(const DTFxdValueWrapper& other) noexcept:
        view_(other.view_)
    {}


    DTFxdValueWrapper(DTFxdValueWrapper&& other) noexcept:
        view_(std::move(other.view_))
    {}

    template<typename U>
    DTFxdValueWrapper(DTFxdValueWrapper<U>&& other) noexcept:
        view_(std::move(other.view_))
    {}

    ~DTFxdValueWrapper() noexcept = default;

    DTFxdValueWrapper& operator=(const DTFxdValueWrapper& other) noexcept {
        view_ = other.view_;
        return *this;
    }

    DTFxdValueWrapper& operator=(DTFxdValueWrapper&& other) noexcept {
        view_ = std::move(other.view_);
        return *this;
    }

    void reset() noexcept {}

    friend void swap(DTFxdValueWrapper& lhs, DTFxdValueWrapper& rhs) {
        std::swap(lhs.view_, rhs.view_);
    }

    size_t use_count() const {
        return 1;
    }

    ViewT* operator->() {return &view_;}
    const ViewT* operator->() const {return &view_;}

    ViewT& operator*() {return view_;}
    const ViewT& operator*() const {return view_;}

    ViewT* get() {return &view_;}
    const ViewT* get() const {return &view_;}

    operator bool() const {
        return true;
    }
};


}
