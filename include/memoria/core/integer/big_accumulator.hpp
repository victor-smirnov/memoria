
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

#include <memoria/core/integer/accumulator_common.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <boost/multiprecision/integer.hpp>
#include <boost/predef/other/endian.h>

#include <ostream>

namespace memoria {

namespace bmp = boost::multiprecision;

namespace _ {

    template <typename IntT>
    using LimbType = std::remove_pointer_t<std::decay_t<decltype(std::declval<IntT>().backend().limbs())>>;

    template <typename T, typename IntT, size_t N = sizeof(T) / sizeof(LimbType<IntT>)> struct UAccCvtHelper;

    template <typename T, typename IntT>
    struct UAccCvtHelper<T, IntT, 0>
    {
        template <typename UAcc, typename BmpInt>
        static constexpr void to_acc(UAcc& acc, const BmpInt& value) {
            acc.from_larger_limb_cppint(value);
        }

        template <typename UAcc, typename BmpInt>
        static constexpr void to_bmp_int(const UAcc& acc, BmpInt& value) {
            acc.to_larger_limb_cppint(value);
        }
    };


    template <typename T, typename IntT>
    struct UAccCvtHelper<T, IntT, 1>
    {
        template <typename UAcc, typename BmpInt>
        static constexpr void to_acc(UAcc& acc, const BmpInt& value) {
            acc.from_same_limb_cppint(value);
        }

        template <typename UAcc, typename BmpInt>
        static constexpr void to_bmp_int(const UAcc& acc, BmpInt& value) {
            acc.to_same_limb_cppint(value);
        }
    };

    template <typename T, typename IntT>
    struct UAccCvtHelper<T, IntT, 2>
    {
        template <typename UAcc, typename BmpInt>
        static constexpr void to_acc(UAcc& acc, const BmpInt& value) {
            acc.from_smaller_limb_cppint(value);
        }

        template <typename UAcc, typename BmpInt>
        static constexpr void to_bmp_int(const UAcc& acc, BmpInt& value) {
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

    static constexpr size_t ByteSize = Size * sizeof (ValueT);

    static constexpr size_t AccBitLength = BitLength;

    ValueT value_[Size];

    constexpr UnsignedAccumulator(): value_{} {}

    constexpr UnsignedAccumulator(const UnsignedAccumulator& other) = default;

    template <unsigned BmpBitLength>
    constexpr UnsignedAccumulator(
            const _::UAccBmpInt<BmpBitLength>& bmp_value
    ): value_{}
    {
        static_assert(BitLength >= BmpBitLength, "");

        _::UAccCvtHelper<ValueT, _::UAccBmpInt<BmpBitLength>>::to_acc(*this, bmp_value);
    }

    constexpr UnsignedAccumulator(ValueT v): value_{} {
        value_[0] = v;
    }

    UnsignedAccumulator(const std::string& digits):
        UnsignedAccumulator(_::UAccBmpInt<BitLength>(digits))
    {}

    template <size_t OtherBitLength>
    constexpr UnsignedAccumulator(const UnsignedAccumulator<OtherBitLength>& other):value_{}
    {
        static_assert(BitLength >= OtherBitLength, "");
        for (size_t c = 0; c < other.Size; c++)
        {
            value_[c] = other.value_[c];
        }
    }


    constexpr UnsignedAccumulator(const UUID& uuid): value_{}
    {
        static_assert(BitLength >= 128, "");
        value_[0] = uuid.lo();
        value_[1] = uuid.hi();
    }

    UUID to_uuid() const
    {
        static_assert (BitLength >= 128, "");
        return UUID(value_[1], value_[0]);
    }

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

    MMA_NODISCARD bool operator==(const UnsignedAccumulator& other) const
    {
        for (size_t c = 0; c < Size; c++) {
            if (value_[c] != other.value_[c]) {
                return false;
            }
        }

        return true;
    }

    template <unsigned BmpBitLength>
    MMA_NODISCARD bool operator==(const _::UAccBmpInt<BmpBitLength>& other) const
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


    UnsignedAccumulator& operator=(const UnsignedAccumulator& other) = default;

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
        _::UAccCvtHelper<ValueT, _::UAccBmpInt<BmpBitLength>>::to_acc(*this, other);
        return *this;
    }

    template <unsigned BmpBitLength>
    operator _::UAccBmpInt<BmpBitLength>() const
    {
        _::UAccBmpInt<BmpBitLength> bmp_value;
        _::UAccCvtHelper<ValueT, _::UAccBmpInt<BmpBitLength>>::to_bmp_int(*this, bmp_value);

        return bmp_value;
    }


    _::UAccBmpInt<BitLength> to_bmp() const
    {
        _::UAccBmpInt<BitLength> bmp_value;
        _::UAccCvtHelper<ValueT, _::UAccBmpInt<BitLength>>::to_bmp_int(*this, bmp_value);
        return bmp_value;
    }

    template <size_t TgtBitLength>
    std::enable_if_t<TgtBitLength >= BitLength, UnsignedAccumulator<TgtBitLength>> cast_to() const
    {
        UnsignedAccumulator<TgtBitLength> tgt{};

        for (size_t c = 0; c < Size; c++)
        {
            tgt.value_[c] = value_[c];
        }

        return tgt;
    }

    template <size_t TgtBitLength>
    std::enable_if_t<TgtBitLength < BitLength, UnsignedAccumulator<TgtBitLength>> cast_to() const
    {
        UnsignedAccumulator<TgtBitLength> tgt{};

        for (size_t c = 0; c < UnsignedAccumulator<TgtBitLength>::Size; c++)
        {
            tgt.value_[c] = value_[c];
        }

        return tgt;
    }

private:
    template <typename T, typename IntT, size_t N> friend struct _::UAccCvtHelper;

    template <unsigned BmpBitLength>
    constexpr void from_same_limb_cppint(const _::UAccBmpInt<BmpBitLength>& bmp_value)
    {
        auto size = bmp_value.backend().size();
        const auto* limbs = bmp_value.backend().limbs();

        for (size_t c = 0; c < size; c++)
        {
            value_[c] = limbs[c];
        }
    }


    template <unsigned BmpBitLength>
    constexpr void to_same_limb_cppint(_::UAccBmpInt<BmpBitLength>& bmp_value) const
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
    constexpr void to_smaller_limb_cppint(_::UAccBmpInt<BmpBitLength>& bmp_value) const
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
    constexpr void to_larger_limb_cppint(_::UAccBmpInt<BmpBitLength>& bmp_value) const
    {
        using LimbT = _::LimbType<_::UAccBmpInt<BmpBitLength>>;

        bmp_value.backend().resize(Size / 2 , Size / 2);
        auto* limbs = bmp_value.backend().limbs();

        for (unsigned c = 0; c < Size; c += 2)
        {
            LimbT v0 = value_[c];
            LimbT v1 = value_[c + 1];

#ifdef BOOST_LITTLE_ENDIAN
            limbs[c / 2] = v0 | (v1 << 64);
#else
            limbs[c / 2] = v1 | (v0 << 64);
#endif
        }

        bmp_value.backend().normalize();
    }



    template <unsigned BmpBitLength>
    constexpr void from_smaller_limb_cppint(const _::UAccBmpInt<BmpBitLength>& bmp_value)
    {
        unsigned size = bmp_value.backend().size();
        const auto* limbs = bmp_value.backend().limbs();

        for (size_t c = 0; c < size; c += 2)
        {
            ValueT v0 = limbs[c];
            ValueT v1 = limbs[c + 1];

#ifdef BOOST_LITTLE_ENDIAN
            value_[c / 2] = (v1 << 32) | v0;
#else
            value_[c / 2] = (v0 << 32) | v1;
#endif
        }

        if (size & 0x1)
        {
#ifdef BOOST_LITTLE_ENDIAN
            value_[size / 2] = limbs[size - 1];
#else
            value_[size / 2] = ((ValueT)limbs[size - 1]) << 32;
#endif
        }
    }


    template <unsigned BmpBitLength>
    constexpr void from_larger_limb_cppint(const _::UAccBmpInt<BmpBitLength>& bmp_value)
    {
        unsigned size = bmp_value.backend().size();
        const auto* limbs = bmp_value.backend().limbs();

        using LimbT = _::LimbType<_::UAccBmpInt<BmpBitLength>>;
        for (size_t c = 0; c < Size; c += 2)
        {
            LimbT vv = limbs[c / 2];

#ifdef BOOST_LITTLE_ENDIAN
            value_[c] = vv;
            value_[c + 1] = vv >> 64;
#else
            value_[c + 1] = vv;
            value_[c] = vv >> 64;
#endif
        }

        if (Size & 0x01)
        {
            value_[Size - 1] = limbs[size - 1];
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


}

namespace fmt {

template <size_t BitLength>
struct formatter<memoria::UnsignedAccumulator<BitLength>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::UnsignedAccumulator<BitLength>& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_bmp().str());
    }
};

template <size_t BitLength>
struct formatter<memoria::_::UAccBmpInt<BitLength>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::_::UAccBmpInt<BitLength>& d, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", d.str());
    }
};

template <>
struct formatter<boost::multiprecision::uint256_t> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const boost::multiprecision::uint256_t& d, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", d.str());
    }
};

template <
        unsigned MinBits,
        unsigned MaxBits,
        boost::multiprecision::cpp_integer_type SignType,
        boost::multiprecision::cpp_int_check_type Checked,
        class Allocator
>
struct formatter<
        boost::multiprecision::number<
            boost::multiprecision::cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>
        >
> {

    using IntT = boost::multiprecision::number<
        boost::multiprecision::cpp_int_backend<
            MinBits, MaxBits, SignType, Checked, Allocator
        >
    >;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const IntT& d, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", d.str());
    }
};

}
