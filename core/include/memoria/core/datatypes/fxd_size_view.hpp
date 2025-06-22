
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
class FxdSizeView;

template <typename T>
using ByValueFxdSizePtr = Own<FxdSizeView<T, ViewKind::BY_VALUE>, OwningKind::WRAPPING>;

template <typename T>
using ByRefFxdSizePtr = Own<FxdSizeView<T, ViewKind::BY_REF>, OwningKind::HOLDING>;

template <typename T>
class FxdSizeView<T, ViewKind::BY_VALUE> {
    T value_;
public:
    FxdSizeView():
        value_()
    {}

    FxdSizeView(const T& value):
        value_(value)
    {}

    FxdSizeView& operator=(const FxdSizeView& other) {
        value_ = other.value_;
        return *this;
    }

    template <typename U>
    FxdSizeView& operator=(const FxdSizeView<U, ViewKind::BY_VALUE>& other) {
        value_ = other.value_;
        return *this;
    }

    template <typename U>
    FxdSizeView& operator=(const U& other) {
        value_ = other;
        return *this;
    }

    bool operator==(const FxdSizeView& other) const {
        return value_ == other.value_;
    }

    template <typename U>
    bool operator==(const FxdSizeView<U, ViewKind::BY_VALUE>& other) const {
        return value_ == other.value_;
    }

    template <typename U>
    bool operator==(const U& other) const {
        return value_ == other;
    }

    friend void swap(FxdSizeView& left, FxdSizeView& right) {
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

    const T& value_t() const {
        return value_;
    }
};

template <>
struct ViewToDTMapping<bool>: HasType<Boolean> {};

template <>
struct ViewToDTMapping<FxdSizeView<bool, ViewKind::BY_VALUE>>: HasType<Boolean> {};


}

namespace fmt {

template <typename T, memoria::ViewKind Kind>
struct formatter<memoria::FxdSizeView<T, Kind>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::FxdSizeView<T, Kind>& d, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", *d.ptr());
    }
};


template <typename T, memoria::ViewKind Kind, memoria::OwningKind KT>
struct formatter<memoria::Own<memoria::FxdSizeView<T, Kind>, KT>> {
    using ArgT = memoria::Own<memoria::FxdSizeView<T, Kind>, KT>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ArgT& d, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", *d.ptr());
    }
};

}
