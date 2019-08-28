
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

#include <memoria/v1/core/tools/span.hpp>

#include <memoria/v1/core/tools/ptr_cast.hpp>

#include <type_traits>
#include <functional>


namespace memoria {
namespace v1 {

enum class ArenaBufferCmd: int32_t {
    ALLOCATE, FREE
};

// memaddr(buffer_id, command, size, existing_memaddr)
using ArenaBufferMemoryMgr = std::function<uint8_t* (int32_t, ArenaBufferCmd, size_t, uint8_t*)>;

template <typename TT>
class ArenaBuffer {

protected:    
    using ValueT = TT;
    static_assert(std::is_trivially_copyable<ValueT>::value, "ArenaBufferBase supports only trivially copyable types");

    ValueT* buffer_;
    size_t capacity_;
    size_t size_;

    ArenaBufferMemoryMgr memory_mgr_;
    int32_t buffer_id_{-1};

public:
    ArenaBuffer(size_t capacity):
        buffer_(capacity > 0 ? allocate_system<ValueT>(capacity).release() : nullptr),
        capacity_(capacity),
        size_(0)
    {}

    ArenaBuffer(size_t capacity, ArenaBufferMemoryMgr memory_mgr):
        capacity_(capacity),
        size_(0),
        memory_mgr_(std::move(memory_mgr))
    {
        buffer_ = allocate_buffer(capacity);
    }

    ArenaBuffer(uint8_t* provided_buffer, size_t capacity, ArenaBufferMemoryMgr memory_mgr):
        buffer_(tools::ptr_cast<ValueT>(provided_buffer)),
        capacity_(capacity),
        size_(0),
        memory_mgr_(std::move(memory_mgr))
    {}

    ArenaBuffer(ArenaBufferMemoryMgr memory_mgr):
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

    ArenaBuffer(ArenaBuffer&& other):
        buffer_(other.buffer_),
        capacity_(other.capacity_),
        size_(other.size_),
        memory_mgr_(std::move(other.memory_mgr_))
    {
        other.buffer_ = nullptr;
    }

    ArenaBuffer(): ArenaBuffer(0){}

    ~ArenaBuffer() noexcept
    {
        if (buffer_) {
            free_buffer(buffer_);
        }
    }

    bool operator==(const ArenaBuffer&& other) const {
        return buffer_ == other.buffer_;
    }

    ArenaBuffer& operator=(const ArenaBuffer&& other)
    {
        if (&other != this)
        {
            free_buffer(buffer_);

            capacity_ = other.capacity_;
            size_ = other.size_;

            memory_mgr_ = other.memory_mgr_;

            buffer_ = allocate_buffer(size_);

            MemCpyBuffer(other.buffer_, buffer_, size_);
        }

        return *this;
    }

    ArenaBuffer& operator=(ArenaBuffer&& other)
    {
        if (&other != this)
        {
            free_buffer(buffer_);

            capacity_ = other.capacity_;
            size_ = other.size_;

            memory_mgr_ = std::move(other.memory_mgr_);

            buffer_ = other.buffer_;

            other.buffer_ = nullptr;
        }

        return *this;
    }

    int32_t buffer_id() const {return buffer_id_;}

    void set_buffer_id(int32_t buffer_id) {
        buffer_id_ = buffer_id;
    }

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return capacity_;
    }

    size_t remaining() const {
        return capacity_ - size_;
    }

    ValueT& tail() {
        return *buffer_;
    }

    ValueT* tail_ptr() {
        return buffer_;
    }

    const ValueT& tail() const {
        return *buffer_;
    }

    const ValueT* tail_ptr() const {
        return buffer_;
    }

    const ValueT* data() const {
        return buffer_;
    }

    ValueT* data() {
        return buffer_;
    }

    ValueT& head() {
        return *(buffer_ + size_ - 1);
    }

    const ValueT& head() const {
        return *(buffer_ + size_ - 1);
    }


    void append_value(const ValueT& value)
    {
        ensure(1);
        *(buffer_ + size_) = value;
        size_++;
    }

    void append_values(const ValueT* values, size_t size)
    {
        ensure(size);
        MemCpyBuffer(values, buffer_ + size_, size);
        size_ += size;
    }

    void append_values(Span<const ValueT> values)
    {
        size_t size = values.size();
        ensure(size);
        MemCpyBuffer(values.data(), buffer_ + size_, size);
        size_ += size;
    }

    void ensure(size_t size)
    {
        if (size_ + size > capacity_)
        {
            enlarge(size);
        }
    }


    void enlarge(size_t requested)
    {
        size_t next_capaicty = capacity_ * 2;
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

        if (buffer_) {
            free_buffer(buffer_);
        }

        buffer_ = new_ptr;
        capacity_ = next_capaicty;
    }

    ValueT& access(size_t idx) {
        return *(buffer_ + idx);
    }

    const ValueT& access(size_t idx) const {
        return *(buffer_ + idx);
    }

    void clear() {
        size_ = 0;
    }

    void reset()
    {
        size_ = 0;
        capacity_ = 64;
        buffer_ = allocate_buffer(capacity_);
    }

    Span<ValueT> span() {
        return Span<ValueT>(buffer_, size_);
    }

    Span<const ValueT> span() const {
        return Span<ValueT>(buffer_, size_);
    }

    Span<ValueT> span(size_t from) {
        return Span<ValueT>(buffer_ + from, size_ - from);
    }

    Span<const ValueT> span(size_t from) const
    {
        return Span<ValueT>(buffer_ + from, size_ - from);
    }

    Span<ValueT> span(size_t from, size_t length)
    {
        return Span<ValueT>(buffer_ + from, length);
    }

    Span<const ValueT> span(size_t from, size_t length) const
    {
        return Span<ValueT>(buffer_ + from, length);
    }

    TT& operator[](size_t idx) {return access(idx);}
    const TT& operator[](size_t idx) const {return access(idx);}

    void emplace_back(const TT& tt) {
        append_value(tt);
    }

    void emplace_back(TT&& tt) {
        append_value(tt);
    }

private:

    ValueT* allocate_buffer(size_t size) const
    {
        if (MMA1_LIKELY(!memory_mgr_))
        {
            return allocate_system<ValueT>(size).release();
        }
        else {
            return tools::ptr_cast<ValueT>(memory_mgr_(buffer_id_, ArenaBufferCmd::ALLOCATE, size * sizeof(ValueT), nullptr));
        }
    }


    void free_buffer(ValueT* existing) noexcept
    {
        if (MMA1_LIKELY(!memory_mgr_))
        {
            free_system(existing);
        }
        else {
            memory_mgr_(buffer_id_, ArenaBufferCmd::FREE, capacity_ * sizeof(ValueT), tools::ptr_cast<uint8_t>(existing));
        }
    }
};



}}
