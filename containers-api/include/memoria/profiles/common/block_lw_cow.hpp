
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


template <typename BlockType_>
class LWSharedBlockPtr {

    template <typename BlkT>
    static constexpr bool IsConstCompatible = !std::is_const<BlkT>::value;

public:
    using MutableBlockType = BlockType_;

private:
    MutableBlockType* ptr_;

    template <typename> friend class LWSharedBlockPtr;

public:
    enum {UNDEFINED, READ, UPDATE, _DELETE};

    using Shared = LWSharedBlockPtr;
    using BlockType = BlockType_;

    LWSharedBlockPtr() noexcept:
        ptr_()
    {}

    template <typename U, typename = std::enable_if_t<IsConstCompatible<U>>>
    LWSharedBlockPtr(LWSharedBlockPtr<U>&& other) noexcept:
        ptr_(ptr_cast<MutableBlockType>(other.ptr_))
    {}

    template <typename U, typename = std::enable_if_t<IsConstCompatible<U>>>
    LWSharedBlockPtr(const LWSharedBlockPtr<U>& other) noexcept:
        ptr_(ptr_cast<MutableBlockType>(other.ptr_))
    {}

    LWSharedBlockPtr(MutableBlockType* ptr) noexcept:
        ptr_(ptr)
    {}

    ~LWSharedBlockPtr() noexcept = default;

    LWSharedBlockPtr<MutableBlockType> as_mutable() const noexcept {
        return LWSharedBlockPtr<MutableBlockType>{ptr_};
    }

    LWSharedBlockPtr<const BlockType> as_immutable() const noexcept {
        return LWSharedBlockPtr<const BlockType>{ptr_};
    }


    operator bool() const noexcept {
        return ptr_;
    }

    bool isSet() const noexcept {
        return ptr_;
    }

    bool is_null() const noexcept {
        return ptr_ == nullptr;
    }

    template <typename TT>
    bool operator==(const LWSharedBlockPtr<TT>& other) const noexcept {
        return ptr_ == other.ptr_;
    }

    BlockType* block() noexcept {
        return ptr_;
    }

    BlockType* block() const noexcept {
        return ptr_;
    }

    BlockType* get() noexcept {
        return ptr_;
    }

    BlockType* get() const noexcept {
        return ptr_;
    }

    void update() const noexcept {}

    BlockType* operator->() noexcept {
        return ptr_;
    }

    BlockType* operator->() const noexcept {
        return ptr_;
    }

    Shared* shared() noexcept {
        return this;
    }

    Shared* shared() const noexcept {
        return this;
    }
};




template <typename BlockType_>
class LWSharedBlockPtr<const BlockType_> {

    template <typename BlkT>
    static constexpr bool IsConstCompatible = true;

public:
    using MutableBlockType = std::remove_const_t<BlockType_>;

private:
    MutableBlockType* ptr_;

    template <typename> friend class LWSharedBlockPtr;

public:
    enum {UNDEFINED, READ, UPDATE, _DELETE};

    using Shared = LWSharedBlockPtr;
    using BlockType = BlockType_;

    LWSharedBlockPtr() noexcept:
        ptr_()
    {}

    template <typename U, typename = std::enable_if_t<IsConstCompatible<U>>>
    LWSharedBlockPtr(LWSharedBlockPtr<U>&& other) noexcept:
        ptr_(ptr_cast<MutableBlockType>(other.ptr_))
    {}

    template <typename U, typename = std::enable_if_t<IsConstCompatible<U>>>
    LWSharedBlockPtr(const LWSharedBlockPtr<U>& other) noexcept:
        ptr_(ptr_cast<MutableBlockType>(other.ptr_))
    {}

    LWSharedBlockPtr(MutableBlockType* ptr) noexcept:
        ptr_(ptr)
    {}

    ~LWSharedBlockPtr() noexcept = default;

    LWSharedBlockPtr<MutableBlockType> as_mutable() const noexcept {
        return LWSharedBlockPtr<MutableBlockType>{ptr_};
    }

    LWSharedBlockPtr<const BlockType> as_immutable() const noexcept {
        return LWSharedBlockPtr<const BlockType>{ptr_};
    }


    operator bool() const noexcept {
        return ptr_;
    }

    bool isSet() const noexcept {
        return ptr_;
    }

    bool is_null() const noexcept {
        return ptr_ == nullptr;
    }

    template <typename TT>
    bool operator==(const LWSharedBlockPtr<TT>& other) const noexcept {
        return ptr_ == other.ptr_;
    }

    const BlockType* block() const noexcept {
        return ptr_;
    }

    const BlockType* get() const noexcept {
        return ptr_;
    }


    void update() const noexcept {
    }


    const BlockType* operator->() const noexcept {
        return ptr_;
    }

    Shared* shared() noexcept {
        return this;
    }

    const Shared* shared() const noexcept {
        return this;
    }
};








template <
        typename T,
        typename U
>
Result<T> static_cast_block(Result<LWSharedBlockPtr<U>>&& src) noexcept {
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
        typename U
>
T static_cast_block(LWSharedBlockPtr<U>&& src) noexcept {
    T tgt(std::move(src));
    return std::move(tgt);
}



template <
        typename T,
        typename U,
        typename = std::enable_if_t<
            std::is_const<typename T::BlockType>::value
        >
>
Result<T> static_cast_block(Result<LWSharedBlockPtr<const U>>&& src) noexcept {
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
        typename = std::enable_if_t<
            std::is_const<typename T::BlockType>::value
        >
>
T static_cast_block(LWSharedBlockPtr<const U>&& src) noexcept {
    T tgt(std::move(src));
    return std::move(tgt);
}


}
