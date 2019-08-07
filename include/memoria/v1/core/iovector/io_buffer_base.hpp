
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

#include <type_traits>

namespace memoria {
namespace v1 {
namespace io {

template <typename TT>
class IOBufferBase {

protected:
    using ValueT = TT;
    static_assert(std::is_trivially_copyable<ValueT>::value, "IOBufferBase supports only trivially copyable types");

    UniquePtr<ValueT> buffer_;
    size_t capaicty_;
    size_t size_;

protected:
    IOBufferBase(size_t capacity):
        buffer_(capacity > 0 ? allocate_system<ValueT>(capacity) : UniquePtr<ValueT>(nullptr, ::free)),
        capaicty_(capacity),
        size_(0)
    {}

    IOBufferBase(): IOBufferBase(0)
    {}

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return capaicty_;
    }

    size_t remaining() const {
        return capaicty_ - size_;
    }

    ValueT& tail() {
        return *buffer_.get();
    }

    ValueT* tail_ptr() {
        return buffer_.get();
    }

    const ValueT& tail() const {
        return *buffer_.get();
    }

    const ValueT* tail_ptr() const {
        return buffer_.get();
    }

    ValueT& head() {
        return *(buffer_.get() + size_ - 1);
    }

    const ValueT& head() const {
        return *(buffer_.get() + size_ - 1);
    }


    void append_value(const ValueT& value)
    {
        ensure(1);
        *(buffer_.get() + size_) = value;
        size_++;
    }

    void append_values(const ValueT* values, size_t size)
    {
        ensure(size);
        MemCpyBuffer(values, buffer_.get() + size_, size);
        size_ += size;
    }

    void ensure(size_t size)
    {
        if (size_ + size > capaicty_)
        {
            enlarge(size);
        }
    }


    void enlarge(size_t requested)
    {
        size_t next_capaicty = capaicty_ * 2;
        if (next_capaicty == 0) next_capaicty = 1;

        while (capaicty_ + requested > next_capaicty)
        {
            next_capaicty *= 2;
        }

        auto new_ptr = allocate_system<ValueT>(next_capaicty);

        if (size_ > 0)
        {
            MemCpyBuffer(buffer_.get(), new_ptr.get(), size_);
        }

        buffer_ = std::move(new_ptr);
        capaicty_ = next_capaicty;
    }

    ValueT& access(size_t idx) {
        return *(buffer_.get() + idx);
    }

    const ValueT& access(size_t idx) const {
        return *(buffer_.get() + idx);
    }

    void clear() {
        size_ = 0;
    }

    void reset()
    {
        size_ = 0;
        capaicty_ = 64;
        buffer_ = allocate_system<ValueT>(capaicty_);
    }

    Span<ValueT> span() {
        return Span<ValueT>(buffer_.get(), size_);
    }

    Span<const ValueT> span() const {
        return Span<ValueT>(buffer_.get(), size_);
    }

    Span<ValueT> span(size_t from) {
        return Span<ValueT>(buffer_.get() + from, size_ - from);
    }

    Span<const ValueT> span(size_t from) const
    {
        return Span<ValueT>(buffer_.get() + from, size_ - from);
    }

    Span<ValueT> span(size_t from, size_t length)
    {
        return Span<ValueT>(buffer_.get() + from, length);
    }

    Span<const ValueT> span(size_t from, size_t length) const
    {
        return Span<ValueT>(buffer_.get() + from, length);
    }
};

template <typename TT>
class DefaultIOBuffer: public IOBufferBase<TT> {
    using Base = IOBufferBase<TT>;

public:
    DefaultIOBuffer(size_t capacity): Base(capacity) {}
    DefaultIOBuffer(): Base() {}

    using Base::append_value;
    using Base::append_values;
    using Base::access;
    using Base::size;
    using Base::clear;
    using Base::head;
    using Base::tail;
    using Base::tail_ptr;
    using Base::remaining;
    using Base::reset;
    using Base::span;
    using Base::ensure;

    TT& operator[](size_t idx) {return access(idx);}
    const TT& operator[](size_t idx) const {return access(idx);}

    void emplace_back(const TT& tt) {
        append_value(tt);
    }

    void emplace_back(TT&& tt) {
        append_value(tt);
    }
};

}}}
