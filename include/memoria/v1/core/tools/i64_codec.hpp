
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/config.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/container/macros.hpp>

namespace memoria {
namespace v1 {

template <typename V>
size_t
GetI64ValueLength(V value)
{
    if (value == 0) {
        return 1;
    }
    else if (value < 64)
    {
        return 8;
    }
    else {
        return 8 + Log2(value);
    }
}

template <typename T>
size_t GetI64ValueLength(const T* buffer, size_t start)
{
    T length = GetBits(buffer, start, 8);

    if ((length & 0x1) == 0)
    {
        return 1;
    }
    else if ((length & 0x2) == 0)
    {
        return 8;
    }
    else
    {
        return (length >> 2) + 8;
    }
}


template <typename T, typename V>
size_t EncodeI64(T* buffer, V value, size_t start)
{
    if (value == 0)
    {
        SetBit(buffer, start, 0);
        return 1;
    }
    else if (value < 64)
    {
        V v = (value << 2) + 1;

        SetBits(buffer, start, v, 8);
        return 8;
    }
    else
    {
        V len   = Log2(value);
        V v     = (len << 2) + 3;

        SetBits(buffer, start, v, 8);
        SetBits(buffer, start + 8, value, len);

        return 8 + len;
    }
}

template <typename T, typename V>
size_t DecodeI64(const T* buffer, V& value, size_t start)
{
    T length = GetBits(buffer, start, 8);

    if ((length & 0x1) == 0)
    {
        value = 0;
        return 1;
    }
    else if ((length & 0x2) == 0)
    {
        value = length >> 2;
        return 8;
    }
    else
    {
        V len = length >> 2;
        value = GetBits(buffer, start + 8, len);
        return len + 8;
    }
}

template <typename T, typename V>
struct I64Codec {

    typedef T BufferType;
    static const int32_t BitsPerOffset  = 8;
    static const int32_t ElementSize    = 1; // In bits;

    size_t length(const T* buffer, size_t idx, size_t limit) const {
        return GetI64ValueLength(buffer, idx);
    }

    size_t length(V value) const {
        return GetI64ValueLength(value);
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return DecodeI64(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        return DecodeI64(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx, size_t limit) const
    {
        return EncodeI64(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx) const
    {
        return EncodeI64(buffer, value, idx);
    }

    void move(T* buffer, size_t from, size_t to, size_t size) const
    {
        MoveBits(buffer, buffer, from, to, size);
    }

    void copy(const T* src, size_t from, T* tgt, size_t to, size_t size) const
    {
        MoveBits(src, tgt, from, to, size);
    }
};

template <typename Value>
using UInt64I64Codec = I64Codec<uint64_t, Value>;

}}
