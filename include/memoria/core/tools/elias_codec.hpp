
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_ELIASCODEC_HPP_
#define MEMORIA_CORE_TOOLS_ELIASCODEC_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/bitmap_select.hpp>

namespace memoria {

template <typename V>
size_t GetEliasDeltaValueLength(V value)
{
	size_t length 			= Log2(value);
	size_t length_length	= Log2(length) - 1;

	return length_length * 2 + length;
}


template <typename T, typename V>
size_t EncodeEliasDelta(T* buffer, V value, size_t start, size_t limit)
{
	size_t length 			= Log2(value);
	size_t length_length 	= Log2(length) - 1;

	FillZero(buffer, start, start + length_length);

	V length0 = length;

	length0 <<= 1;
	length0 += 1;

	start += length_length;
	SetBits(buffer, start, length0, length_length + 1);

	start += length_length + 1;
	SetBits(buffer, start, value, length - 1);

	return length_length * 2 + length;
}

template <typename T, typename V>
size_t DecodeEliasDelta(const T* buffer, V& value, size_t start, size_t limit)
{
	size_t trailing_zeroes = CountTrailingZeroes(buffer, start, limit);

	start += trailing_zeroes;
	T length = GetBits(buffer, start, trailing_zeroes + 1);

	length >>= 1;

	length += (1 << trailing_zeroes);

	MEMORIA_ASSERT(length, <, 64);

	start += trailing_zeroes + 1;
	value = GetBits(buffer, start, length);

	value += (1 << (length - 1));

	return 2 * trailing_zeroes + length;
}

template <typename T, typename V>
struct EliasDeltaCodec {

	size_t length(const T* buffer, size_t idx) const {
		return buffer[idx] + 1;
	}

	size_t length(V value) const {
		return GetEliasDeltaValueLength(value);
	}

	size_t decode(const T* buffer, V& value, size_t idx) const
	{
		return DecodeEliasDelta(buffer, value, idx);
	}

	size_t encode(T* buffer, V value, size_t idx) const
	{
		return EncodeEliasDelta(buffer, value, idx);
	}
};



}

#endif
