
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
        buffer_(allocate_system<ValueT>(capacity)),
        capaicty_(capacity)
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

    const ValueT& tail() const {
        return *buffer_.get();
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

    void ensure(size_t size)
    {
        if (size_ + size > capaicty_)
        {
            enlarge(size);
        }
    }


    void enlarge(size_t requested)
    {
        size_t capaicty = capaicty_;

        while (capaicty + requested < capaicty * 2)
        {
            capaicty *= 2;
        }

        auto new_ptr = allocate_system<ValueT>(capaicty);

        MemCpyBuffer(buffer_.get(), new_ptr.get(), size_);
        buffer_ = std::move(new_ptr);
        capaicty_ = capaicty;
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
};


}}}
