
// Copyright 2011-2025 Victor Smirnov
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




template <typename PageT, typename StoreT, typename Shared_>
class NoCowSharedBlockPtr {

    template <typename BlkT>
    static constexpr bool IsConstCompatible = true;

public:
    using Page      = PageT;
    using BlockType = PageT;
    using Allocator = StoreT;
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
    NoCowSharedBlockPtr(const NoCowSharedBlockPtr<Page, StoreT, Shared>& guard) noexcept:
        shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    NoCowSharedBlockPtr(NoCowSharedBlockPtr<Page, StoreT, Shared>&& guard) noexcept: shared_(guard.shared_)
    {
        guard.shared_ = nullptr;
    }

    ~NoCowSharedBlockPtr() noexcept
    {
        unref();
    }

    NoCowSharedBlockPtr& as_mutable() & noexcept {
        if (!shared_ || shared_->is_mutable()) {
            return *this;
        }
        else {
            terminate("Trying to mutate immutable block");
        }
    }

    NoCowSharedBlockPtr as_mutable() const & noexcept {
        if (!shared_ || shared_->is_mutable()) {
            return NoCowSharedBlockPtr<MutableBlockType, StoreT, Shared>{shared_};
        }
        else {
            terminate("Trying to mutate immutable block");
        }
    }

    NoCowSharedBlockPtr as_mutable() && noexcept {
        if (!shared_ || shared_->is_mutable()) {
            Shared* tmp = shared_;
            shared_ = nullptr;
            return NoCowSharedBlockPtr<MutableBlockType, StoreT, Shared>{tmp, NoRef{}};
        }
        else {
            terminate("Trying to mutate immutable block");
        }
    }

    NoCowSharedBlockPtr<const MutableBlockType, StoreT, Shared> as_immutable() & noexcept {
        return NoCowSharedBlockPtr<const MutableBlockType, StoreT, Shared>{shared_};
    }

    NoCowSharedBlockPtr<const MutableBlockType, StoreT, Shared> as_immutable() const& noexcept {
        return NoCowSharedBlockPtr<const MutableBlockType, StoreT, Shared>{shared_};
    }


    NoCowSharedBlockPtr<const MutableBlockType, StoreT, Shared> as_immutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return NoCowSharedBlockPtr<const MutableBlockType, StoreT, Shared>{tmp, NoRef{}};
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
    NoCowSharedBlockPtr& operator=(const NoCowSharedBlockPtr<P, StoreT, Shared>& guard) noexcept
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
    NoCowSharedBlockPtr& operator=(NoCowSharedBlockPtr<P, StoreT, Shared>&& guard) noexcept
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



    void update() const
    {
        if (shared_)
        {
            shared_->store()->updateBlock(shared_);
        }
    }

    void resize(int32_t new_size)
    {
        if (shared_)
        {
            shared_->store()->resizeBlock(shared_, new_size);
        }
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
                shared_->store()->releaseBlock(shared_);
            }
        }
    }
};



template <typename PageT, typename StoreT, typename Shared_>
class NoCowSharedBlockPtr<const PageT, StoreT, Shared_> {

    template <typename BlkT>
    static constexpr bool IsConstCompatible = std::is_const<BlkT>::value;

public:
    using Page      = PageT;
    using BlockType = PageT;
    using Allocator = StoreT;
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
    NoCowSharedBlockPtr(const NoCowSharedBlockPtr<Page, StoreT, Shared>& guard) noexcept:
        shared_(guard.shared_)
    {
        ref();
    }

    template <typename Page, typename = std::enable_if_t<IsConstCompatible<Page>>>
    NoCowSharedBlockPtr(NoCowSharedBlockPtr<Page, StoreT, Shared>&& guard) noexcept: shared_(guard.shared_)
    {
        guard.shared_ = nullptr;
    }

    ~NoCowSharedBlockPtr() noexcept {
        unref();
    }


    NoCowSharedBlockPtr& as_immutable() & noexcept {
        return *this;
    }

    NoCowSharedBlockPtr as_immutable() const & noexcept {
        return NoCowSharedBlockPtr<const MutableBlockType, StoreT, Shared>{shared_};
    }

    NoCowSharedBlockPtr as_immutable() && noexcept {
        Shared* tmp = shared_;
        shared_ = nullptr;
        return NoCowSharedBlockPtr<const MutableBlockType, StoreT, Shared>{tmp, NoRef{}};
    }

    NoCowSharedBlockPtr<MutableBlockType, StoreT, Shared> as_mutable() & noexcept {
        if (!shared_ || shared_->is_mutable()) {
            return NoCowSharedBlockPtr<MutableBlockType, StoreT, Shared>{shared_};
        }
        else {
            terminate("Trying to mutate immutable block");
        }
    }

    NoCowSharedBlockPtr<MutableBlockType, StoreT, Shared> as_mutable() const& noexcept {        
        if (!shared_ || shared_->is_mutable()) {
            return NoCowSharedBlockPtr<MutableBlockType, StoreT, Shared>{shared_};
        }
        else {
            terminate("Trying to mutate immutable block");
        }
    }

    NoCowSharedBlockPtr<MutableBlockType, StoreT, Shared> as_mutable() && noexcept {
        if (!shared_ || shared_->is_mutable())
        {
            Shared* tmp = shared_;
            shared_ = nullptr;
            return NoCowSharedBlockPtr<MutableBlockType, StoreT, Shared>{tmp, NoRef{}};
        }
        else {
            terminate("Trying to mutate immutable block");
        }
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
    NoCowSharedBlockPtr& operator=(const NoCowSharedBlockPtr<P, StoreT, Shared>& guard) noexcept
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
    NoCowSharedBlockPtr& operator=(NoCowSharedBlockPtr<P, StoreT, Shared>&& guard) noexcept
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


    void set_block(PageT* block) noexcept {
        shared_->set_block(block);
    }

    const PageT* operator->() const noexcept {
        return *shared_;
    }

    bool is_updated() const noexcept {
        return shared_->updated();
    }



    void update() const {
        if (shared_) {
            shared_->store()->updateBlock(shared_);
        }
    }

    void resize(int32_t new_size) {
        if (shared_ != nullptr) {
            return shared_->store()->resizeBlock(shared_, new_size);
        }
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
        if (shared_ != nullptr) {
            shared_->ref();
        }
    }

    void unref() noexcept
    {
        if (shared_) {
            if (shared_->unref()) {
                shared_->store()->releaseBlock(shared_);
            }
        }
    }
};






template <
        typename T,
        typename U,
        typename StoreT,
        typename Shared
>
Result<T> static_cast_block(Result<NoCowSharedBlockPtr<U, StoreT, Shared>>&& src) noexcept {
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
T static_cast_block(NoCowSharedBlockPtr<U, StoreT, Shared>&& src) noexcept {
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
Result<T> static_cast_block(Result<NoCowSharedBlockPtr<const U, StoreT, Shared>>&& src) noexcept {
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
T static_cast_block(NoCowSharedBlockPtr<const U, StoreT, Shared>&& src) noexcept {
    T tgt(std::move(src));
    return std::move(tgt);
}

}
