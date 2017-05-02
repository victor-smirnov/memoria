
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/tools/bignum/cppint_codec.hpp>
#include <memoria/v1/core/tools/bignum/int64_codec.hpp>

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>

namespace memoria {
namespace v1 {

namespace mp = boost::multiprecision;

namespace {

    template <
        typename Backend
    >
    class CPPIntBackendCodec {
    public:
        using BufferType    = uint8_t;
        using T             = BufferType;
        using V             = Backend;

        static const int32_t BitsPerOffset  = 16;
        static const int32_t ElementSize    = 8; // In bits;

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
            auto size       = value.size();
            auto limbs      = value.limbs();
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


        std::pair<size_t, bool> decode(const T* buffer, V& value, size_t idx) const
        {
            auto header = buffer[idx];

            if (header < 126u)
            {
                value = (unsigned long long)header;
                return std::make_pair(1ull, false);
            }
            else if (header < 252u)
            {
                value = (unsigned long long) (header - 125);
                return std::make_pair(1ull, true);
            }
            else if (header < 254u)
            {
                idx++;

                size_t len = buffer[idx++];

                deserialize(buffer, value, idx, len);

                return std::make_pair(len + 2, header == 253u);
            }
            else {
                idx++;

                size_t len = 0;

                for (uint32_t c = 0; c < 4; c++)
                {
                    len |= ((size_t)buffer[idx++]) << (c << 3);
                }

                deserialize(buffer, value, idx, len);

                return std::make_pair(len + 2, header == 255u);
            }
        }

        size_t encode(T* buffer, const V& value, size_t idx, size_t limit) const
        {
            return encode(buffer, value, idx);
        }

        size_t encode(T* buffer, const V& value, size_t idx, bool negative) const
        {
            auto size       = value.size();
            auto limbs      = value.limbs();
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
            uint32_t len = bytes(value);

            for (uint32_t c = 0; c < len; c++)
            {
                buffer[idx++] = value >> (c << 3);
            }

            return len;
        }

        static size_t serialize_limb(T* buffer, unsigned long long value, size_t idx)
        {
            uint32_t len = bytes(value);

            for (uint32_t c = 0; c < len; c++)
            {
                buffer[idx++] = value >> (c << 3);
            }

            return len;
        }

        static size_t serialize(T* buffer, const V& value, size_t idx)
        {
            using TagType = typename V::trivial_tag;

            uint32_t pos = 0;
            uint32_t size = msb(value.limbs(), value.size());

            auto idx0 = idx;

            do
            {
                buffer[idx++] = mp::detail::extract_bits(value, pos, 8, TagType());
                pos += 8;
            }
            while(pos < size);

            return idx - idx0;
        }

        static void deserialize(const T* buffer, V& value, size_t idx, size_t len)
        {
            V newval;

            using TagType = typename V::trivial_tag;


            constexpr size_t chunk_size = 8;

            size_t limbs = len;
            size_t bits  = limbs * chunk_size;

            mp::detail::resize_to_bit_size(newval, bits, TagType());

            uint32_t pos = 0;

            for (size_t c = 0; c < len; c++)
            {
                unsigned i = buffer[idx++];
                mp::detail::assign_bits(newval, i, pos, chunk_size, TagType());
                pos += chunk_size;
            }

            newval.normalize();

            value.swap(newval);
        }

        constexpr static uint32_t msb(unsigned digits)
        {
            return 31 - __builtin_clz(digits);
        }

        constexpr static    uint32_t msb(unsigned long digits)
        {
            return 63 - __builtin_clzl(digits);
        }

        constexpr static uint32_t msb(unsigned long long digits)
        {
            return 63 - __builtin_clzll(digits);
        }

        template <typename T>
        constexpr static uint32_t msb(const T* data, unsigned size)
        {
            return msb(data[size - 1]) + (size - 1) * sizeof(T) * 8;
        }

        template <typename T>
        constexpr static uint32_t bytes(T digits)
        {
            uint32_t v = msb(digits) + 1;
            return (v >> 3) + ((v & 0x7) != 0);
        }

        template <typename T>
        constexpr static uint32_t byte_length(const T* data, unsigned size)
        {
            return bytes(data[size - 1]) + (size - 1) * sizeof(T);
        }
    };
}


template <
        unsigned Digits,
        mp::backends::digit_base_type DigitBase,
        class Allocator,
        class Exponent,
        Exponent MinExponent,
        Exponent MaxExponent,
        mp::expression_template_option ExpressionTemplates
>
class ValueCodec<mp::number<mp::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinExponent, MaxExponent>, ExpressionTemplates>> {
public:
    using BufferType    = uint8_t;
    using T             = BufferType;
    using V             = mp::number<mp::cpp_bin_float<Digits, DigitBase, Allocator, Exponent, MinExponent, MaxExponent>, ExpressionTemplates>;

    static const int32_t BitsPerOffset  = 16;
    static const int32_t ElementSize    = 8; // In bits;

private:
    using IntBackend = typename std::remove_reference<decltype(std::declval<V>().backend().bits())>::type;
    CPPIntBackendCodec<IntBackend> cppint_codec_;
    ValueCodec<int64_t> int64_codec_;

public:
    size_t length(const T* buffer, size_t idx, size_t limit) const
    {
        size_t len1 = cppint_codec_.length(buffer, idx, limit);
        size_t len2 = int64_codec_.length(buffer, idx + len1, limit);

        return len1 + len2;
    }

    size_t length(const V& value) const
    {
        size_t len1 = cppint_codec_.length(value);
        size_t len2 = int64_codec_.length(value);

        return len1 + len2;
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return decode(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        auto len1 = cppint_codec_.decode(buffer, value.backend().bits(), idx);

        int64_t exponent;
        size_t len2 = int64_codec_.decode(buffer, exponent, idx + len1.first);

        value.backend().exponent() = exponent;

        if (len1.second)
        {
            value.backend().sign() = -1;
        }

        return len1.first + len2;
    }

    size_t encode(T* buffer, const V& value, size_t idx, size_t limit) const
    {
        return encode(buffer, value, idx);
    }

    size_t encode(T* buffer, const V& value, size_t idx) const
    {
        size_t len1 = cppint_codec_.encode(buffer, value.backend().bits(), idx, value.sign() == -1);
        size_t len2 = int64_codec_.encode(buffer, value.backend().exponent(), idx + len1);

        return len1 + len2;
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



}}
