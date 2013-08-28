
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_I64CODEC_HPP_
#define MEMORIA_CORE_TOOLS_I64CODEC_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/container/macros.hpp>

namespace memoria {

template <typename V>
//__attribute__((always_inline))
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
//__attribute__((always_inline))
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
//__attribute__((always_inline))
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
//__attribute__((always_inline))
size_t DecodeI64(const T* buffer, V& value, size_t start)
{
    T length = GetBits(buffer, start, 8);

    if (__builtin_expect((length & 0x1) == 0, 0))
    {
        value = 0;
        return 1;
    }
    else if (__builtin_expect((length & 0x2) == 0, 1))
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
    static const Int BitsPerOffset  = 8;
    static const Int ElementSize    = 1; // In bits;

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
using UBigIntI64Codec = I64Codec<UBigInt, Value>;

}

#endif
