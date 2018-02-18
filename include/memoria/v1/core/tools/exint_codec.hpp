
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

/**
 * Partial implementation of EXINT universal code by Dustin Juliano
 * http://julianoresearch.com/publications/standards/exint/
 */

#include <memoria/v1/core/config.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {

template <typename V>
size_t GetExintValueLength(V value)
{
    //MEMORIA_V1_ASSERT(value, >=, 0);

    size_t length;

    for (length = 1; value > 0; value >>= 8)
    {
        length++;
    }

    return length;
}


template <typename T, typename V>
size_t EncodeExint(T* buffer, V value, size_t start)
{
    //MEMORIA_V1_ASSERT(value, >=, 0);

    uint8_t& byte_length = buffer[start];

    byte_length = 0;

    for (size_t idx = start + 1; value > 0; value >>= 8, idx++)
    {
        byte_length++;

        buffer[idx] = value & 0xFF;
    }

    return byte_length + 1;
}

template <typename T, typename V>
size_t DecodeExint(const T* buffer, V& value, size_t start)
{
    size_t byte_length = buffer[start];

    value = 0;

    for (size_t idx = start + 1, shift = 0; shift < byte_length * 8; idx++, shift += 8)
    {
        value |= ((V)buffer[idx]) << shift;
    }

    return byte_length + 1;
}

template <typename T, typename V>
struct ExintCodec {

    typedef T BufferType;
    static const int32_t BitsPerOffset  = 4;
    static const int32_t ElementSize    = 8; // In bits;

    const void* addr(const T* buffer, size_t pos) const {
        return &buffer[pos / sizeof(T)];
    }

    size_t length(const T* buffer, size_t idx, size_t limit) const {
        return buffer[idx] + 1;
    }

    size_t length(V value) const {
        return GetExintValueLength(value);
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return DecodeExint(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        return DecodeExint(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx, size_t limit) const
    {
        return EncodeExint(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx) const
    {
        return EncodeExint(buffer, value, idx);
    }

    void move(T* buffer, size_t from, size_t to, size_t size) const
    {
        CopyBuffer(buffer + from, buffer + to, size);
    }

    void copy(const T* src, size_t from, T* tgt, size_t to, size_t size) const
    {
        CopyBuffer(src + from, tgt + to, size);
    }
};

template <typename Value>
using UByteExintCodec = ExintCodec<uint8_t, Value>;

}}
