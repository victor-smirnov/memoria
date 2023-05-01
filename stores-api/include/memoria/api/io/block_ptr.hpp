
// Copyright 2023 Victor Smirnov
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

#include <atomic>
#include <type_traits>

namespace memoria::io {

template <typename To, typename From, typename Selector>
struct BlockPtrConvertible: BoolValue<false> {};

template <typename From, typename To, typename Selector>
struct BlockPtrCastable: BoolValue<false> {};

class IBlockHolder {
    std::atomic<size_t> refcount_;
public:

    virtual ~IBlockHolder() = default;

    void ref_copy() {
        refcount_.fetch_add(1, std::memory_order_relaxed);
    }

    void unref() {
        if (refcount_.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            release();
        }
    }

    size_t uses() const {
        return refcount_.load(std::memory_order_seq_cst);
    }

    virtual void release() noexcept = 0;
    virtual void dispose() noexcept = 0;
};




template <typename T>
class BlockPtr {
    static_assert(std::is_standard_layout_v<T>);

    using RefHolder = IBlockHolder;

    T* ptr_;
    RefHolder* ref_holder_;

    // FIXME: Check how it works with const
    template <typename From>
    using IsConvertibleFrom = std::enable_if_t<
        std::is_convertible_v<From*, T*> ||
        BlockPtrConvertible<
            std::remove_const_t<T>,
            std::remove_const_t<From>
        >::Value
    >;

    // FIXME: Check how it works with const
    template <typename To>
    using IsCastableTo = std::enable_if_t<
        std::is_base_of_v<T, To> ||
        BlockPtrCastable<std::remove_const_t<T>, To>::Value
    >;

public:
    BlockPtr(T* ptr, RefHolder* holder) noexcept :
        ptr_(ptr), ref_holder_(holder)
    {}

    BlockPtr(T* ptr, RefHolder* holder, EmptyType) noexcept :
        ptr_(ptr), ref_holder_(holder)
    {
        holder->ref_copy();
    }

private:

    template <typename> friend class BlockPtr;

public:
    using element_type = T;

    BlockPtr() noexcept : ptr_(), ref_holder_() {}

    template<typename U, typename = IsConvertibleFrom<U>>
    BlockPtr(const BlockPtr<U>& other) noexcept:
        ptr_(other.ptr_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }

    BlockPtr(const BlockPtr& other) noexcept:
        ptr_(other.ptr_),
        ref_holder_(other.ref_holder_)
    {
        if (MMA_LIKELY((bool)ref_holder_)) {
            ref_holder_->ref_copy();
        }
    }


    BlockPtr(BlockPtr&& other) noexcept:
        ptr_(other.ptr_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    template<typename U, typename = IsConvertibleFrom<U>>
    BlockPtr(BlockPtr<U>&& other) noexcept:
        ptr_(other.ptr_), ref_holder_(other.ref_holder_)
    {
        other.ref_holder_ = nullptr;
    }

    ~BlockPtr() noexcept {
        if (ref_holder_) {
            ref_holder_->unref();
        }
    }

    BlockPtr& operator=(const BlockPtr& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (ref_holder_) {
                ref_holder_->unref();
            }

            ptr_ = other.ptr_;
            ref_holder_ = other.ref_holder_;

            if (ref_holder_){
                ref_holder_->ref_copy();
            }
        }

        return *this;
    }

    BlockPtr& operator=(BlockPtr&& other) noexcept {
        if (MMA_LIKELY(&other != this))
        {
            if (MMA_UNLIKELY(ref_holder_ != nullptr)) {
                ref_holder_->unref();
            }

            ptr_ = other.ptr_;
            ref_holder_ = other.ref_holder_;
            other.ref_holder_ = nullptr;
        }

        return *this;
    }

    template<typename U, typename = IsConvertibleFrom<U>>
    BlockPtr& operator=(BlockPtr<U>&& other) noexcept
    {
        if (MMA_UNLIKELY(ref_holder_ != nullptr)) {
            ref_holder_->unref();
        }

        ptr_ = reinterpret_cast<T*>(other.ptr_);
        ref_holder_ = other.ref_holder_;
        other.ref_holder_ = nullptr;

        return *this;
    }


    template<typename U, typename = IsConvertibleFrom<U>>
    bool operator==(const BlockPtr<U>& other) const noexcept {
        return ref_holder_ == other.ref_holder_;
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
        ptr_ = nullptr;
        return tmp;
    }

    friend void swap(BlockPtr& lhs, BlockPtr& rhs) {
        std::swap(lhs.ptr_, rhs.ptr_);
        std::swap(lhs.ref_holder_, rhs.ref_holder_);
    }

    auto use_count() const {
        if (ref_holder_) {
            return ref_holder_->uses();
        }
        else {
            return 0;
        }
    }

    T* operator->() {return ptr_;}
    T* operator->() const {return ptr_;}

    T& operator*() {return *ptr_;}
    T& operator*() const {return *ptr_;}

    T* get() {return ptr_;}
    T* get() const {return ptr_;}

    operator bool() const {
        return ref_holder_ != nullptr;
    }

    bool is_null() const noexcept {
        return ref_holder_ == nullptr;
    }

    template<typename U, typename = IsCastableTo<U>>
    BlockPtr<U> static_cast_to() && {
        BlockPtr<U> pp (reinterpret_cast<U*>(ptr_), ref_holder_);
        ref_holder_ = nullptr;
        return pp;
    }

    template<typename U, typename = IsCastableTo<U>>
    BlockPtr<U> static_cast_to() & {
        BlockPtr<U> pp (reinterpret_cast<U*>(ptr_), ref_holder_, EmptyType{});
        ref_holder_ = nullptr;
        return pp;
    }

    template<typename U, typename = IsCastableTo<U>>
    BlockPtr<U> const_cast_to() && {
        BlockPtr<U> pp(const_cast<U*>(reinterpret_cast<const U*>(ptr_)), ref_holder_);
        ref_holder_ = nullptr;
        return pp;
    }

    template<typename U, typename = IsCastableTo<U>>
    BlockPtr<U> const_cast_to() & {
        BlockPtr<U> pp(const_cast<U*>(reinterpret_cast<const U*>(ptr_)), ref_holder_, EmptyType{});
        ref_holder_ = nullptr;
        return pp;
    }
};

}
