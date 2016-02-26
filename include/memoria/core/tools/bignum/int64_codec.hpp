
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_BIGNUM_INT64_CODEC_HPP_
#define MEMORIA_CORE_TOOLS_BIGNUM_INT64_CODEC_HPP_

#include <memoria/core/tools/strings/string.hpp>
#include <memoria/core/tools/bitmap.hpp>

namespace memoria {

template <typename> class ValueCodec;

template <>
class ValueCodec<int64_t> {
public:
    using BufferType 	= UByte;
    using T 			= BufferType;
    using V 			= int64_t;

    static const Int BitsPerOffset  = 4;
    static const Int ElementSize    = 8; // In bits;

    size_t length(const T* buffer, size_t idx, size_t limit) const
    {
    	auto head = buffer[idx];
    	if (head < 252) {
    		return 1;
    	}
    	else {
    		return buffer[idx + 1] + 1;
    	}
    }

    size_t length(const V& value) const
    {
    	if (value >= 0)
    	{
    		if (value < 126)
    		{
    			return 1;
    		}
    		else {
    			return 2 + byte_length(value);
    		}
    	}
    	else {
    		if (value > -127)
    		{
    			return 1;
    		}
    		else {
    			return 2 + byte_length(-value);
    		}
    	}
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return decode(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
    	auto header = buffer[idx];

    	if (header < 126u)
    	{
    		value = header;
    		return 1;
    	}
    	else if (header < 252u)
    	{
    		value = -(header - 125);
    		return 1;
    	}
    	else
    	{
    		value = 0;
    		idx++;

    		size_t len = buffer[idx++];

    		deserialize(buffer, value, idx, len);

    		if (header == 253u)
    		{
    			value = -value;
    		}

    		return len + 2;
    	}
    }

    size_t encode(T* buffer, const V& value, size_t idx, size_t limit) const
    {
        return encode(buffer, value, idx);
    }

    size_t encode(T* buffer, const V& value, size_t idx) const
    {
    	if (value >= 0)
    	{
    		if (value < 126ul)
    		{
    			buffer[idx] = value;
    			return 1;
    		}
    		else {
    			buffer[idx] = 252u;

    			size_t len = serialize(buffer, value, idx + 2);

    			buffer[idx + 1] = len;

    			return 2 + len;
    		}
    	}
    	else {
    		if (value > -127)
    		{
    			buffer[idx] = (-value) + 125;
    			return 1;
    		}
    		else {
    			buffer[idx] = 253u;

    			size_t len = serialize(buffer, -value, idx + 2);

    			buffer[idx + 1] = len;

    			return 2 + len;
    		}
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



    static size_t serialize(T* buffer, int64_t value, size_t idx)
    {
    	UInt len = bytes(value);

    	for (UInt c = 0; c < len; c++)
    	{
    		buffer[idx++] = value >> (c << 3);
    	}

    	return len;
    }

    static void deserialize(const T* buffer, int64_t& value, size_t idx, size_t len)
    {
    	for (size_t c = 0; c < len; c++)
    	{
    		value |= ((int64_t)buffer[idx++]) << (c << 3);
    	}
    }


	constexpr static UInt msb(unsigned long long digits)
	{
		return 63 - __builtin_clzll(digits);
	}

	template <typename T>
	constexpr static UInt bytes(T digits)
	{
		UInt v = msb(digits) + 1;
		return (v >> 3) + ((v & 0x7) != 0);
	}

	constexpr static UInt byte_length(const int64_t data)
	{
		return bytes(data);
	}
};



}

#endif
