
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

#include <memoria/core/types.hpp>
#include <memoria/core/memory/malloc.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/lifetime_guard.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <type_traits>
#include <functional>
#include <algorithm>


namespace memoria {

template <typename TT, typename SizeT = size_t>
class ArenaBuffer;

enum class ArenaBufferCmd: int32_t {
    ALLOCATE, FREE
};

// memaddr(buffer_id, command, size, existing_memaddr)
using ArenaBufferMemoryMgr = std::function<uint8_t* (int32_t, ArenaBufferCmd, size_t, uint8_t*)>;



template <typename TT, typename SizeT>
class ArenaBuffer {

protected:
    using MyType = ArenaBuffer;
    using ValueT = TT;
    static_assert(std::is_trivially_copyable<ValueT>::value, "ArenaBufferBase currently supports only trivially copyable types");

    using InvalidationListener = LifetimeInvalidationListener;

    ValueT* buffer_;
    SizeT capacity_;
    SizeT size_;

    ArenaBufferMemoryMgr memory_mgr_;
    int32_t buffer_id_{-1};

    class ArenaBufferSharedGuardImpl: public detail::LifetimeGuardShared {
        const MyType* arena_;
    public:
        ArenaBufferSharedGuardImpl(const MyType* arena) noexcept:
            arena_(arena)
        {}

        void destroy() noexcept {
            if (valid_) {
                arena_->guard_ = nullptr;
            }

            delete this;
        }

        void invalidate() noexcept {
            valid_ = false;
        }
    };

    mutable ArenaBufferSharedGuardImpl* guard_{nullptr};
    mutable InvalidationListener invalidation_listener_;

    friend class ArenaBufferSharedGuardImpl;

public:
    ArenaBuffer(SizeT capacity):
        buffer_(capacity > 0 ? allocate_system<ValueT>(capacity).release() : nullptr),
        capacity_(capacity),
        size_(0)
    {}

    ArenaBuffer(SizeT capacity, ArenaBufferMemoryMgr memory_mgr):
        capacity_(capacity),
        size_(0),
        memory_mgr_(std::move(memory_mgr))
    {
        buffer_ = allocate_buffer(capacity);
    }

    ArenaBuffer(uint8_t* provided_buffer, SizeT capacity, ArenaBufferMemoryMgr memory_mgr):
        buffer_(ptr_cast<ValueT>(provided_buffer)),
        capacity_(capacity),
        size_(0),
        memory_mgr_(std::move(memory_mgr))
    {}

    ArenaBuffer(ArenaBufferMemoryMgr memory_mgr) noexcept:
        buffer_(nullptr),
        capacity_(0),
        size_(0),
        memory_mgr_(std::move(memory_mgr))
    {}

    ArenaBuffer(const ArenaBuffer& other):
        capacity_(other.capacity_),
        size_(other.size_),
        memory_mgr_(other.memory_mgr_)
    {
        if (other.buffer_)
        {
            buffer_ = allocate_buffer(size_);
            MemCpyBuffer(other.buffer_, buffer_, size_);
        }
        else {
            buffer_ = nullptr;
        }
    }

    ArenaBuffer(const ArenaBuffer& other, ArenaBufferMemoryMgr memory_mgr):
        capacity_(other.capacity_),
        size_(other.size_),
        memory_mgr_(memory_mgr)
    {
        if (other.buffer_)
        {
            buffer_ = allocate_buffer(size_);
            MemCpyBuffer(other.buffer_, buffer_, size_);
        }
        else {
            buffer_ = nullptr;
        }
    }

    ArenaBuffer(ArenaBuffer&& other) noexcept:
        buffer_(other.buffer_),
        capacity_(other.capacity_),
        size_(other.size_),
        memory_mgr_(std::move(other.memory_mgr_))
    {
        other.buffer_ = nullptr;
    }

    ArenaBuffer(ArenaBuffer&& other, ArenaBufferMemoryMgr memory_mgr) noexcept:
        buffer_(other.buffer_),
        capacity_(other.capacity_),
        size_(other.size_),
        memory_mgr_(memory_mgr)
    {
        other.buffer_ = nullptr;
        other.memory_mgr_ = nullptr;
    }

    ArenaBuffer() noexcept : ArenaBuffer(0) {}

    ~ArenaBuffer() noexcept
    {
        invalidate_guard();

        if (buffer_) {
            free_buffer(buffer_);
        }
    }

    bool operator==(const ArenaBuffer&& other) const noexcept {
        return buffer_ == other.buffer_;
    }

    ArenaBuffer& operator=(const ArenaBuffer&& other) noexcept
    {
        if (&other != this)
        {
            invalidate_guard();
            free_buffer(buffer_);

            capacity_ = other.capacity_;
            size_ = other.size_;

            memory_mgr_ = other.memory_mgr_;

            buffer_ = allocate_buffer(size_);

            MemCpyBuffer(other.buffer_, buffer_, size_);
        }

        return *this;
    }

    ArenaBuffer& operator=(ArenaBuffer&& other) noexcept
    {
        if (&other != this)
        {
            invalidate_guard();
            free_buffer(buffer_);

            capacity_ = other.capacity_;
            size_ = other.size_;

            memory_mgr_ = std::move(other.memory_mgr_);

            buffer_ = other.buffer_;

            other.invalidate_guard();
            other.buffer_ = nullptr;
        }

        return *this;
    }

    // NOTE: other's memory must be compatible with
    // own's memory manager!

    void move_data_from(ArenaBuffer&& other) noexcept
    {
        if (this != &other)
        {
            reset();

            if (buffer_) {
                free_buffer(buffer_);
            }

            buffer_     = other.buffer_;
            size_       = other.size_;
            capacity_   = other.capacity_;

            other.invalidate_guard();
            other.buffer_ = nullptr;
        }
    }

    int32_t buffer_id() const noexcept {return buffer_id_;}

    void set_buffer_id(int32_t buffer_id) noexcept {
        buffer_id_ = buffer_id;
    }

    SizeT size() const noexcept {
        return size_;
    }

    SizeT capacity() const noexcept {
        return capacity_;
    }

    SizeT remaining() const noexcept {
        return capacity_ - size_;
    }

    ValueT& tail() noexcept {
        return *buffer_;
    }

    ValueT* tail_ptr() noexcept {
        return buffer_;
    }

    const ValueT& tail() const noexcept {
        return *buffer_;
    }

    const ValueT* tail_ptr() const noexcept {
        return buffer_;
    }

    const ValueT* data() const noexcept {
        return buffer_;
    }

    ValueT* data() noexcept {
        return buffer_;
    }

    ValueT& head() noexcept {
        return *(buffer_ + size_ - 1);
    }

    const ValueT& head() const noexcept {
        return *(buffer_ + size_ - 1);
    }

    const ValueT* top() const noexcept {
        return buffer_ + size_;
    }

    ValueT* top() noexcept {
        return buffer_ + size_;
    }

    bool append_value(const ValueT& value) noexcept
    {
        bool resized = ensure(1);
        *(buffer_ + size_) = value;
        size_++;
        return resized;
    }

    bool append_values(const ValueT* values, SizeT size) noexcept
    {
        bool resized = ensure(size);
        MemMoveBuffer(values, buffer_ + size_, size);
        size_ += size;
        return resized;
    }

    bool append_values(Span<const ValueT> values) noexcept
    {
        SizeT size = values.size();
        bool resized = ensure(size);
        MemMoveBuffer(values.data(), buffer_ + size_, size);
        size_ += size;
        return resized;
    }

    bool ensure(SizeT size) noexcept
    {
        if (size_ + size > capacity_)
        {
            enlarge(size_ + size - capacity_);
            return true;
        }

        return false;
    }


    void enlarge(SizeT requested) noexcept
    {
        SizeT next_capaicty = capacity_ * 2;
        if (next_capaicty == 0) next_capaicty = 1;

        while (capacity_ + requested > next_capaicty)
        {
            next_capaicty *= 2;
        }

        ValueT* new_ptr = allocate_buffer(next_capaicty);

        if (size_ > 0)
        {
            MemCpyBuffer(buffer_, new_ptr, size_);
        }

        invalidate_guard();

        if (buffer_) {
            free_buffer(buffer_);
        }

        buffer_ = new_ptr;
        capacity_ = next_capaicty;
    }

    ValueT& access(SizeT idx) noexcept {
        return *(buffer_ + idx);
    }

    const ValueT& access(SizeT idx) const noexcept {
        return *(buffer_ + idx);
    }

    void clear() noexcept {
        invalidate_guard();
        size_ = 0;
    }


    void reset(size_t capacity = 0)
    {
        invalidate_guard();

        if (buffer_) {
            free_buffer(buffer_);
        }

        size_ = 0;
        capacity_ = capacity;

        buffer_ = capacity_ > 0 ? allocate_buffer(capacity_) : 0;
    }

    void remove(size_t from, size_t to) noexcept
    {
        //std::cout << "ArenaBuffer: invalid range: " << from << " :: " << to << " :: " << size_ << std::endl;

        //if (to > from)
        {
            invalidate_guard();

            if (to < from || from >= size_ || to >= size_) {
                std::cout << "ArenaBuffer: invalid range: " << from << " :: " << to << " :: " << size_ << std::endl;
            }

            MemMoveBuffer(buffer_ + to, buffer_ + from, size_ - to);
            size_ -= to - from;
        }
    }

    GuardedView<ValueT> get_guarded(size_t idx) noexcept {
        if (!guard_) {
            guard_ = new ArenaBufferSharedGuardImpl(this);
        }
        return GuardedView<ValueT>(buffer_[idx], LifetimeGuard(guard_));
    }

    GuardedView<const ValueT> get_guarded(size_t idx) const noexcept {
        if (!guard_) {
            guard_ = new ArenaBufferSharedGuardImpl(this);
        }
        return GuardedView<const ValueT>(buffer_[idx], LifetimeGuard(guard_));
    }

    LifetimeGuard guard() const noexcept {
        if (!guard_) {
            guard_ = new ArenaBufferSharedGuardImpl(this);
        }

        return LifetimeGuard(guard_);
    }

    void invalidate_guard() const noexcept {
        if (guard_) {
            guard_->invalidate();
            guard_ = nullptr;
        }

        if (invalidation_listener_) {
            invalidation_listener_();
        }
    }

    void set_invalidation_listener(InvalidationListener listener) const noexcept {
        invalidation_listener_ = listener;
    }


    Span<ValueT> span() noexcept {
        return Span<ValueT>(buffer_, size_);
    }

    Span<const ValueT> span() const noexcept {
        return Span<ValueT>(buffer_, size_);
    }

    Span<ValueT> span(SizeT from) noexcept {
        return Span<ValueT>(buffer_ + from, size_ - from);
    }

    Span<const ValueT> span(SizeT from) const noexcept
    {
        return Span<ValueT>(buffer_ + from, size_ - from);
    }

    Span<ValueT> span(SizeT from, SizeT length) noexcept
    {
        return Span<ValueT>(buffer_ + from, length);
    }

    Span<const ValueT> span(SizeT from, SizeT length) const noexcept
    {
        return Span<ValueT>(buffer_ + from, length);
    }

    TT& operator[](SizeT idx) noexcept {return access(idx);}
    const TT& operator[](SizeT idx) const noexcept {return access(idx);}

    bool emplace_back(const TT& tt) noexcept {
        return append_value(tt);
    }

    bool emplace_back(TT&& tt) noexcept {
        return append_value(tt);
    }

    void add_size(SizeT size) noexcept {
        size_ += size;
    }

    void sort() noexcept
    {
        if (buffer_)
        {
            invalidate_guard();
            std::sort(buffer_, buffer_ + size());
        }
    }

private:

    ValueT* allocate_buffer(SizeT size) const
    {
        if (MMA_LIKELY(!memory_mgr_))
        {
            return allocate_system<ValueT>(size).release();
        }
        else {
            return ptr_cast<ValueT>(memory_mgr_(buffer_id_, ArenaBufferCmd::ALLOCATE, size * sizeof(ValueT), nullptr));
        }
    }


    void free_buffer(ValueT* existing) noexcept
    {
        if (MMA_LIKELY(!memory_mgr_))
        {
            free_system(existing);
        }
        else {
            memory_mgr_(buffer_id_, ArenaBufferCmd::FREE, capacity_ * sizeof(ValueT), ptr_cast<uint8_t>(existing));
        }
    }
};



}
