
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

#include <type_traits>

namespace memoria {

namespace pool {
template <typename> class SharedPtr;
template <typename> class UniquePtr;
template <typename> class WeakPtr;

template <typename> class enable_shared_from_this;
}

class SharedPtrHolder {
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




class ViewPtrHolder {
    using HolderT = SharedPtrHolder;
    mutable HolderT* owner_;
    int64_t references_{};
public:
    ViewPtrHolder():
        owner_(), references_()
    {}

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

template <typename, bool> class ViewPtr;

class HoldingView {
protected:
    mutable ViewPtrHolder* ptr_holder_;

    template <typename, bool>
    friend class ViewPtr;

public:
    HoldingView() noexcept:
        ptr_holder_()
    {
    }

    HoldingView(ViewPtrHolder* holder) noexcept:
        ptr_holder_(holder)
    {}

protected:
    ViewPtrHolder* get_ptr_holder() const noexcept {
        return ptr_holder_;
    }

    void reset_ptr_holder() noexcept {
        ptr_holder_ = nullptr;
    }
};


template <typename ViewT, bool IsHoldingView = std::is_base_of_v<HoldingView, ViewT>>
class ViewPtr;

template <typename ViewT>
class ViewPtr<ViewT, false> {
    using RefHolder = ViewPtrHolder;

    mutable ViewT view_;
    RefHolder* ref_holder_;

public:
    using element_type = ViewT;

    ViewPtr() noexcept : view_(), ref_holder_() {}

    ViewPtr(ViewT view, RefHolder* holder) noexcept :
        view_(view), ref_holder_(holder)
    {
        if (holder) {
            holder->ref_copy();
        }
    }

    template<typename U>
    ViewPtr(const ViewPtr<U, false>& other) noexcept:
        view_(other.view_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }


    ViewPtr(const ViewPtr& other) noexcept:
        view_(other.view_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }


    ViewPtr(ViewPtr&& other) noexcept:
        view_(other.view_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    template<typename U>
    ViewPtr(ViewPtr<U, false>&& other) noexcept:
        view_(other.view_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    ~ViewPtr() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
        }
    }

    ViewPtr& operator=(const ViewPtr& other) noexcept {
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

    ViewPtr& operator=(ViewPtr&& other) noexcept {
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

    friend void swap(ViewPtr& lhs, ViewPtr& rhs) {
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
    ViewT* operator->() const {return &view_;}

    ViewT& operator*() {return view_;}
    ViewT& operator*() const {return view_;}

    ViewT* get() {return &view_;}
    const ViewT* get() const {return &view_;}

    bool is_empty() const {
        return ref_holder_ == nullptr;
    }

    bool is_not_empty() const {
        return ref_holder_ != nullptr;
    }
};


template <typename ViewT>
class ViewPtr<ViewT, true> {
    using RefHolder = ViewPtrHolder;

    mutable ViewT view_;

    template <typename, bool>
    friend class ViewPtr;

public:
    using element_type = ViewT;

    ViewPtr() noexcept : view_() {}

    ViewPtr(ViewT view) noexcept :
        view_(view)
    {
        RefHolder* holder = view_.get_ptr_holder();

        if (holder) {
            holder->ref_copy();
        }
    }

    template<typename U>
    ViewPtr(const ViewPtr<U, true>& other) noexcept:
        view_(other.view_)
    {
        RefHolder* holder = view_.get_ptr_holder();
        if (MMA_LIKELY((bool)holder)) {
            holder->ref_copy();
        }
    }


    ViewPtr(const ViewPtr& other) noexcept:
        view_(other.view_)
    {
        RefHolder* holder = view_.get_ptr_holder();
        if (MMA_LIKELY((bool)holder)) {
            holder->ref_copy();
        }
    }


    ViewPtr(ViewPtr&& other) noexcept:
        view_(other.view_)
    {
        other.view_.reset_ptr_holder();
    }

    template<typename U>
    ViewPtr(ViewPtr<U, true>&& other) noexcept:
        view_(other.view_)
    {
        other.view_.reset_ptr_holder();
    }

    ~ViewPtr() noexcept
    {
        RefHolder* holder = view_.get_ptr_holder();
        if (holder) {
            holder->unref();
        }
    }

    ViewPtr& operator=(const ViewPtr& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            RefHolder* holder = view_.get_ptr_holder();

            if (holder) {
                holder->unref();
            }

            view_ = other.view_;

            holder = view_.get_ptr_holder();
            if (holder){
                holder->ref_copy();
            }
        }

        return *this;
    }

    ViewPtr& operator=(ViewPtr&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            RefHolder* holder = view_.get_ptr_holder();

            if (MMA_UNLIKELY(holder != nullptr)) {
                holder->unref();
            }

            view_ = other.view_;

            other.view_.reset_ptr_holder();
        }

        return *this;
    }

    void reset() noexcept
    {
        RefHolder* holder = view_.get_ptr_holder();
        if (holder) {
            holder->unref();
            view_.reset_ptr_holder();
        }
    }

    RefHolder* release_holder()
    {
        RefHolder* tmp = view_.get_ptr_holder();
        view_ = ViewT{};
        return tmp;
    }

    friend void swap(ViewPtr& lhs, ViewPtr& rhs) {
        std::swap(lhs.view_, rhs.view_);
    }

    auto use_count() const {
        RefHolder* holder = view_.get_ptr_holder();
        if (holder) {
            return holder->refs();
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
        return view_.get_ptr_holder() == nullptr;
    }

    bool is_not_empty() const {
        return view_.get_ptr_holder() != nullptr;
    }
};

template <typename View>
bool operator<(const ViewPtr<View>& v0, const ViewPtr<View>& v1) {
    return v0 < v1;
}

template <typename View>
bool operator>(const ViewPtr<View>& v0, const ViewPtr<View>& v1) {
    return v0 > v1;
}

}
