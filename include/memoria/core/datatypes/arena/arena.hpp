
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

#include <memoria/core/tools/result.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/memory/malloc.hpp>

#include <memoria/core/linked/common/arena.hpp>

#include <memoria/core/datatypes/datatype_ptrs.hpp>

#include <new>
#include <memory>

namespace memoria {
namespace arena {


template <typename T>
struct ArenaTypeTag;
using TypeTag = uint64_t;
static constexpr uint8_t SHORT_TYPETAG_LENGTH_BASE = 249;

constexpr size_t type_tag_size(TypeTag tag) {
    return tag < SHORT_TYPETAG_LENGTH_BASE ? 1 : sizeof(tag);
}


namespace detail_ {
    template <typename, typename = VoidT<>>
    struct HasObjectSizeHelper: std::false_type {};

    template <typename T>
    struct HasObjectSizeHelper<T, VoidT<decltype(T::UseObjectSize)>>: std::true_type {};
}


template <typename T>
constexpr bool HasObjectSize = detail_::HasObjectSizeHelper<T>::type::value;

template <typename T, bool HasObjectSize = HasObjectSize<T>>
struct ObjectSpaceAllocator;

template <typename T>
struct ObjectSpaceAllocator<T, false> {
    template <typename Arena, typename... CtrArgs>
    static auto allocate_space(size_t tag_size, Arena* arena, CtrArgs&&... args)
    {
        size_t size = sizeof(T);
        auto addr = arena->template allocate_space<T>(size, tag_size);
        //arena->construct(addr, std::forward<CtrArgs>(args)...);
        return addr;
    }
};

template <typename T>
struct ObjectSpaceAllocator<T, true> {
    template <typename Arena, typename... CtrArgs>
    static auto allocate_space(size_t tag_size, Arena* arena, CtrArgs&&... args)
    {
        size_t size = T::object_size(std::forward<CtrArgs>(args)...);
        auto addr = arena->template allocate_space<T>(size, tag_size);
        //arena->construct(addr, std::forward<CtrArgs>(args)...);
        return addr;
    }
};


class MemorySegment {
protected:
    const uint8_t* buffer_;
    size_t size_;
    DTViewHolder* view_holder_;

public:
    MemorySegment(const uint8_t* buffer, size_t size, DTViewHolder* view_holder) noexcept :
        buffer_(buffer), size_(size), view_holder_(view_holder)
    {}

    const uint8_t* buffer() const noexcept {return buffer_;}
    size_t size() const noexcept {return size_;}

    bool is_null(void* ptr) const {
        return buffer_ == ptr;
    }

    DTViewHolder* view_holder() const noexcept {return view_holder_;}

    TypeTag read_type_tag(size_t offset) const {
        const uint8_t* buf = buffer_ + offset - 1;

        uint8_t short_value = *buf;
        if (MMA_LIKELY(short_value < SHORT_TYPETAG_LENGTH_BASE)) {
            return short_value;
        }
        else {
            TypeTag long_value{};
            buf--;

            for (size_t c = 0; c < short_value - SHORT_TYPETAG_LENGTH_BASE; c++) {
                long_value |= static_cast<TypeTag>(*(buf - c)) << c;
            }

            return long_value ;
        }
    }
};

template <typename T> class SegmentPtr;

class ArenaSegment: public MemorySegment {
protected:
    uint8_t* buffer_mut_;
    size_t capacity_;
    size_t enlarges_;
public:
    ArenaSegment(uint8_t* buffer, size_t size, size_t capacity, DTViewHolder* view_holder) noexcept :
        MemorySegment(buffer, size, view_holder),
        buffer_mut_(buffer),
        capacity_(capacity),
        enlarges_()
    {}

    virtual ~ArenaSegment() noexcept = default;

    class State {
        ArenaSegment* arena_;
        size_t enlarges_;
    public:
        State(ArenaSegment* arena) noexcept :
            arena_(arena), enlarges_(arena_->enlarges())
        {}

        State(): arena_(), enlarges_() {}

        template <typename Fn>
        void on_enlarge(Fn&& fn) {
            if (MMA_UNLIKELY(arena_ && arena_->enlarges() > enlarges_)) {
                fn();
            }
        }

        operator bool() const {
            return arena_ && arena_->enlarges() > enlarges_;
        }
    };

    State state() noexcept {
        return State(this);
    }

    MMA_NODISCARD size_t enlarges() const noexcept {
        return enlarges_;
    }

    MMA_NODISCARD bool is_enlarged(size_t prev_enlarges) const noexcept {
        return enlarges_ > prev_enlarges;
    }

    MMA_NODISCARD uint8_t* buffer_mut() noexcept {return buffer_mut_;}
    MMA_NODISCARD const uint8_t* buffer_mut() const noexcept {return buffer_mut_;}

    size_t capacity() const {return capacity_;}

    MMA_NODISCARD virtual size_t allocate_space(size_t size, size_t alignment, size_t tag_size) = 0;

    template <typename T>
    MMA_NODISCARD SegmentPtr<T> allocate_space(size_t size, size_t tag_size = 0);

    template <typename T, typename... Args>
    MMA_NODISCARD SegmentPtr<T> allocate(Args&&... args);

    template <typename T, typename... Args>
    MMA_NODISCARD SegmentPtr<T> allocate_untagged(Args&&... args);

    template <typename T, typename... Args>
    void construct(const SegmentPtr<T>& ptr, Args&&... args);

    void write_type_tag(size_t offset, TypeTag tag)
    {
        uint8_t* buf = buffer_mut_ + offset - 1;

        if (MMA_LIKELY(tag < SHORT_TYPETAG_LENGTH_BASE)) {
            *buf = static_cast<uint8_t>(tag);
        }
        else {
            // It's 7 bytes for now
            size_t tag_len = 7;
            *buf = SHORT_TYPETAG_LENGTH_BASE + tag_len;
            buf--;
            for (size_t c = 0; c < tag_len; c++) {
                *(buf - c) = static_cast<uint8_t>(tag >> c);
            }
        }
    }
};





class SegmentOffset {
protected:
    size_t offset_;
public:
    SegmentOffset() noexcept : offset_() {}

    size_t offset() const noexcept {return offset_;}
    bool is_null() const {return offset_ == 0;}
    bool is_not_null() const {return offset_ != 0;}
};

template <typename T>
class SegmentPtr: public SegmentOffset {

public:
    SegmentPtr() noexcept {}

    static SegmentPtr<T> from(size_t offset) {
        SegmentPtr<T> ss;
        ss.offset_ = offset;
        return ss;
    }

    static SegmentPtr<T> from(const MemorySegment* sgm, const T* ptr)
    {
        const uint8_t* addr = reinterpret_cast<const uint8_t*>(ptr);
        return SegmentPtr<T>::from(addr - sgm->buffer());
    }

    void reset() noexcept {
        offset_ = 0;
    }

    MMA_NODISCARD const T* read(const MemorySegment* sgm) const
    {
        if (offset_ && offset_ < sgm->size()) {
            return ptr_cast<const T>(sgm->buffer() + offset_);
        }
        else if (!offset_) {
            MEMORIA_MAKE_GENERIC_ERROR("SegmentPtr is null").do_throw();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("SegmentPtr read_idx get out of range: {} {}", offset_, sgm->size()).do_throw();
        }
    }


    MMA_NODISCARD const T* read(const MemorySegment* sgm, size_t idx) const
    {
        size_t idx8 = idx * sizeof (T);
        if (offset_ && offset_ + idx8 < sgm->size()) {
            return ptr_cast<const T>(sgm->buffer() + offset_ + idx8);
        }
        else if (!offset_) {
            MEMORIA_MAKE_GENERIC_ERROR("SegmentPtr is null").do_throw();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("SegmentPtr read_idx get out of range: {} {}", offset_ + idx8 , sgm->size()).do_throw();
        }
    }

    MMA_NODISCARD T* write(const ArenaSegment* sgm) const
    {
        if (offset_ && offset_ < sgm->size()) {
            return ptr_cast<T>(sgm->buffer() + offset_);
        }
        else if (!offset_) {
            MEMORIA_MAKE_GENERIC_ERROR("SegmentPtr is null").do_throw();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("SegmentPtr write get out of range: {} {}", offset_, sgm->size()).do_throw();
        }
    }

    MMA_NODISCARD T* write(const ArenaSegment* sgm, size_t idx) const
    {
        size_t idx8 = idx * sizeof(T);
        if (offset_ && offset_ + idx8 < sgm->size()) {
            return ptr_cast<T>(sgm->buffer() + offset_ + idx8);
        }
        else if (!offset_) {
            MEMORIA_MAKE_GENERIC_ERROR("SegmentPtr is null").do_throw();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("SegmentPtr write_idx get out of range: {} {}", offset_ + idx8, sgm->size()).do_throw();
        }
    }

    void set(const ArenaSegment* sgm, const T* ptr)
    {
        ptrdiff_t offset = static_cast<const uint8_t*>(ptr) - sgm->buffer_mut();
        if (offset >= 0 && offset < sgm->size()) {
            offset_ = offset;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("SegmentPtr set out of range: {} {}", offset, sgm->size()).do_throw();
        }
    }
};



template <typename T, typename... Args>
SegmentPtr<T> allocate(ArenaSegment* sgm, Args&&... args) {
    return sgm->template allocate<T>(std::forward<Args>(args)...);
}



template <typename T>
MMA_NODISCARD inline SegmentPtr<T> ArenaSegment::allocate_space(size_t size, size_t tag_size)
{
    size_t alignment = alignof(T);
    return SegmentPtr<T>::from(allocate_space(size, alignment, tag_size));
}


template <typename T, typename... Args>
inline SegmentPtr<T> ArenaSegment::allocate(Args&&... args)
{
    constexpr TypeTag tag = ArenaTypeTag<T>::Value;
    constexpr size_t tag_size = type_tag_size(tag);
    auto ptr = ObjectSpaceAllocator<T>::allocate_space(
        tag_size, this, std::forward<Args>(args)...
    );

    write_type_tag(ptr.offset(), tag);
    this->construct(ptr, std::forward<Args>(args)...);
    return ptr;
}

template <typename T, typename... Args>
MMA_NODISCARD SegmentPtr<T> ArenaSegment::allocate_untagged(Args&&... args) {
    auto ptr = ObjectSpaceAllocator<T>::allocate_space(
        0, this, std::forward<Args>(args)...
    );
    this->construct(ptr, std::forward<Args>(args)...);
    return ptr;
}

template <typename T, typename... Args>
inline void ArenaSegment::construct(const SegmentPtr<T>& ptr, Args&&... args)
{
    (void)new (buffer_mut_ + ptr.offset()) T(std::forward<Args>(args)...);
}

class ArenaSegmentImpl: public ArenaSegment {
    DTViewHolder view_holder_;
public:
    ArenaSegmentImpl() noexcept: ArenaSegment(nullptr, 0, 0, &view_holder_) {}

    ArenaSegmentImpl(const uint8_t* buffer, size_t size, size_t capacity):
        ArenaSegment(nullptr, size, capacity, &view_holder_)
    {
        if (buffer) {
            buffer_ = buffer_mut_ = allocate_system<uint8_t>(capacity).release();
            std::memmove(buffer_mut_, buffer, size);
            std::memset(buffer_mut_ + size_, 0, capacity - size);
        }
    }

    ArenaSegmentImpl(size_t initial_capacity):
        ArenaSegmentImpl(nullptr, 0, initial_capacity)
    {
        buffer_ = buffer_mut_ = allocate_system<uint8_t>(initial_capacity).release();
        std::memset(buffer_mut_, 0, initial_capacity);
    }

    ArenaSegmentImpl(const ArenaSegmentImpl& other):
        ArenaSegmentImpl(other.buffer_, other.size_, other.capacity_)
    {}

    ArenaSegmentImpl(ArenaSegmentImpl&&) = delete;

    virtual ~ArenaSegmentImpl() noexcept {
        free_system(buffer_mut_);
    }

    void ensure(size_t free_space)
    {
        if (size_ + free_space > capacity_) {
            enlarge(size_ + free_space);
        }
    }

    MMA_NODISCARD virtual size_t allocate_space(size_t size, size_t alignment, size_t tag_size)
    {
        size_t addr   = align_up(size_, alignment);
        size_t upsize = addr - size_;

        while (upsize < tag_size) {
            addr += alignment;
            upsize += alignment;
        }

        ensure(upsize + size);

        size_ += (size + upsize);

        //println("allocate_space: {}::{} -- {}::{}::{}", size_, capacity_, addr, size, addr + size);

        return addr;
    }

protected:
    void enlarge(size_t required)
    {
        size_t new_target = capacity_ > 0 ? capacity_ : 2;
        while (new_target < required) {
            new_target *= 2;
        }

        auto new_buffer = allocate_system<uint8_t>(new_target);

        std::memmove(new_buffer.get(), buffer_mut_, size_);
        std::memset(new_buffer.get() + size_, 0, new_target - size_);

        uint8_t* tmp = buffer_mut_;
        buffer_ = buffer_mut_ = new_buffer.release();
        capacity_ = new_target;
        enlarges_++;

        if (tmp) {
            free_system(tmp);
        }
    }

    static constexpr size_t align_up(size_t value, size_t alignment) noexcept
    {
        size_t mask = alignment - 1;
        size_t res = value & mask;

        if (MMA_UNLIKELY(res)) {
            return (value | mask) + 1;
        }

        return value;
    }
};


}}
