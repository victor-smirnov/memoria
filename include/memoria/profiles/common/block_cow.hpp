
// Copyright 2011-2021 Victor Smirnov
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

#include <memoria/profiles/common/block.hpp>

#include <type_traits>

namespace memoria {

namespace detail {

}

template <typename PageT, typename StoreT, typename Shared_>
class CowSharedBlockPtr {

    template <typename BlkT>
    static constexpr bool IsConstCompatible = std::is_const<PageT>::value ? std::is_const<BlkT>::value : true;

public:
    using Page      = PageT;
    using BlockType = PageT;
    using Allocator = StoreT;
    using Shared    = Shared_;

    using MutableBlockType = std::remove_const_t<PageT>;

private:
    using SharedBlockT = typename Shared_::BlockType;


    mutable BlockType* ptr_;
    Shared* shared_;

    CowSharedBlockPtr(BlockType* ptr, Shared* shared) noexcept:
        ptr_(ptr), shared_(shared)
    {}

public:
    CowSharedBlockPtr(Shared* shared) noexcept:
        ptr_(cast_me(shared->ptr())), shared_(shared)
    {
        ref();
    }

    CowSharedBlockPtr() noexcept: shared_(nullptr) {}

    CowSharedBlockPtr(const CowSharedBlockPtr& guard) noexcept:
        ptr_(cast_me(guard.ptr_)), shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    CowSharedBlockPtr(const CowSharedBlockPtr<Page, StoreT, Shared>& guard) noexcept:
        ptr_(cast_me(guard.ptr_)), shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    CowSharedBlockPtr(CowSharedBlockPtr<Page, StoreT, Shared>&& guard) noexcept:
        ptr_(cast_me(guard.ptr_)),
        shared_(guard.shared_)
    {
        guard.shared_ = nullptr;
    }

    ~CowSharedBlockPtr() noexcept
    {
        unref();
    }


    CowSharedBlockPtr& as_mutable() & noexcept {
        return *this;
    }

    CowSharedBlockPtr as_mutable() const & noexcept {
        return CowSharedBlockPtr<MutableBlockType, StoreT, Shared>{shared_};
    }


    CowSharedBlockPtr as_mutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return CowSharedBlockPtr<MutableBlockType, StoreT, Shared>{ptr_, tmp};
    }

    CowSharedBlockPtr<const MutableBlockType, StoreT, Shared> as_immutable() & noexcept {
        return CowSharedBlockPtr<const MutableBlockType, StoreT, Shared>{shared_};
    }

    CowSharedBlockPtr<const MutableBlockType, StoreT, Shared> as_immutable() const & noexcept {
        return CowSharedBlockPtr<const MutableBlockType, StoreT, Shared>{shared_};
    }

    CowSharedBlockPtr<const MutableBlockType, StoreT, Shared> as_immutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return CowSharedBlockPtr<const MutableBlockType, StoreT, Shared>{ptr_, shared_};
    }



    const CowSharedBlockPtr& operator=(const CowSharedBlockPtr& guard) noexcept
    {
        if (shared_ != guard.shared_)
        {
            unref();
            ptr_ = cast_me(guard.ptr_);
            shared_ = guard.shared_;
            ref();
        }

        return *this;
    }


    template <typename P, typename = std::enable_if_t<IsConstCompatible<P>>>
    CowSharedBlockPtr& operator=(const CowSharedBlockPtr<P, StoreT, Shared>& guard) noexcept
    {
        unref();
        ptr_ = cast_me(guard.ptr_);
        shared_ = guard.shared_;
        ref();
        return *this;
    }

    CowSharedBlockPtr& operator=(CowSharedBlockPtr&& guard) noexcept
    {
        if (this != &guard)
        {
            unref();
            ptr_ = cast_me(guard.ptr_);
            shared_ = guard.shared_;
            guard.shared_ = nullptr;
        }

        return *this;
    }


    template <typename P, typename = std::enable_if_t<IsConstCompatible<P>>>
    CowSharedBlockPtr& operator=(CowSharedBlockPtr<P, StoreT, Shared>&& guard) noexcept
    {
        unref();
        ptr_ = cast_me(guard.ptr_);
        shared_ = guard.shared_;
        guard.shared_ = nullptr;

        return *this;
    }


    bool operator==(const PageT* block) const noexcept
    {
        return shared_ ? ptr_ == block : false;
    }

    bool operator!=(const PageT* block) const noexcept
    {
        return shared_ ? ptr_ != block : true;
    }

    bool isEmpty() const noexcept {
        return (!shared_) || shared_->get() == nullptr;
    }

    bool isSet() const noexcept {
        return shared_ && shared_->get() != nullptr;
    }

    bool operator==(const CowSharedBlockPtr& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() == other.shared_->id();
    }

    bool operator!=(const CowSharedBlockPtr& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() != other.shared_->id();
    }

    operator bool() const noexcept {
        return this->isSet();
    }

    PageT* ptr() const noexcept {
        return ptr_;
    }

    PageT* block() const noexcept {
        return ptr_;
    }

    void set_block(PageT* block) noexcept
    {
        ptr_ = block;
        shared_->set_block(block);
    }

    PageT* operator->() const noexcept {
        return ptr_;
    }

    void update() const
    {
        if (shared_)
        {
            shared_->store()->updateBlock(shared_);
            ptr_ = ptr_cast<PageT>(shared_->get());
        }
    }

//    VoidResult resize(int32_t new_size) noexcept
//    {
//        if (shared_ != nullptr)
//        {
//            return shared_->store()->resizeBlock(shared_, new_size);
//        }

//        return VoidResult::of();
//    }

    void clear() noexcept {
        *this = nullptr;
    }

    Shared* shared() const noexcept {
        return shared_;
    }

    Shared* shared() noexcept{
        return shared_;
    }

    template <typename, typename, typename> friend class CowSharedBlockPtr;

private:
    void ref() noexcept
    {
        if (shared_ != nullptr)
        {
            shared_->ref();
        }
    }

    void unref() noexcept
    {
        if (shared_)
        {
            if (shared_->unref())
            {
                shared_->store()->releaseBlock(shared_);
            }
        }
    }


    template <typename TT>
    static BlockType* cast_me(TT* blk) {
        return ptr_cast<BlockType>(blk);
    }
};




template <typename PageT, typename StoreT, typename Shared_>
class CowSharedBlockPtr<const PageT, StoreT, Shared_> {

    template <typename BlkT>
    static constexpr bool IsConstCompatible = std::is_const<BlkT>::value;

public:
    using Page      = PageT;
    using BlockType = PageT;
    using Allocator = StoreT;
    using Shared    = Shared_;

    using MutableBlockType = std::remove_const_t<PageT>;

private:
    using SharedBlockT = typename Shared_::BlockType;


    mutable MutableBlockType* ptr_;
    Shared* shared_;

    CowSharedBlockPtr(MutableBlockType* ptr, Shared* shared) noexcept:
        ptr_(ptr), shared_(shared)
    {}

public:
    CowSharedBlockPtr(Shared* shared) noexcept:
        ptr_(cast_me(shared->ptr())), shared_(shared)
    {
        ref();
    }

    CowSharedBlockPtr() noexcept: shared_(nullptr) {}

    CowSharedBlockPtr(const CowSharedBlockPtr& guard) noexcept:
        ptr_(cast_me(guard.ptr_)), shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    CowSharedBlockPtr(const CowSharedBlockPtr<Page, StoreT, Shared>& guard) noexcept:
        ptr_(cast_me(guard.ptr_)), shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    CowSharedBlockPtr(CowSharedBlockPtr<Page, StoreT, Shared>&& guard) noexcept:
        ptr_(cast_me(guard.ptr_)),
        shared_(guard.shared_)
    {
        guard.shared_ = nullptr;
    }

    ~CowSharedBlockPtr() noexcept
    {
        unref();
    }


    CowSharedBlockPtr& as_immutable() & noexcept {
        return *this;
    }

    CowSharedBlockPtr as_immutable() const & noexcept {
        return CowSharedBlockPtr<MutableBlockType, StoreT, Shared>{shared_};
    }

    CowSharedBlockPtr as_immutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return CowSharedBlockPtr<MutableBlockType, StoreT, Shared>{ptr_, tmp};
    }

    CowSharedBlockPtr<MutableBlockType, StoreT, Shared> as_mutable() & {
        shared_->assert_mutable();
        return CowSharedBlockPtr<MutableBlockType, StoreT, Shared>{shared_};
    }

    CowSharedBlockPtr<MutableBlockType, StoreT, Shared> as_mutable() const & {
        shared_->assert_mutable();
        return CowSharedBlockPtr<MutableBlockType, StoreT, Shared>{shared_};
    }

    CowSharedBlockPtr<MutableBlockType, StoreT, Shared> as_mutable() && {
        Shared* tmp = shared_;
        shared_ = nullptr;
        tmp->assert_mutable();
        return CowSharedBlockPtr<MutableBlockType, StoreT, Shared>{ptr_, tmp};
    }

    const CowSharedBlockPtr& operator=(const CowSharedBlockPtr& guard) noexcept
    {
        if (shared_ != guard.shared_)
        {
            unref();
            ptr_ = cast_me(guard.ptr_);
            shared_ = guard.shared_;
            ref();
        }

        return *this;
    }


    template <typename P, typename = std::enable_if_t<IsConstCompatible<P>>>
    CowSharedBlockPtr& operator=(const CowSharedBlockPtr<P, StoreT, Shared>& guard) noexcept
    {
        unref();
        ptr_ = cast_me(guard.ptr_);
        shared_ = guard.shared_;
        ref();
        return *this;
    }

    CowSharedBlockPtr& operator=(CowSharedBlockPtr&& guard) noexcept
    {
        if (this != &guard)
        {
            unref();
            ptr_ = cast_me(guard.ptr_);
            shared_ = guard.shared_;
            guard.shared_ = nullptr;
        }

        return *this;
    }


    template <typename P, typename = std::enable_if_t<IsConstCompatible<P>>>
    CowSharedBlockPtr& operator=(CowSharedBlockPtr<P, StoreT, Shared>&& guard) noexcept
    {
        if (this != &guard)
        {
            unref();
            ptr_ = cast_me(guard.ptr_);
            shared_ = guard.shared_;
            guard.shared_ = nullptr;
        }

        return *this;
    }


    bool operator==(const PageT* block) const noexcept
    {
        return shared_ ? ptr_ == block : false;
    }

    bool operator!=(const PageT* block) const noexcept
    {
        return shared_ ? ptr_ != block : true;
    }

    bool isEmpty() const noexcept {
        return (!shared_) || shared_->get() == nullptr;
    }

    bool isSet() const noexcept {
        return shared_ && shared_->get() != nullptr;
    }

    bool operator==(const CowSharedBlockPtr& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() == other.shared_->id();
    }

    bool operator!=(const CowSharedBlockPtr& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() != other.shared_->id();
    }

    operator bool() const noexcept {
        return this->isSet();
    }

    const PageT* ptr() const {
        return ptr_;
    }

    const PageT* block() const noexcept {
        return ptr_;
    }

    void set_block(PageT* block) noexcept
    {
        ptr_ = block;
        shared_->set_block(block);
    }

    const PageT* operator->() const noexcept {
        return ptr_;
    }

    void update() const
    {
        if (shared_)
        {
            shared_->store()->updateBlock(shared_);
            ptr_ = ptr_cast<PageT>(shared_->get());
        }
    }

//    VoidResult resize(int32_t new_size) noexcept
//    {
//        if (shared_ != nullptr)
//        {
//            return shared_->store()->resizeBlock(shared_, new_size);
//        }

//        return VoidResult::of();
//    }

    void clear() noexcept {
        *this = nullptr;
    }

    Shared* shared() const noexcept {
        return shared_;
    }

    template <typename, typename, typename> friend class CowSharedBlockPtr;

private:
    void ref() noexcept
    {
        if (shared_ != nullptr)
        {
            shared_->ref();
        }
    }

    void unref() noexcept
    {
        if (shared_)
        {
            if (shared_->unref())
            {
                shared_->store()->releaseBlock(shared_);
            }
        }
    }


    template <typename TT>
    static BlockType* cast_me(TT* blk) {
        return ptr_cast<BlockType>(blk);
    }
};







template <
        typename T,
        typename U,
        typename StoreT,
        typename Shared
>
Result<T> static_cast_block(Result<CowSharedBlockPtr<U, StoreT, Shared>>&& src) noexcept {
    using BlkPtrT = T;
    using ResultT = Result<BlkPtrT>;

    if (MMA_LIKELY(src.is_ok()))
    {
        BlkPtrT tgt(std::move(src).get());
        return ResultT::of(std::move(tgt));
    }
    return std::move(src).transfer_error();
}

template <
        typename T,
        typename U,
        typename StoreT,
        typename Shared
>
T static_cast_block(CowSharedBlockPtr<U, StoreT, Shared>&& src) noexcept {
    T tgt(std::move(src));
    return std::move(tgt);
}


template <
        typename T,
        typename U,
        typename StoreT,
        typename Shared,
        typename = std::enable_if_t<
            std::is_const<typename T::BlockType>::value
        >
>
Result<T> static_cast_block(Result<CowSharedBlockPtr<const U, StoreT, Shared>>&& src) noexcept {
    using BlkPtrT = T;
    using ResultT = Result<BlkPtrT>;

    if (MMA_LIKELY(src.is_ok()))
    {
        BlkPtrT tgt(std::move(src).get());
        return ResultT::of(std::move(tgt));
    }
    return std::move(src).transfer_error();
}


template <
        typename T,
        typename U,
        typename StoreT,
        typename Shared,
        typename = std::enable_if_t<
            std::is_const<typename T::BlockType>::value
        >
>
T static_cast_block(CowSharedBlockPtr<const U, StoreT, Shared>&& src) noexcept {
    T tgt(std::move(src));
    return std::move(tgt);
}


}
