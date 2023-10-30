
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
 * Elias Delta based codec. It has right the same code lengths as the original Elias Delta code,
 * but a bit different code words. See the EncodeEliasDelta procedure for details.
 */


#include <memoria/core/types.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/bitmap_select.hpp>

namespace memoria {

template <typename V>
size_t GetEliasDeltaValueLength(V value)
{
    size_t length           = Log2(value);
    size_t length_length    = Log2(length) - 1;

    return length_length * 2 + length;
}


template <typename T, typename V>
size_t EncodeEliasDelta(T* buffer, V value, size_t start, size_t limit = -1)
{
    if (value > 1)
    {
        size_t length           = Log2(value);
        size_t length_length    = Log2(length) - 1;

        // Fill in leading zeroes.
        FillZero(buffer, start, start + length_length);

        V length0 = length;

        // Move the most significant bit of the value's length into position 0.
        // The MSB will always be 1, so this 1 will delimit run of zeroes from
        // the bit run of the value's length.
        // This operation differs this code from the original ELias Delta code.
        length0 <<= 1;
        length0 +=  1;

        // Write in the length
        start += length_length;
        SetBits(buffer, start, length0, length_length + 1);

        // Write in the value
        start += length_length + 1;
        SetBits(buffer, start, value, length - 1);

        return length_length * 2 + length;
    }
    else {
        SetBits(buffer, start, 1, 1);

        return 1;
    }


}

template <typename T, typename V>
inline size_t DecodeEliasDelta(const T* buffer, V& value, size_t start, size_t limit = -1)
{
    if (TestBit(buffer, start))
    {
        value = 1;
        return 1;
    }
    else {
        // TODO: Small values optimization
        size_t trailing_zeroes = CountTrailingZeroes(buffer, start, limit);

        start += trailing_zeroes;
        T length = GetBits(buffer, start, trailing_zeroes + 1);

        T ONE = static_cast<T>(1);

        length >>= 1;
        length += (ONE << trailing_zeroes);

        start += trailing_zeroes + 1;
        value = GetBits(buffer, start, length - 1);

        value += (ONE << (length - 1));

        return 2 * trailing_zeroes + length;
    }
}


template <typename T>
inline size_t DecodeEliasDeltaLength(const T* buffer, size_t start, size_t limit = -1)
{
    if (TestBit(buffer, start))
    {
        return 1;
    }
    else {
        size_t trailing_zeroes = CountTrailingZeroes(buffer, start, limit);

        start += trailing_zeroes;
        T length = GetBits(buffer, start, trailing_zeroes + 1);

        T ONE = static_cast<T>(1);

        length >>= 1;

        length += (ONE << trailing_zeroes);

        return 2 * trailing_zeroes + length;
    }
}



template <typename T, typename V>
struct EliasDeltaCodec {

    typedef T BufferType;
    static const int32_t BitsPerOffset  = 8;
    static const int32_t ElementSize    = 1; // In bits;

    const void* addr(const T* buffer, size_t pos) const {
        return &buffer[pos/TypeBitsize<T>()];
    }

    size_t length(const T* buffer, size_t idx, size_t limit) const
    {
        return DecodeEliasDeltaLength(buffer, idx, limit);
    }

    size_t length(V value) const
    {
        return GetEliasDeltaValueLength(value + 1);
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        size_t len = DecodeEliasDelta(buffer, value, idx, limit);

        value--;

        return len;
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        size_t len = DecodeEliasDelta(buffer, value, idx);

        value--;

        return len;
    }

    size_t encode(T* buffer, V value, size_t idx, size_t limit) const
    {
        value++;
        return EncodeEliasDelta(buffer, value, idx, limit);
    }

    size_t encode(T* buffer, V value, size_t idx) const
    {
        value++;
        return EncodeEliasDelta(buffer, value, idx);
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


template <typename V>
using UInt64EliasCodec = EliasDeltaCodec<uint64_t, V>;



}
