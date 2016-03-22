
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/bitmap.hpp>

namespace memoria {

template <typename V>
size_t GetI7ValueLength(V value)
{
    if (value < 128) {
        return 1;
    }
    else {
        size_t length;

        for (length = 1; value > 0; value >>= 8)
        {
            length++;
        }

        return length;
    }
}


template <typename T, typename V>
size_t EncodeI7(T* buffer, V value, size_t start)
{
    if (value < 128)
    {
        buffer[start] = value;
        return 1;
    }
    else {
        UByte& byte_length = buffer[start];

        byte_length = 0;

        for (size_t idx = start + 1; value > 0; value >>= 8, idx++)
        {
            byte_length++;

            buffer[idx] = value & 0xFF;
        }

        UByte len = byte_length;

        byte_length += 128;

        return len + 1;
    }
}

template <typename T, typename V>
size_t DecodeI7(const T* buffer, V& value, size_t start)
{
    if (buffer[start] < 128)
    {
        value = buffer[start];
        return 1;
    }
    else {
        size_t byte_length = buffer[start] - 128;

        value = 0;

        for (size_t idx = start + 1, shift = 0; shift < byte_length * 8; idx++, shift += 8)
        {
            value |= ((V)buffer[idx]) << shift;
        }

        return byte_length + 1;
    }
}

template <typename T, typename V>
struct I7Codec {

    typedef T BufferType;
    static const Int BitsPerOffset  = 4;
    static const Int ElementSize    = 8; // In bits;

    const void* addr(const T* buffer, size_t pos) const {
        return &buffer[pos / sizeof(T)];
    }

    size_t length(const T* buffer, size_t idx, size_t limit) const {
        return buffer[idx] < 128 ? 1 : buffer[idx] - 128 + 1;
    }

    size_t length(V value) const {
        return GetI7ValueLength(value);
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return DecodeI7(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        return DecodeI7(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx, size_t limit) const
    {
        return EncodeI7(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx) const
    {
        return EncodeI7(buffer, value, idx);
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
using UByteI7Codec = I7Codec<UByte, Value>;

}
