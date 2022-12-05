
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/datatypes/core.hpp>

#include <type_traits>
#include <iostream>



namespace memoria {


template <typename T, ViewKind Kind>
class NumberView;

template <typename T>
using ByValueNumberView = NumberView<T, ViewKind::BY_VALUE>;

template <typename T>
using ByRefNumberView = NumberView<T, ViewKind::BY_REF>;

template <typename T>
using ByValueNumberPtr = ViewPtr<NumberView<T, ViewKind::BY_VALUE>, VIEW_KIND_NON_HOLDING>;

template <typename T>
using ByRefNumberPtr = ViewPtr<NumberView<T, ViewKind::BY_REF>, VIEW_KIND_HOLDING>;

template <typename T>
class NumberView<T, ViewKind::BY_VALUE> {
    T value_;
public:
    NumberView():
        value_()
    {}

    NumberView(const T& value):
        value_(value)
    {}

    NumberView& operator=(const NumberView& other) {
        value_ = other.value_;
        return *this;
    }

    template <typename U>
    NumberView& operator=(const NumberView<U, ViewKind::BY_VALUE>& other) {
        value_ = other.value_;
        return *this;
    }

    template <typename U>
    NumberView& operator=(const U& other) {
        value_ = other;
        return *this;
    }

    bool operator==(const NumberView& other) const {
        return value_ == other.value_;
    }

    template <typename U>
    bool operator==(const NumberView<U, ViewKind::BY_VALUE>& other) const {
        return value_ == other.value_;
    }

    template <typename U>
    bool operator==(const U& other) const {
        return value_ == other;
    }

    friend void swap(NumberView& left, NumberView& right) {
        std::swap(left.value_, right.value_);
    }

    T* ptr() {
        return &value_;
    }

    const T* ptr() const {
        return &value_;
    }

    operator T() const {
        return value_;
    }

    operator const T&() const {
        return value_;
    }

    operator T&() {
        return value_;
    }
};

template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() + std::declval<U>())> operator+(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() + std::declval<U>())>(
        *lhs.ptr() + *rhs.ptr()
    );
}

template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() - std::declval<U>())> operator-(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() - std::declval<U>())>(
        *lhs.ptr() - *rhs.ptr()
    );
}

template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() * std::declval<U>())> operator*(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() * std::declval<U>())>(
        *lhs.ptr() * *rhs.ptr()
    );
}

template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() / std::declval<U>())> operator/(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() / std::declval<U>())>(
        *lhs.ptr() / *rhs.ptr()
    );
}

template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() % std::declval<U>())> operator%(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() % std::declval<U>())>(
        *lhs.ptr() % *rhs.ptr()
    );
}


template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() ^ std::declval<U>())> operator^(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() ^ std::declval<U>())>(
        *lhs.ptr() ^ *rhs.ptr()
    );
}


template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() & std::declval<U>())> operator&(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() & std::declval<U>())>(
        *lhs.ptr() & *rhs.ptr()
    );
}

template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() | std::declval<U>())> operator|(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() | std::declval<U>())>(
        *lhs.ptr() & *rhs.ptr()
    );
}

template <typename T, typename U>
ByValueNumberPtr<decltype(~std::declval<T>())> operator~(
        const ByValueNumberPtr<T>& lhs
)
{
    return ByValueNumberPtr<decltype(!std::declval<T>())>(
        ~(*lhs.ptr())
    );
}


template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() << std::declval<U>())> operator<<(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() << std::declval<U>())>(
        *lhs.ptr() << *rhs.ptr()
    );
}


template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() >> std::declval<U>())> operator>>(
    const ByValueNumberPtr<T>& lhs,
    const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() << std::declval<U>())>(
        *lhs.ptr() >> *rhs.ptr()
    );
}




template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() && std::declval<U>())> operator&&(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() && std::declval<U>())>(
        *lhs.ptr() && *rhs.ptr()
    );
}

template <typename T, typename U>
ByValueNumberPtr<decltype(std::declval<T>() || std::declval<U>())> operator||(
        const ByValueNumberPtr<T>& lhs,
        const ByValueNumberPtr<U>& rhs
)
{
    return ByValueNumberPtr<decltype(std::declval<T>() || std::declval<U>())>(
        *lhs.ptr() || *rhs.ptr()
    );
}


template <typename T, typename U>
ByValueNumberPtr<decltype(!std::declval<T>())> operator!(
        const ByValueNumberPtr<T>& lhs
)
{
    return ByValueNumberPtr<decltype(!std::declval<T>())>(
        !*lhs.ptr()
    );
}




template <typename T>
class NumberView<T, ViewKind::BY_REF>: public HoldingView<NumberView<T, ViewKind::BY_REF>> {
    using Base = HoldingView<NumberView<T, ViewKind::BY_REF>>;
    T* value_;
public:
    NumberView():
        value_()
    {}

    NumberView(const T& value):
        value_(value)
    {}

    NumberView& operator=(const NumberView& value) noexcept;

    T* ptr() {
        return value_;
    }

    const T* ptr() const {
        return value_;
    }
};


template <typename T>
NumberView<T, ViewKind::BY_REF>& NumberView<T, ViewKind::BY_REF>::operator=(
        const NumberView<T, ViewKind::BY_REF>& other
) noexcept {
    Base::operator=(other);
    this->value_ = other.value_;
    return *this;
}

template <>
struct ViewToDTMapping<uint8_t, UTinyInt> {};

template <>
struct ViewToDTMapping<ByValueNumberView<uint8_t>, UTinyInt> {};



template <>
struct ViewToDTMapping<int8_t, TinyInt> {};

template <>
struct ViewToDTMapping<ByValueNumberView<int8_t>, TinyInt> {};



template <>
struct ViewToDTMapping<uint16_t, USmallInt> {};

template <>
struct ViewToDTMapping<ByValueNumberView<uint16_t>, UTinyInt> {};



template <>
struct ViewToDTMapping<int16_t, SmallInt> {};

template <>
struct ViewToDTMapping<ByValueNumberView<int16_t>, SmallInt> {};


template <>
struct ViewToDTMapping<uint32_t, UInteger> {};

template <>
struct ViewToDTMapping<ByValueNumberView<uint32_t>, UInteger> {};


template <>
struct ViewToDTMapping<int32_t, Integer> {};

template <>
struct ViewToDTMapping<ByValueNumberView<int32_t>, Integer> {};


template <>
struct ViewToDTMapping<uint64_t, UBigInt> {};

template <>
struct ViewToDTMapping<ByValueNumberView<uint64_t>, UBigInt> {};


template <>
struct ViewToDTMapping<double, Double> {};

template <>
struct ViewToDTMapping<ByValueNumberView<double>, Double> {};


template <>
struct ViewToDTMapping<float, Real> {};

template <>
struct ViewToDTMapping<ByValueNumberView<float>, Real> {};


}

namespace fmt {

template <typename T, memoria::ViewKind Kind>
struct formatter<memoria::NumberView<T, Kind>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::NumberView<T, Kind>& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", *d.ptr());
    }
};

template <typename T, memoria::ViewKind Kind, size_t PtrT>
struct formatter<memoria::ViewPtr<memoria::NumberView<T, Kind>, PtrT>> {
    using ArgT = memoria::ViewPtr<memoria::NumberView<T, Kind>, PtrT>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ArgT& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", *d.ptr());
    }
};

}
