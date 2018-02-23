
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

#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/core/bignum/cppint_codec.hpp>

#include <ostream>

namespace memoria {
namespace v1 {

/*
template <typename> class IntegerCodec;

template <size_t FixedPartPrecision = 4>
class BigIntegerT {
    uint32_t metadata_;

    using DigitType = unsigned;

    struct Fixed {
        DigitType digits_[FixedPartPrecision];
    };

    struct Variable {
        DigitType* digits_;
        size_t block_size_;
    };

    union {
        Fixed fixed_;
        Variable variable_;
    }
    content_;

    template <typename> friend class IntegerCodec;

public:
    BigIntegerT()
    {
        init_small();
    }

    BigIntegerT(int32_t value)
    {
        init_small();

        if (value < 0)
        {
            value = -value;
            set_negative(true);
        }

        content_.fixed_.digits_[0] = -value;

        compute_msb_small();
    }

    BigIntegerT(uint32_t value)
    {
        init_small();

        content_.fixed_.digits_[0] = value;
        compute_msb_small();
    }

    BigIntegerT(int64_t value)
    {
        init_small();

        if (value < 0)
        {
            value = -value;
            set_negative(true);
        }

        content_.fixed_.digits_[0] = value;
        content_.fixed_.digits_[1] = value >> (sizeof(DigitType) * 32);

        compute_msb_small();
    }

    BigIntegerT(uint64_t value)
    {
        init_small();

        content_.fixed_.digits_[0] = value;
        content_.fixed_.digits_[1] = value >> (sizeof(DigitType) * 32);

        compute_msb_small();
    }


    ~BigIntegerT()
    {
        if (!is_small())
        {
            free_system(content_.variable_.digits_);
        }
    }

    bool is_small() const
    {
        return metadata_ & 0x1;
    }

    bool is_negative() const
    {
        return metadata_ & 0x2;
    }

    int32_t sign() const
    {
        return !(metadata_ & 0x2) ? 1 : -1;
    }

    const DigitType* data() const
    {
        if (is_small())
        {
            return content_.fixed_.digits_;
        }
        else {
            return content_.variable_.digits_;
        }
    }

    DigitType* data()
    {
        if (is_small())
        {
            return content_.fixed_.digits_;
        }
        else {
            return content_.variable_.digits_;
        }
    }

    uint32_t msb() const
    {
        return metadata_ >> 2;
    }

protected:

    void set_negative(bool negative)
    {
        metadata_ |= negative ? 0x2 : 0x0;
    }

    void compute_msb()
    {
        uint32_t msb;
        if (is_small())
        {
            msb = compute_msb(content_.fixed_.digits_, FixedPartPrecision);
        }
        else {
            msb = compute_msb(content_.variable_.digits_, content_.variable_.block_size_ / sizeof(DigitType));
        }

        metadata_ |= msb << 2;
    }

    void compute_msb_small()
    {
        uint32_t msb = compute_msb(content_.fixed_.digits_, FixedPartPrecision);
        metadata_ |= msb << 2;
    }

    void make_big(size_t len)
    {
        len = round_up(len);

        if (is_small())
        {
            //FIXME: throw ex
            content_.variable_.digits_      = allocate_system<unsigned>(len);
            content_.variable_.block_size_  = len;
            metadata_ &= ~0x1u;
        }
        else if (len != content_.variable_.block_size_)
        {
            free_system(content_.variable_.digits_);

            content_.variable_.digits_      = allocate_system<unsigned>(len);
            content_.variable_.block_size_  = len;
        }

        memset(content_.variable_.digits_, 0, len);
    }

    void make_small()
    {
        if (!is_small())
        {
            free_system(content_.variable_.digits_);
            metadata_ |= 0x1u;
        }

        memset(content_.fixed_.digits_, 0, FixedPartPrecision * sizeof(DigitType));
    }

private:
    size_t round_up(size_t size)
    {
        constexpr size_t Mask = (Log2(sizeof(DigitType)) << 1) - 1;

        if (size & Mask)
        {
            return (size | Mask) + 1;
        }
        else {
            return size;
        }
    }

    uint32_t compute_msb(const DigitType* digits, size_t len)
    {
        for (size_t c = 0; c < len; c++)
        {
            auto data = digits[len - c - 1];
            if (data > 0)
            {
                return c * (sizeof(DigitType) * 8) + msb(data);
            }
        }

        return 0;
    }

    uint32_t msb(unsigned digits)
    {
        return 31 - __builtin_clz(digits);
    }

    uint32_t msb(unsigned long digits)
    {
        return 63 - __builtin_clzl(digits);
    }

    uint32_t msb(unsigned long long digits)
    {
        return 63 - __builtin_clzll(digits);
    }

    void init_small()
    {
        metadata_ = 0x1;
        memset(content_.fixed_.digits_, 0, FixedPartPrecision * sizeof(DigitType));
    }
};

*/

using BigInteger = boost::multiprecision::cpp_int;

template <typename T> struct TypeHash;

template <>
struct TypeHash<BigInteger> {
    static const uint64_t Value = 50;
};

}}
