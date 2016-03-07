
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_BIGNUM_CPPINT_CODEC_HPP_
#define MEMORIA_CORE_TOOLS_BIGNUM_CPPINT_CODEC_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <boost/multiprecision/cpp_int.hpp>

namespace memoria {

template <typename> class ValueCodec;

namespace mp = boost::multiprecision;

template <
        unsigned MinDigits,
        unsigned MaxDigits,
        mp::cpp_integer_type SignType,
        mp::cpp_int_check_type Checked,
        class Allocator,
        mp::expression_template_option ExpressionTemplates
>
class ValueCodec<mp::number<mp::cpp_int_backend<MinDigits, MaxDigits, SignType, Checked, Allocator>, ExpressionTemplates>> {
public:
    using BufferType    = UByte;
    using T             = BufferType;
    using V             = mp::number<mp::cpp_int_backend<MinDigits, MaxDigits, SignType, Checked, Allocator>, ExpressionTemplates>;

    using ValuePtr      = ValuePtrT1<BufferType>;

    static const Int BitsPerOffset  = 16;
    static const Int ElementSize    = 8; // In bits;

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
        else if (head < 254) {
            return buffer[idx + 1] + 2;
        }
        else {
            size_t len = 0;
            for (size_t c = idx + 1; c < idx + 4; c++)
            {
                len |= buffer[c];
                len <<= 8;
            }

            return len + 5;
        }
    }

    size_t length(const V& value) const
    {
        auto size       = value.backend().size();
        auto limbs      = value.backend().limbs();
        bool negative   = value.sign() == -1;

        using LimbType  = decltype(*limbs);

        if (size == 1u)
        {
            if (!negative)
            {
                if (limbs[0] < 126ul)
                {
                    return 1;
                }
                else {
                    return 2 + byte_length(limbs, size);
                }
            }
            else {
                if (limbs[0] < 127ul)
                {
                    return 1;
                }
                else {
                    return 2 + byte_length(limbs, size);
                }
            }
        }
        else if (size <= 256u / sizeof(LimbType))
        {
            return 2 + byte_length(limbs, size);
        }
        else {
            return 5 + byte_length(limbs, size);
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
            value = (unsigned long long)header;
            return 1;
        }
        else if (header < 252u)
        {
            value = -(header - 125);
            return 1;
        }
        else if (header < 254u)
        {
            idx++;

            size_t len = buffer[idx++];

            deserialize(buffer, value, idx, len);

            value.backend().sign(header == 253u);

            return len + 2;
        }
        else {
            idx++;

            size_t len = 0;

            for (UInt c = 0; c < 4; c++)
            {
                len |= ((size_t)buffer[idx++]) << (c << 3);
            }

            deserialize(buffer, value, idx, len);

            value.backend().sign(header == 255u);

            return len + 5;
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
        auto size       = value.backend().size();
        auto limbs      = value.backend().limbs();
        bool negative   = value.sign() == -1;
        using LimbType  = decltype(*limbs);

        if (size == 1u)
        {
            if (!negative)
            {
                if (limbs[0] < 126ul)
                {
                    buffer[idx] = limbs[0];
                    return 1;
                }
                else {
                    buffer[idx]     = 252u;

                    size_t len = serialize_limb(buffer, limbs[0], idx + 2);

                    buffer[idx + 1] = len;

                    return 2 + len;
                }
            }
            else {
                if (limbs[0] < 127ul)
                {
                    buffer[idx] = limbs[0] + 125u;
                    return 1;
                }
                else {
                    buffer[idx] = 253u;

                    size_t len = serialize_limb(buffer, limbs[0], idx + 2);

                    buffer[idx + 1] = len;

                    return 2 + len;
                }
            }
        }
        else if (size <= 256u/sizeof(LimbType))
        {
            size_t len = serialize(buffer, value, idx + 2);

            buffer[idx] = negative ? 253u : 252u;
            buffer[idx + 1] = len;

            return 2 + len;
        }
        else {
            size_t len = serialize(buffer, value, idx + 5);

            buffer[idx++] = negative ? 255u : 254u;

            for (size_t c = 0; c < 4; c++)
            {
                buffer[idx++] = len >> (c << 3);
            }

            return 5 + len;
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

    static size_t serialize_limb(T* buffer, unsigned long value, size_t idx)
    {
        UInt len = bytes(value);

        for (UInt c = 0; c < len; c++)
        {
            buffer[idx++] = value >> (c << 3);
        }

        return len;
    }

    static size_t serialize_limb(T* buffer, unsigned long long value, size_t idx)
    {
        UInt len = bytes(value);

        for (UInt c = 0; c < len; c++)
        {
            buffer[idx++] = value >> (c << 3);
        }

        return len;
    }

    static size_t serialize(T* buffer, const V& value, size_t idx)
    {
        auto end = mp::export_bits(value, buffer + idx, 8, false);
        return (size_t)(end - (buffer + idx));
    }

    static void deserialize(const T* buffer, V& value, size_t idx, size_t len)
    {
        mp::import_bits(value, buffer + idx, buffer + idx + len, 8, false);
    }

    constexpr static UInt msb(unsigned digits)
    {
        return 31 - __builtin_clz(digits);
    }

    constexpr static    UInt msb(unsigned long digits)
    {
        return 63 - __builtin_clzl(digits);
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

    template <typename T>
    constexpr static UInt byte_length(const T* data, unsigned size)
    {
        return bytes(data[size - 1]) + (size - 1) * sizeof(T);
    }
};

}

#endif
