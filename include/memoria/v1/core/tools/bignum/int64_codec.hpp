
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/tools/strings/string.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {

template <typename> class ValueCodec;

template <>
class ValueCodec<int64_t> {
public:
    using BufferType    = UByte;
    using T             = BufferType;
    using V             = int64_t;

    using ValuePtr      = ValuePtrT1<BufferType>;

    static constexpr Int BitsPerOffset  = 4;
    static constexpr Int ElementSize    = 8; // In bits;

    ValuePtr describe(const T* buffer, size_t idx)
    {
        return ValuePtr(buffer + idx, length(buffer, idx, -1ull));
    }

    size_t length(const T* buffer, size_t idx, size_t limit) const
    {
        auto head = buffer[idx];
        if (head < 252) {
            return 1;
        }
        else {
            return buffer[idx + 1] + 2;
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


template <>
class ValueCodec<uint64_t> {
public:
    using BufferType    = UByte;
    using T             = BufferType;
    using V             = uint64_t;

    using ValuePtr      = ValuePtrT1<BufferType>;

    static constexpr Int BitsPerOffset  = 4;
    static constexpr Int ElementSize    = 8; // In bits;

    ValuePtr describe(const T* buffer, size_t idx)
    {
        return ValuePtr(buffer + idx, length(buffer, idx, -1ull));
    }

    size_t length(const T* buffer, size_t idx, size_t limit) const
    {
        auto head = buffer[idx];
        if (head < 252) {
            return 1;
        }
        else {
            return buffer[idx + 1] + 2;
        }
    }

    size_t length(const V& value) const
    {
        if (value < 126)
        {
            return 1;
        }
        else {
            return 2 + byte_length(value);
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
        else
        {
            value = 0;
            idx++;

            size_t len = buffer[idx++];

            deserialize(buffer, value, idx, len);

            return len + 2;
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

    void move(T* buffer, size_t from, size_t to, size_t size) const
    {
        CopyBuffer(buffer + from, buffer + to, size);
    }

    void copy(const T* src, size_t from, T* tgt, size_t to, size_t size) const
    {
        CopyBuffer(src + from, tgt + to, size);
    }

private:

    static size_t serialize(T* buffer, V value, size_t idx)
    {
        UInt len = bytes(value);

        for (UInt c = 0; c < len; c++)
        {
            buffer[idx++] = value >> (c << 3);
        }

        return len;
    }

    static void deserialize(const T* buffer, V& value, size_t idx, size_t len)
    {
        for (size_t c = 0; c < len; c++)
        {
            value |= ((V)buffer[idx++]) << (c << 3);
        }
    }


    constexpr static UInt msb(unsigned long long digits)
    {
        return 63 - __builtin_clzll(digits);
    }

    template <typename T>
    static UInt bytes(T digits)
    {
        UInt v = msb(digits) + 1;
        return (v >> 3) + ((v & 0x7) != 0);
    }

    static UInt byte_length(const int64_t data)
    {
        return bytes(data);
    }
};


template <typename V>
class PrimitiveTypeCodec {
    using Codec = IfThenElse<
            std::is_signed<V>::value,
            ValueCodec<int64_t>,
            ValueCodec<uint64_t>
    >;

    Codec codec_;

public:
    using BufferType    = typename Codec::BufferType;
    using T             = BufferType;
    using VV            = typename Codec::V;

    using ValuePtr      = typename Codec::ValuePtr;

    static constexpr Int BitsPerOffset  = Codec::BitsPerOffset;
    static constexpr Int ElementSize    = Codec::ElementSize;

    auto describe(const T* buffer, size_t idx)
    {
        return codec_.describe(buffer, idx);
    }

    size_t length(const T* buffer, size_t idx, size_t limit) const
    {
        return codec_.length(buffer, idx, limit);
    }

    size_t length(V value) const
    {
        return codec_.length(value);
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return decode(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        VV value0;
        auto len = codec_.decode(buffer, value0, idx);
        value = value0;
        return len;
    }

    size_t encode(T* buffer, const ValuePtr& value, size_t idx) const
    {
        codec_.encode(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx, size_t limit) const
    {
        return codec_.encode(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx) const
    {
        return codec_.encode(buffer, value, idx);
    }

    void move(T* buffer, size_t from, size_t to, size_t size) const
    {
        codec_.move(buffer, from, to, size);
    }

    void copy(const T* src, size_t from, T* tgt, size_t to, size_t size) const
    {
        codec_.copy(src, from, tgt, to, size);
    }
};

template <>
class ValueCodec<Byte>: public PrimitiveTypeCodec<Byte> {};

template <>
class ValueCodec<Char>: public PrimitiveTypeCodec<Char> {};

template <>
class ValueCodec<UByte>: public PrimitiveTypeCodec<UByte> {};

template <>
class ValueCodec<Short>: public PrimitiveTypeCodec<Short> {};

template <>
class ValueCodec<UShort>: public PrimitiveTypeCodec<UShort> {};

template <>
class ValueCodec<Int>: public PrimitiveTypeCodec<Int> {};

template <>
class ValueCodec<UInt>: public PrimitiveTypeCodec<UInt> {};

template <>
class ValueCodec<bool>: public PrimitiveTypeCodec<bool> {};

}
