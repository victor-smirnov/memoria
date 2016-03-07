
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_BIGINT_HPP_
#define MEMORIA_CORE_TOOLS_BIGINT_HPP_


#include <memoria/core/types/types.hpp>

#include <memoria/core/tools/bignum/cppint_codec.hpp>

#include <boost/multiprecision/cpp_int.hpp>



#include <ostream>

namespace memoria {

/*
template <typename> class IntegerCodec;

template <size_t FixedPartPrecision = 4>
class BigIntegerT {
    UInt metadata_;

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

    BigIntegerT(Int value)
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

    BigIntegerT(UInt value)
    {
        init_small();

        content_.fixed_.digits_[0] = value;
        compute_msb_small();
    }

    BigIntegerT(BigInt value)
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

    BigIntegerT(UBigInt value)
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
            ::free(content_.variable_.digits_);
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

    Int sign() const
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

    UInt msb() const
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
        UInt msb;
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
        UInt msb = compute_msb(content_.fixed_.digits_, FixedPartPrecision);
        metadata_ |= msb << 2;
    }

    void make_big(size_t len)
    {
        len = round_up(len);

        if (is_small())
        {
            //FIXME: throw ex
            content_.variable_.digits_      = T2T<unsigned*>(::malloc(len));
            content_.variable_.block_size_  = len;
            metadata_ &= ~0x1u;
        }
        else if (len != content_.variable_.block_size_)
        {
            ::free(content_.variable_.digits_);

            content_.variable_.digits_      = T2T<unsigned*>(::malloc(len));
            content_.variable_.block_size_  = len;
        }

        memset(content_.variable_.digits_, 0, len);
    }

    void make_small()
    {
        if (!is_small())
        {
            ::free(content_.variable_.digits_);
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

    UInt compute_msb(const DigitType* digits, size_t len)
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

    UInt msb(unsigned digits)
    {
        return 31 - __builtin_clz(digits);
    }

    UInt msb(unsigned long digits)
    {
        return 63 - __builtin_clzl(digits);
    }

    UInt msb(unsigned long long digits)
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
    static const UInt Value = 50;
};

}

#endif
