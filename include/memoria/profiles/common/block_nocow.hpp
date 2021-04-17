
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




template <typename PageT, typename AllocatorT, typename Shared_>
class NoCowSharedBlockPtr {

    template <typename BlkT>
    static constexpr bool IsConstCompatible = true;

public:
    using Page      = PageT;
    using BlockType = PageT;
    using Allocator = AllocatorT;
    using Shared    = Shared_;

    using MutableBlockType = std::remove_const_t<PageT>;

private:
    mutable Shared* shared_;

    struct NoRef{};

    NoCowSharedBlockPtr(Shared* shared, NoRef) noexcept:
        shared_(shared)
    {
    }

public:
    NoCowSharedBlockPtr(Shared* shared) noexcept:
        shared_(shared)
    {
        ref();
    }

    NoCowSharedBlockPtr() noexcept: shared_(nullptr) {}

    NoCowSharedBlockPtr(const NoCowSharedBlockPtr& guard) noexcept:
        shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    NoCowSharedBlockPtr(const NoCowSharedBlockPtr<Page, AllocatorT, Shared>& guard) noexcept:
        shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    NoCowSharedBlockPtr(NoCowSharedBlockPtr<Page, AllocatorT, Shared>&& guard) noexcept: shared_(guard.shared_)
    {
        guard.shared_ = nullptr;
    }

    ~NoCowSharedBlockPtr() noexcept
    {
        unref();
    }

    NoCowSharedBlockPtr& as_mutable() & noexcept {
        return *this;
    }

    NoCowSharedBlockPtr as_mutable() const & noexcept {
        return NoCowSharedBlockPtr<MutableBlockType, AllocatorT, Shared>{shared_};
    }

    NoCowSharedBlockPtr as_mutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return NoCowSharedBlockPtr<MutableBlockType, AllocatorT, Shared>{tmp, NoRef{}};
    }

    NoCowSharedBlockPtr<const MutableBlockType, AllocatorT, Shared> as_immutable() & noexcept {
        return NoCowSharedBlockPtr<const MutableBlockType, AllocatorT, Shared>{shared_};
    }

    NoCowSharedBlockPtr<const MutableBlockType, AllocatorT, Shared> as_immutable() const& noexcept {
        return NoCowSharedBlockPtr<const MutableBlockType, AllocatorT, Shared>{shared_};
    }


    NoCowSharedBlockPtr<const MutableBlockType, AllocatorT, Shared> as_immutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return NoCowSharedBlockPtr<const MutableBlockType, AllocatorT, Shared>{tmp, NoRef{}};
    }


    const NoCowSharedBlockPtr& operator=(const NoCowSharedBlockPtr& guard) noexcept
    {
        if (shared_ != guard.shared_)
        {
            unref();
            shared_ = guard.shared_;
            ref();
        }

        return *this;
    }


    template <typename P, typename = std::enable_if_t<IsConstCompatible<P>>>
    NoCowSharedBlockPtr& operator=(const NoCowSharedBlockPtr<P, AllocatorT, Shared>& guard) noexcept
    {
        unref();
        shared_ = guard.shared_;
        ref();
        return *this;
    }

    NoCowSharedBlockPtr& operator=(NoCowSharedBlockPtr&& guard) noexcept
    {
        if (this != &guard)
        {
            unref();

            shared_ = guard.shared_;
            guard.shared_ = nullptr;
        }

        return *this;
    }


    template <typename P, typename = std::enable_if_t<IsConstCompatible<P>>>
    NoCowSharedBlockPtr& operator=(NoCowSharedBlockPtr<P, AllocatorT, Shared>&& guard) noexcept
    {
        unref();

        shared_ = guard.shared_;
        guard.shared_ = nullptr;

        return *this;
    }


    bool operator==(const PageT* block) const noexcept
    {
        return shared_ ? *shared_ == block : shared_ == block;
    }

    bool operator!=(const PageT* block) const noexcept
    {
        return shared_ ? *shared_ != block : shared_ != block;
    }

    bool isEmpty() const noexcept {
        return (!shared_) || shared_->get() == nullptr;
    }

    bool isSet() const noexcept {
        return shared_ && shared_->get() != nullptr;
    }

    bool operator==(const NoCowSharedBlockPtr& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() == other.shared_->id();
    }

    bool operator!=(const NoCowSharedBlockPtr& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() != other.shared_->id();
    }

    operator bool() const noexcept {
        return this->isSet();
    }

    PageT* block() const noexcept {
        return *shared_;
    }

    PageT* block() noexcept {
        return *shared_;
    }

    void set_block(PageT* block) noexcept
    {
        shared_->set_block(block);
    }

    PageT* operator->() const noexcept {
        return *shared_;
    }

    PageT* operator->() noexcept {
        return *shared_;
    }

    bool is_updated() const noexcept
    {
        return shared_->updated();
    }



    VoidResult update() const noexcept
    {
        if (shared_)
        {
            MEMORIA_TRY(guard, shared_->store()->updateBlock(shared_));
            if (guard.shared() != shared_)
            {
                terminate("Allocator returned different shared block for update()");
            }
        }

        return VoidResult::of();
    }

    VoidResult resize(int32_t new_size) noexcept
    {
        if (shared_ != nullptr)
        {
            return shared_->store()->resizeBlock(shared_, new_size);
        }

        return VoidResult::of();
    }

    void clear() noexcept {
        *this = nullptr;
    }

    Shared* shared() const noexcept {
        return shared_;
    }

    Shared* shared() noexcept{
        return shared_;
    }

    template <typename, typename, typename> friend class NoCowSharedBlockPtr;

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
};



template <typename PageT, typename AllocatorT, typename Shared_>
class NoCowSharedBlockPtr<const PageT, AllocatorT, Shared_> {

    template <typename BlkT>
    static constexpr bool IsConstCompatible = std::is_const<BlkT>::value;

public:
    using Page      = PageT;
    using BlockType = PageT;
    using Allocator = AllocatorT;
    using Shared    = Shared_;

    using MutableBlockType = std::remove_const_t<PageT>;

private:
    mutable Shared* shared_;

    struct NoRef{};

    NoCowSharedBlockPtr(Shared* shared, NoRef) noexcept:
        shared_(shared)
    {
    }

public:
    NoCowSharedBlockPtr(Shared* shared) noexcept:
        shared_(shared)
    {
        ref();
    }

    NoCowSharedBlockPtr() noexcept: shared_(nullptr) {}

    NoCowSharedBlockPtr(const NoCowSharedBlockPtr& guard) noexcept:
        shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    NoCowSharedBlockPtr(const NoCowSharedBlockPtr<Page, AllocatorT, Shared>& guard) noexcept:
        shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    NoCowSharedBlockPtr(NoCowSharedBlockPtr<Page, AllocatorT, Shared>&& guard) noexcept: shared_(guard.shared_)
    {
        guard.shared_ = nullptr;
    }

    ~NoCowSharedBlockPtr() noexcept
    {
        unref();
    }


    NoCowSharedBlockPtr& as_immutable() & noexcept {
        return *this;
    }

    NoCowSharedBlockPtr as_immutable() const & noexcept {
        return NoCowSharedBlockPtr<const MutableBlockType, AllocatorT, Shared>{shared_};
    }

    NoCowSharedBlockPtr as_immutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return NoCowSharedBlockPtr<const MutableBlockType, AllocatorT, Shared>{tmp, NoRef{}};
    }

    NoCowSharedBlockPtr<MutableBlockType, AllocatorT, Shared> as_mutable() & noexcept {
        return NoCowSharedBlockPtr<MutableBlockType, AllocatorT, Shared>{shared_};
    }

    NoCowSharedBlockPtr<MutableBlockType, AllocatorT, Shared> as_mutable() const& noexcept {
        return NoCowSharedBlockPtr<MutableBlockType, AllocatorT, Shared>{shared_};
    }

    NoCowSharedBlockPtr<MutableBlockType, AllocatorT, Shared> as_mutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return NoCowSharedBlockPtr<MutableBlockType, AllocatorT, Shared>{tmp, NoRef{}};
    }

    const NoCowSharedBlockPtr& operator=(const NoCowSharedBlockPtr& guard) noexcept
    {
        if (shared_ != guard.shared_)
        {
            unref();
            shared_ = guard.shared_;
            ref();
        }

        return *this;
    }


    template <typename P, typename = std::enable_if_t<IsConstCompatible<P>>>
    NoCowSharedBlockPtr& operator=(const NoCowSharedBlockPtr<P, AllocatorT, Shared>& guard) noexcept
    {
        unref();
        shared_ = guard.shared_;
        ref();
        return *this;
    }

    NoCowSharedBlockPtr& operator=(NoCowSharedBlockPtr&& guard) noexcept
    {
        if (this != &guard)
        {
            unref();

            shared_ = guard.shared_;
            guard.shared_ = nullptr;
        }

        return *this;
    }


    template <typename P, typename = std::enable_if_t<IsConstCompatible<P>>>
    NoCowSharedBlockPtr& operator=(NoCowSharedBlockPtr<P, AllocatorT, Shared>&& guard) noexcept
    {
        if (this != &guard)
        {
            unref();

            shared_ = guard.shared_;
            guard.shared_ = nullptr;
        }

        return *this;
    }


    bool operator==(const PageT* block) const noexcept
    {
        return shared_ ? *shared_ == block : shared_ == block;
    }

    bool operator!=(const PageT* block) const noexcept
    {
        return shared_ ? *shared_ != block : shared_ != block;
    }

    bool isEmpty() const noexcept {
        return (!shared_) || shared_->get() == nullptr;
    }

    bool isSet() const noexcept {
        return shared_ && shared_->get() != nullptr;
    }

    bool operator==(const NoCowSharedBlockPtr& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() == other.shared_->id();
    }

    bool operator!=(const NoCowSharedBlockPtr& other) const noexcept
    {
        return shared_ && other.shared_ && shared_->id() != other.shared_->id();
    }

    operator bool() const noexcept {
        return this->isSet();
    }

    const PageT* block() const noexcept {
        return *shared_;
    }


    void set_block(PageT* block) noexcept
    {
        shared_->set_block(block);
    }

    const PageT* operator->() const noexcept {
        return *shared_;
    }

    bool is_updated() const noexcept
    {
        return shared_->updated();
    }



    VoidResult update() const noexcept
    {
        if (shared_)
        {
            MEMORIA_TRY(guard, shared_->store()->updateBlock(shared_));
            if (guard.shared() != shared_)
            {
                terminate("Allocator returned different shared block for update()");
            }
        }

        return VoidResult::of();
    }

    VoidResult resize(int32_t new_size) noexcept
    {
        if (shared_ != nullptr)
        {
            return shared_->store()->resizeBlock(shared_, new_size);
        }

        return VoidResult::of();
    }

    void clear() noexcept {
        *this = nullptr;
    }

    Shared* shared() const noexcept {
        return shared_;
    }

    template <typename, typename, typename> friend class NoCowSharedBlockPtr;

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
};






template <
        typename T,
        typename U,
        typename AllocatorT,
        typename Shared
>
Result<T> static_cast_block(Result<NoCowSharedBlockPtr<U, AllocatorT, Shared>>&& src) noexcept {
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
        typename AllocatorT,
        typename Shared,
        typename = std::enable_if_t<
            std::is_const<typename T::BlockType>::value
        >
>
Result<T> static_cast_block(Result<NoCowSharedBlockPtr<const U, AllocatorT, Shared>>&& src) noexcept {
    using BlkPtrT = T;
    using ResultT = Result<BlkPtrT>;

    if (MMA_LIKELY(src.is_ok()))
    {
        BlkPtrT tgt(std::move(src).get());
        return ResultT::of(std::move(tgt));
    }
    return std::move(src).transfer_error();
}

}
