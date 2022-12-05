
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
#include <memoria/core/strings/u8_string.hpp>

#include <memoria/core/datatypes/core.hpp>

#include <type_traits>
#include <iostream>



namespace memoria {

class U8StringOView;

template<>
class HoldingView<U8StringOView>: public U8StringView {
    using Base = U8StringView;
protected:
    mutable ViewPtrHolder* ptr_holder_;

    template <typename, size_t>
    friend class ViewPtr;

public:
    HoldingView() noexcept:
        ptr_holder_()
    {
    }

    HoldingView(ViewPtrHolder* holder, Base base) noexcept:
        Base(base),
        ptr_holder_(holder)
    {}

protected:
    ViewPtrHolder* get_ptr_holder() const noexcept {
        return ptr_holder_;
    }

    void reset_ptr_holder() noexcept {
        ptr_holder_ = nullptr;
    }
};

class U8StringOView: public HoldingView<U8StringOView> {
    using Base = HoldingView<U8StringOView>;
    using StringV = U8StringView;
    using PH = ViewPtrHolder*;

public:
    U8StringOView() noexcept:
        Base()
    {}

    U8StringOView(PH ph, const value_type* str) noexcept:
        Base(ph, StringV(str))
    {}

    U8StringOView(PH ph, const value_type* str, size_t size) noexcept:
        Base(ph, StringV(str, size))
    {}

    U8StringOView(const U8StringOView& other) noexcept :
        Base(other.ptr_holder_, *static_cast<const StringV*>(&other))
    {}

    U8StringOView(U8StringOView&& other) noexcept:
        Base(other.ptr_holder_, std::move(*static_cast<StringV*>(&other)))
    {}

    U8StringOView(PH ph, const std::string& str) noexcept:
        Base(ph, StringV(str.data(), str.size()))
    {}

    U8StringOView(PH ph, boost::string_view view) noexcept:
        Base(ph, StringV(view.data(), view.size()))
    {}

    U8StringOView(PH ph, std::string_view view) noexcept:
        Base(ph, StringV(view.data(), view.size()))
    {}

    const U8StringView* ptr() const {
        return this;
    }
};

template <>
struct ViewToDTMapping<U8StringView, Varchar> {};

template <>
struct ViewToDTMapping<U8StringOView, Varchar> {};

}

namespace fmt {

template <>
struct formatter<memoria::U8StringOView> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::U8StringOView& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_string());
    }
};


template <size_t PtrT>
struct formatter<memoria::ViewPtr<memoria::U8StringOView, PtrT>> {
    using ArgT = memoria::ViewPtr<memoria::U8StringOView, PtrT>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ArgT& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_string());
    }
};

}
