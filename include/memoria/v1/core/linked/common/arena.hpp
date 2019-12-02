
// Copyright 2019 Victor Smirnov
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


#include <memoria/v1/core/tools/arena_buffer.hpp>
#include <memoria/v1/core/types/type2type.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/containers/map.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <utility>
#include <type_traits>
#include <unordered_map>

namespace memoria {
namespace v1 {

struct ArenaException: virtual RuntimeException {};
struct UnmodifiableArenaException: virtual ArenaException {};

namespace mapped_ {
    template <typename, typename = VoidT<>>
    struct HasObjectSizeHelper: std::false_type {};

    template <typename T>
    struct HasObjectSizeHelper<T, VoidT<decltype(T::UseObjectSize)>>: std::true_type {};
}


template <typename T>
constexpr bool LinkedHasObjectSize = mapped_::HasObjectSizeHelper<T>::type::value;




template <typename T, typename HolderT, typename Arena>
class LinkedPtr {
    HolderT ptr_value_;
public:
    using Ptr = T*;
    using ValueT = T;

    LinkedPtr() = default;
    LinkedPtr(HolderT value): ptr_value_(value) {}

    HolderT get() const {
        return ptr_value_;
    }

    T* get(Arena* arena) {
        return T2T<T*>(arena->data() + ptr_value_);
    }

    T* get_mutable(const Arena* arena) {
        return T2T<T*>(arena->mutable_data() + ptr_value_);
    }

    const T* get(const Arena* arena) const {
        return T2T<const T*>(arena->data() + ptr_value_);
    }

    operator HolderT() const {
        return ptr_value_;
    }
};


template <typename T, typename HolderT, typename Arena>
class GenericLinkedPtr {
    HolderT ptr_value_;
public:
    using Ptr = T*;
    using ValueT = T;

    GenericLinkedPtr() = default;
    GenericLinkedPtr(HolderT value): ptr_value_(value) {}

    template <typename TT>
    GenericLinkedPtr(LinkedPtr<TT, HolderT, Arena> other): ptr_value_(other.get()) {}

    HolderT get() const {
        return ptr_value_;
    }

    operator HolderT() const {
        return ptr_value_;
    }
};





template <typename T, typename Arena>
class LinkedPtrResolver {
    T ptr_;
    Arena* arena_;
public:
    LinkedPtrResolver(T ptr, Arena* arena): ptr_(ptr), arena_(arena) {}

    auto get() const {
        return ptr_.get(arena_);
    }
};


namespace mapped_ {

    template <typename T, bool HasObjectSize = LinkedHasObjectSize<T>> struct ObjectCreator;

    template <typename T>
    struct ObjectCreator<T, false> {

        template <typename Arena, typename... CtrArgs>
        static auto allocate_and_construct(size_t tag_size, Arena* arena, CtrArgs&&... args)
        {
            size_t size = sizeof(T);
            auto addr = arena->template allocate_space<T>(size, tag_size);
            arena->construct(addr, std::forward<CtrArgs>(args)...);
            return addr;
        }
    };

    template <typename T>
    struct ObjectCreator<T, true> {
        template <typename Arena, typename... CtrArgs>
        static auto allocate_and_construct(size_t tag_size, Arena* arena, CtrArgs&&... args)
        {
            size_t size = T::object_size(std::forward<CtrArgs>(args)...);
            auto addr = arena->template allocate_space<T>(size, tag_size);
            arena->construct(addr, std::forward<CtrArgs>(args)...);
            return addr;
        }
    };
}




template <typename HeaderT, typename PtrHolderT>
class LinkedArena;


template <typename HeaderT_, typename PtrHolderT_>
class LinkedArenaView {
protected:

    uint8_t* data_;
    size_t size_;

    LinkedArena<HeaderT_, PtrHolderT_>* arena_;

    template <typename, typename>
    friend class LinkedArena;
public:
    using AtomType = uint8_t;

    using PtrHolderT = PtrHolderT_;
    using AddressMapping = std::unordered_map<PtrHolderT, PtrHolderT>;

    template <typename T>
    using PtrT = LinkedPtr<T, PtrHolderT, LinkedArenaView>;

    template <typename T>
    using GenericPtrT = GenericLinkedPtr<T, PtrHolderT, LinkedArenaView>;

    LinkedArenaView() = default;

    LinkedArenaView(Span<const AtomType> span):
        data_(const_cast<AtomType*>(span.data())),
        size_(span.size()),
        arena_()
    {}

    void clear_arena_ptr() noexcept {
        arena_ = nullptr;
    }

    void clear_arena(size_t extra_size = 0) noexcept {
        arena_->clear(extra_size);
    }

    void reset_arena(size_t extra_size = 0) noexcept {
        arena_->reset(extra_size);
    }

    void set_size(size_t size) {
        this->size_ = size;
    }

    AddressMapping make_address_mapping() const noexcept {
        return AddressMapping();
    }

    LinkedArenaView* make_mutable() const {
        if (arena_) {
            return const_cast<LinkedArenaView*>(this);
        }

        MMA1_THROW(RuntimeException()) << WhatCInfo("Data is not mutable");
    }

    bool is_mutable() const {
        return arena_ != nullptr;
    }

    MMA1_NODISCARD uint8_t* mutable_data() const {
        if (arena_) {
            return data_;
        }

        MMA1_THROW(RuntimeException()) << WhatCInfo("Data is not mutable");
    }

    MMA1_NODISCARD const uint8_t* data() const {return data_;}

    MMA1_NODISCARD size_t allocate_space(size_t size, size_t alignment, size_t tag_size);

    template <typename T>
    MMA1_NODISCARD PtrT<T> allocate_space(size_t size, size_t tag_size = 0) noexcept
    {
        size_t alignment = alignof(T);
        return allocate_space(size, alignment, tag_size);
    }


    template <typename T, typename... CtrArgs>
    MMA1_NODISCARD PtrT<T> allocate_explicit(size_t size, CtrArgs&&... args) noexcept {
        size_t ptr = allocate_space<T>(size);
        construct<T>(ptr, std::forward<CtrArgs>(args)...);
        return ptr;
    }



    template <typename T, typename... CtrArgs>
    MMA1_NODISCARD PtrT<T> allocate(CtrArgs&&... args) noexcept {
        return mapped_::ObjectCreator<T>::allocate_and_construct(
            0, this, std::forward<CtrArgs>(args)...
        );
    }

    template <typename T, typename... CtrArgs>
    MMA1_NODISCARD PtrT<T> allocate_tagged(size_t tag_size, CtrArgs&&... args) noexcept {
        return mapped_::ObjectCreator<T>::allocate_and_construct(
            tag_size, this, std::forward<CtrArgs>(args)...
        );
    }



    template <typename T, typename... CtrArgs>
    void construct(PtrT<T> ptr, CtrArgs&&... args) noexcept
    {
        (void)new (mutable_data() + ptr.get()) T(std::forward<CtrArgs>(args)...);
    }

    bool is_null(const void* ptr) const {
        return data_ == ptr;
    }

    size_t data_size() const {
        return MMA1_UNLIKELY(arena_ != nullptr) ? arena_->size() : size_;
    }

    Span<const AtomType> span() const {
        return Span<const AtomType>(data_, data_size());
    }

    Span<AtomType> span() {
        return Span<AtomType>(data_, data_size());
    }
};



template <typename T, typename HeaderT, typename PtrHolderT, typename... CtrArgs>
typename LinkedArenaView<HeaderT, PtrHolderT>::template PtrT<T> allocate(LinkedArenaView<HeaderT, PtrHolderT>* arena, CtrArgs&&... args) {
    return arena->template allocate<T>(std::forward<CtrArgs>(args)...);
}

template <typename T, typename HeaderT, typename PtrHolderT, typename... CtrArgs>
typename LinkedArenaView<HeaderT, PtrHolderT>::template PtrT<T> allocate_tagged(size_t tag_size, LinkedArenaView<HeaderT, PtrHolderT>* arena, CtrArgs&&... args) {
    return arena->template allocate_tagged<T>(tag_size, std::forward<CtrArgs>(args)...);
}






template <typename HeaderT, typename PtrHolderT>
class LinkedArena {
    using ArenaBufferT = ArenaBuffer<uint8_t>;
    using ArenaView = LinkedArenaView<HeaderT, PtrHolderT>;

    ArenaView* arena_view_;
    ArenaBufferT arena_;

public:

    LinkedArena(size_t initial_arena_size, ArenaView* arena_view):
        arena_view_(arena_view),
        arena_(initial_arena_size, make_memory_mgr())
    {
        arena_view_->arena_ = this;

        arena_.ensure(sizeof(HeaderT));
        arena_.add_size(sizeof(HeaderT));

        (void)new (arena_.data()) HeaderT{};
    }


    LinkedArena(ArenaView* arena_view, const LinkedArena& other):
        arena_view_(arena_view),
        arena_(other.arena_, make_memory_mgr())
    {
        arena_view_->arena_ = this;
    }

    LinkedArena(ArenaView* arena_view, LinkedArena&& other):
        arena_view_(arena_view),
        arena_(std::move(other.arena_), make_memory_mgr())
    {
        arena_view_->arena_ = this;
        arena_view_->data_ = arena_.data();
    }

    void move_data_from(LinkedArena&& other)
    {
        arena_.move_data_from(std::move(other.arena_));
        arena_view_->data_ = arena_.data();
    }

    ArenaView* view() {
        return arena_view_;
    }

    const ArenaView* view() const {
        return arena_view_;
    }

    HeaderT& header() {
        return *T2T<HeaderT*>(arena_.data());
    }

    const HeaderT& header() const {
        return *T2T<HeaderT*>(arena_.data());
    }


    void reset_view() noexcept {
        arena_view_->data_ = nullptr;
    }

    void clear(size_t extra_size = 0) noexcept
    {
        HeaderT header = this->header();
        arena_.clear();

        arena_.ensure(sizeof(HeaderT) + extra_size);
        arena_.add_size(sizeof(HeaderT));

        *T2T<HeaderT*>(arena_.data()) = header;
    }

    void reset(size_t extra_size = 0) noexcept
    {
        HeaderT header = this->header();
        arena_.reset();

        arena_.ensure(sizeof(HeaderT) + extra_size);
        arena_.add_size(sizeof(HeaderT));

        *T2T<HeaderT*>(arena_.data()) = header;
    }


    size_t allocate_space(size_t size, size_t alignment, size_t tag_size)
    {
        size_t addr   = align_up(arena_.size(), alignment);
        size_t upsize = addr - arena_.size();

        while (upsize < tag_size) {
            addr += alignment;
            upsize += alignment;
        }

        arena_.ensure(upsize + size);
        arena_.add_size(upsize + size);

        return addr;
    }

    size_t size() const
    {
        return arena_.size();
    }

private:
    static constexpr size_t align_up(size_t value, size_t alignment) noexcept
    {
        size_t mask = alignment - 1;
        size_t res = value & mask;

        if (MMA1_UNLIKELY(res)) {
            return (value | mask) + 1;
        }

        return value;
    }

    auto make_memory_mgr() {
        return [&, this](int32_t buffer_id, ArenaBufferCmd cmd, size_t size, uint8_t* mem) -> uint8_t* {
            if (ArenaBufferCmd::ALLOCATE == cmd)
            {
                uint8_t* addr = allocate_system<uint8_t>(size).release();
                this->arena_view_->data_ = addr;
                return addr;
            }
            else if (ArenaBufferCmd::FREE == cmd) {
                free_system(mem);
            }

            return nullptr;
        };
    }
};


template <typename HeaderT, typename PtrHolderT>
MMA1_NODISCARD inline size_t LinkedArenaView<HeaderT, PtrHolderT>::allocate_space(size_t size, size_t alignment, size_t tag_size)
{
    if (arena_) {
        return arena_->allocate_space(size, alignment, tag_size);
    }
    else {
        MMA1_THROW(RuntimeException()) << WhatCInfo("Data is immutable");
    }
}


}}
