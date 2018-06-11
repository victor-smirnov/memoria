
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/core/integer/accumulator_common.hpp>

#include <boost/multiprecision/integer.hpp>
#include <boost/detail/endian.hpp>

#include <ostream>

namespace memoria {
namespace v1 {

namespace bmp = boost::multiprecision;

namespace _ {

    template <typename T, size_t N = sizeof(T) / sizeof (bmp::limb_type)> struct UAccCvtHelper;

    template <typename T>
    struct UAccCvtHelper<T, 1>
    {
        template <typename UAcc, typename BmpInt>
        static void to_acc(UAcc& acc, const BmpInt& value) {
            acc.from_same_limb_cppint(value);
        }

        template <typename UAcc, typename BmpInt>
        static void to_bmp_int(const UAcc& acc, BmpInt& value) {
            acc.to_same_limb_cppint(value);
        }
    };

    template <typename T>
    struct UAccCvtHelper<T, 2>
    {
        template <typename UAcc, typename BmpInt>
        static void to_acc(UAcc& acc, const BmpInt& value) {
            acc.from_smaller_limb_cppint(value);
        }

        template <typename UAcc, typename BmpInt>
        static void to_bmp_int(const UAcc& acc, BmpInt& value) {
            acc.to_smaller_limb_cppint(value);
        }
    };

    template <unsigned BitLength>
    using UAccBmpInt = bmp::number<
        bmp::cpp_int_backend<BitLength, BitLength, bmp::unsigned_magnitude, bmp::unchecked, void>
    >;
}



template <size_t BitLength>
struct UnsignedAccumulator {
    using ValueT = uint64_t;

    static constexpr size_t ValueTBitLength = sizeof(ValueT) * 8;

    static constexpr size_t Size = (BitLength / ValueTBitLength) + (BitLength % ValueTBitLength > 0);

    ValueT value_[Size];

    constexpr UnsignedAccumulator(): value_{} {}

    template <unsigned BmpBitLength>
    constexpr UnsignedAccumulator(
            const _::UAccBmpInt<BmpBitLength>& bmp_value,
            std::enable_if_t<BitLength >= BmpBitLength>* tt = nullptr
    ): value_{}
    {
        _::UAccCvtHelper<ValueT>::to_acc(*this, bmp_value);
    }

    constexpr UnsignedAccumulator(ValueT v): value_{} {
        value_[0] = v;
    }

    UnsignedAccumulator(const std::string& digits):
        UnsignedAccumulator(_::UAccBmpInt<BitLength>(digits))
    {}



    bool operator<(const UnsignedAccumulator& other) const
    {
        for (size_t c = Size - 1; c > 0; c--) {
            if (value_[c] != other.value_[c]) {
                return value_[c] < other.value_[c];
            }
        }

        return value_[0] < other.value_[0];
    }

    bool operator<=(const UnsignedAccumulator& other) const
    {
        for (size_t c = Size - 1; c > 0; c--) {
            if (value_[c] != other.value_[c]) {
                return value_[c] < other.value_[c];
            }
        }

        return value_[0] <= other.value_[0];
    }

    bool operator>(const UnsignedAccumulator& other) const
    {
        for (size_t c = Size - 1; c > 0; c--) {
            if (value_[c] != other.value_[c]) {
                return value_[c] > other.value_[c];
            }
        }

        return value_[0] > other.value_[0];
    }

    bool operator>=(const UnsignedAccumulator& other) const
    {
        for (size_t c = Size - 1; c > 0; c--) {
            if (value_[c] != other.value_[c]) {
                return value_[c] > other.value_[c];
            }
        }

        return value_[0] >= other.value_[0];
    }

    bool operator!=(const UnsignedAccumulator& other) const
    {
        for (size_t c = 0; c < Size; c++) {
            if (value_[c] != other.value_[c]) {
                return true;
            }
        }

        return false;
    }

    MMA1_NODISCARD bool operator==(const UnsignedAccumulator& other) const
    {
        for (size_t c = 0; c < Size; c++) {
            if (value_[c] != other.value_[c]) {
                return false;
            }
        }

        return true;
    }

    template <unsigned BmpBitLength>
    MMA1_NODISCARD bool operator==(const _::UAccBmpInt<BmpBitLength>& other) const
    {
        UnsignedAccumulator tmp{other};
        return tmp == *this;
    }

    UnsignedAccumulator& operator+=(const UnsignedAccumulator& other)
    {
        _::long_add_to(value_, other.value_, Size);
        return *this;
    }

    UnsignedAccumulator& operator+=(ValueT other)
    {
        ValueT tmp[Size] {};
        tmp[0] = other;

        _::long_add_to(value_, tmp, Size);

        return *this;
    }

    template <unsigned BmpBitLength>
    UnsignedAccumulator& operator+=(const _::UAccBmpInt<BmpBitLength>& other)
    {
        UnsignedAccumulator tmp{other};
        return (*this) += tmp;
    }

    UnsignedAccumulator& operator-=(const UnsignedAccumulator& other)
    {
        _::long_sub_from(value_, other.value_, Size);
        return *this;
    }

    UnsignedAccumulator& operator-=(ValueT other)
    {
        ValueT tmp[Size] {};
        tmp[0] = other;

        _::long_sub_from(value_, tmp, Size);

        return *this;
    }

    template <unsigned BmpBitLength>
    UnsignedAccumulator& operator-=(const _::UAccBmpInt<BmpBitLength>& other)
    {
        UnsignedAccumulator tmp{other};
        return (*this) -= tmp;
    }

    UnsignedAccumulator operator++()
    {
        UnsignedAccumulator tmp{*this};
        tmp += 1;

        return tmp;
    }

    UnsignedAccumulator& operator++(int)
    {
        return operator+=(1);
    }

    UnsignedAccumulator operator--()
    {
        UnsignedAccumulator tmp{*this};
        tmp -= 1;
        return tmp;
    }

    UnsignedAccumulator& operator--(int)
    {
        return operator-=(1);
    }

    UnsignedAccumulator operator+(const UnsignedAccumulator& other) const
    {
        UnsignedAccumulator tmp{};
        _::long_add_to(tmp.value_, value_, other.value_, Size);

        return tmp;
    }

    UnsignedAccumulator operator+(ValueT other) const
    {
        UnsignedAccumulator result{};
        ValueT y[Size] = {};
        y[0] = other;

        _::long_add_to(result.value_, value_, y, Size);
        return result;
    }


    UnsignedAccumulator operator-(const UnsignedAccumulator& other) const
    {
        UnsignedAccumulator tmp{};
        _::long_sub_from(tmp.value_, value_, other.value_, Size);
        return tmp;
    }


    UnsignedAccumulator operator-(ValueT other) const
    {
        UnsignedAccumulator result{};
        ValueT y[Size] = {};
        y[0] = other;

        _::long_sub_from(result.value_, value_, y, Size);
        return result;
    }


    UnsignedAccumulator& operator=(const UnsignedAccumulator& other)
    {
        std::memcpy(this->value_, other.value_, BitLength / 8);
        return *this;
    }

    UnsignedAccumulator& operator=(const ValueT& other)
    {
        std::memset(value_, 0, BitLength / 8);
        this->value_[0] = other;
        return *this;
    }

    template <unsigned BmpBitLength>
    UnsignedAccumulator operator=(const _::UAccBmpInt<BmpBitLength>& other)
    {
        static_assert(BmpBitLength <= BitLength, "");
        std::memset(value_, 0, BitLength / 8);
        _::UAccCvtHelper<ValueT>::to_acc(*this, other);
        return *this;
    }

    template <unsigned BmpBitLength>
    operator _::UAccBmpInt<BmpBitLength>() const
    {
        _::UAccBmpInt<BmpBitLength> bmp_value;
        _::UAccCvtHelper<ValueT>::to_bmp_int(*this, bmp_value);

        return bmp_value;
    }


    _::UAccBmpInt<BitLength> to_bmp() const
    {
        _::UAccBmpInt<BitLength> bmp_value;
        _::UAccCvtHelper<ValueT>::to_bmp_int(*this, bmp_value);
        return bmp_value;
    }

private:
    template <typename T, size_t N> friend struct _::UAccCvtHelper;

    template <unsigned BmpBitLength>
    void from_same_limb_cppint(const _::UAccBmpInt<BmpBitLength>& bmp_value)
    {
        auto size = bmp_value.backend().size();
        const auto* limbs = bmp_value.backend().limbs();

        for (size_t c = 0; c < size; c++)
        {
            value_[c] = limbs[c];
        }
    }


    template <unsigned BmpBitLength>
    void to_same_limb_cppint(_::UAccBmpInt<BmpBitLength>& bmp_value) const
    {
        bmp_value.backend().resize(Size, Size);
        auto size = bmp_value.backend().size();
        auto* limbs = bmp_value.backend().limbs();

        for (size_t c = 0; c < size; c++)
        {
            limbs[c] = value_[c];
        }

        bmp_value.backend().normalize();
    }


    template <unsigned BmpBitLength>
    void to_smaller_limb_cppint(_::UAccBmpInt<BmpBitLength>& bmp_value) const
    {
        bmp_value.backend().resize(Size * 2, Size * 2);

        unsigned size = bmp_value.backend().size();
        auto* limbs = bmp_value.backend().limbs();

        for (unsigned c = 0; c < size; c += 2)
        {
#ifdef BOOST_LITTLE_ENDIAN
            limbs[c]     = value_[c / 2] & 0xFFFFFFFFull;
            limbs[c + 1] = (value_[c / 2] >> 32) & 0xFFFFFFFFull;
#else
            limbs[c + 1] = value_[c / 2] & 0xFFFFFFFFull;
            limbs[c]     = (value_[c / 2] >> 32) & 0xFFFFFFFFull;
#endif
        }

        bmp_value.backend().normalize();
    }


    template <unsigned BmpBitLength>
    void from_smaller_limb_cppint(const _::UAccBmpInt<BmpBitLength>& bmp_value)
    {
        unsigned size = bmp_value.backend().size();
        const auto* limbs = bmp_value.backend().limbs();

        for (size_t c = 0; c < (size & 0xFFFFFFFE); c += 2)
        {
            ValueT v0 = limbs[c];
            ValueT v1 = limbs[c + 1];

#ifdef BOOST_LITTLE_ENDIAN
            value_[c / 2] = v1 << 32 | v0;
#else
            value_[c / 2] = v0 << 32 | v1;
#endif
        }

        if (size & 0xFFFFFFFE)
        {
#ifdef BOOST_LITTLE_ENDIAN
            value_[size / 2] = limbs[size - 1];
#else
            value_[size / 2] = ((ValueT)limbs[size - 1]) << 32;
#endif
        }
    }
};

template <size_t BitLength>
std::ostream& operator<<(std::ostream& out, const UnsignedAccumulator<BitLength>& other) {
    out << other.to_bmp();
    return out;
}

template <size_t BitLength>
std::istream& operator>>(std::istream& in, UnsignedAccumulator<BitLength>& other) {
    _::UAccBmpInt<BitLength> bmp_int;
    in >> bmp_int;
    other = bmp_int;
    return in;
}


}}

