
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

template <typename PageT, typename AllocatorT, typename Shared_>
class CowSharedBlockHandler {

    template <typename BlkT>
    static constexpr bool IsConstCompatible = std::is_const<PageT>::value ? std::is_const<BlkT>::value : true;

public:
    using Page      = PageT;
    using BlockType = PageT;
    using Allocator = AllocatorT;
    using Shared    = Shared_;

    using MutableBlockType = std::remove_const_t<PageT>;

private:
    using SharedBlockT = typename Shared_::BlockType;


    BlockType* ptr_;
    Shared* shared_;

    CowSharedBlockHandler(BlockType* ptr, Shared* shared) noexcept:
        ptr_(ptr), shared_(shared)
    {}

public:
    CowSharedBlockHandler(Shared* shared) noexcept:
        ptr_(cast_me(shared->ptr())), shared_(shared)
    {
        ref();
    }

    CowSharedBlockHandler() noexcept: shared_(nullptr) {}

    CowSharedBlockHandler(const CowSharedBlockHandler& guard) noexcept:
        ptr_(cast_me(guard.ptr_)), shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    CowSharedBlockHandler(const CowSharedBlockHandler<Page, AllocatorT, Shared>& guard) noexcept:
        ptr_(cast_me(guard.ptr_)), shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    CowSharedBlockHandler(CowSharedBlockHandler<Page, AllocatorT, Shared>&& guard) noexcept:
        ptr_(cast_me(guard.ptr_)),
        shared_(guard.shared_)
    {
        guard.shared_ = nullptr;
    }

    ~CowSharedBlockHandler() noexcept
    {
        unref();
    }

    CowSharedBlockHandler<MutableBlockType, AllocatorT, Shared> as_mutable() & noexcept {
        return CowSharedBlockHandler<MutableBlockType, AllocatorT, Shared>{shared_};
    }

    CowSharedBlockHandler<MutableBlockType, AllocatorT, Shared> as_mutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return CowSharedBlockHandler<MutableBlockType, AllocatorT, Shared>{ptr_, shared_};
    }

    const CowSharedBlockHandler& operator=(const CowSharedBlockHandler& guard) noexcept
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
    CowSharedBlockHandler& operator=(const CowSharedBlockHandler<P, AllocatorT, Shared>& guard) noexcept
    {
        unref();
        ptr_ = cast_me(guard.ptr_);
        shared_ = guard.shared_;
        ref();
        return *this;
    }

    CowSharedBlockHandler& operator=(CowSharedBlockHandler&& guard) noexcept
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
    CowSharedBlockHandler& operator=(CowSharedBlockHandler<P, AllocatorT, Shared>&& guard) noexcept
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

    bool operator==(const CowSharedBlockHandler& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() == other.shared_->id();
    }

    bool operator!=(const CowSharedBlockHandler& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() != other.shared_->id();
    }

    operator bool() const noexcept {
        return this->isSet();
    }

    PageT* ptr() const {
        return ptr_;
    }

    const PageT* block() const noexcept {
        return ptr_;
    }

    PageT* block() noexcept {
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

    PageT* operator->() noexcept {
        return ptr_;
    }


    VoidResult update() noexcept
    {
        if (shared_)
        {
            MEMORIA_TRY(guard, shared_->store()->updateBlock(shared_));
            if (guard.shared() != shared_)
            {
                *this = guard;
            }
        }

        return VoidResult::of();
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

    const Shared* shared() const noexcept {
        return shared_;
    }

    Shared* shared() noexcept{
        return shared_;
    }

    template <typename, typename, typename> friend class CowSharedBlockHandler;

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
                shared_->store()->releaseBlock(shared_).get_or_throw();
            }
        }
    }


    template <typename TT>
    static BlockType* cast_me(TT* blk) {
        return ptr_cast<BlockType>(blk);
    }
};


template <typename T, typename U, typename AllocatorT, typename Shared>
Result<T> static_cast_block(Result<CowSharedBlockHandler<U, AllocatorT, Shared>>&& src) noexcept {
    if (MMA_LIKELY(src.is_ok()))
    {
        T tgt = std::move(src).get();
        return Result<T>::of(std::move(tgt));
    }
    return std::move(src).transfer_error();
}


}
