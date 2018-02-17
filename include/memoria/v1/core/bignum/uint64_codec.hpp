
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {

template <typename> class ValueCodec;

template <>
class ValueCodec<uint64_t> {
public:
    using BufferType    = uint8_t;
    using T             = BufferType;
    using V             = uint64_t;

    using ValuePtr      = ValuePtrT1<BufferType>;

    static constexpr int32_t BitsPerOffset  = 4;
    static constexpr int32_t ElementSize    = 8; // In bits;

    static constexpr uint64_t UpperBound = 248;

    ValuePtr describe(const T* buffer, size_t idx)
    {
        return ValuePtr(buffer + idx, length(buffer, idx, -1ull));
    }

    size_t length(const T* buffer, size_t idx, size_t limit = -1) const
    {
        auto head = buffer[idx];
        if (head < UpperBound) {
            return 1;
        }
        else {
            return head - UpperBound + 1;
        }
    }

    size_t length(const V& value) const
    {
        if (value < UpperBound - 1)
        {
            return 1;
        }
        else {
            return 1 + byte_length(value);
        }
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return decode(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        auto header = buffer[idx];

        if (header < UpperBound)
        {
            value = header - 1;
            return 1;
        }
        else
        {
            value = 0;
            size_t len = buffer[idx] - UpperBound;

            deserialize(buffer, value, idx + 1, len);

            return len + 1;
        }
    }

    size_t encode(T* buffer, const ValuePtr& value, size_t idx) const
    {
        copy(value.addr(), 0, buffer, idx, value.length());
        return value.length();
    }

    size_t encode(T* buffer, const V& value, size_t idx, size_t limit) const
    {
        return encode(buffer, value, idx);
    }

    size_t encode(T* buffer, const V& value, size_t idx) const
    {
        if (value < UpperBound - 1)
        {
            buffer[idx] = value + 1;
            return 1;
        }
        else {
            size_t len = serialize(buffer, value, idx + 1);

            buffer[idx] = len + UpperBound;

            return 1 + len;
        }
    }

    void move(T* buffer, size_t from, size_t to, size_t size) const
    {
        CopyBuffer(buffer + from, buffer + to, size);
    }

    void copy(const T* src, size_t from, T* tgt, size_t to, size_t size) const
    {
        CopyBuffer(src + from, tgt + to, size);
    }

private:



    static size_t serialize(T* buffer, uint64_t value, size_t idx)
    {
        uint32_t len = bytes(value);

        for (uint32_t c = 0; c < len; c++)
        {
            buffer[idx++] = value >> (c << 3);
        }

        return len;
    }

    static void deserialize(const T* buffer, uint64_t& value, size_t idx, size_t len)
    {
        for (size_t c = 0; c < len; c++, idx++)
        {
            value |= ((uint64_t)buffer[idx]) << (c << 3);
        }
    }


    constexpr static uint32_t msb(uint64_t digits)
    {
        return 63 - __builtin_clzll(digits);
    }

    template <typename T>
    constexpr static uint32_t bytes(T digits)
    {
        uint32_t v = msb(digits) + 1;
        return (v >> 3) + ((v & 0x7) != 0);
    }

    constexpr static uint32_t byte_length(const uint64_t data)
    {
        return bytes(data);
    }
};

}}
