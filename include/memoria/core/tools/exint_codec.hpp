
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_EXINTCODEC_HPP_
#define MEMORIA_CORE_TOOLS_EXINTCODEC_HPP_

/**
 * Partial implementation of EXINT universal code by Dustin Juliano
 * http://julianoresearch.com/publications/standards/exint/
 */

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/bitmap.hpp>

namespace memoria {

template <typename V>
size_t GetExintValueLength(V value)
{
    //MEMORIA_ASSERT(value, >=, 0);

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
    //MEMORIA_ASSERT(value, >=, 0);

    UByte& byte_length = buffer[start];

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
    static const Int BitsPerOffset  = 4;
    static const Int ElementSize    = 8; // In bits;

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
using UByteExintCodec = ExintCodec<UByte, Value>;

}

#endif
