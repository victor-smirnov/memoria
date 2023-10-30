
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

namespace hermes {
class HermesCtrView;
}

template<>
class HoldingView<U8StringOView>: public U8StringView {
    using Base = U8StringView;
protected:
    mutable LWMemHolder* mem_holder_;

    template <typename, OwningKind>
    friend class Own;

    friend class hermes::HermesCtrView;

public:
    HoldingView() noexcept:
        mem_holder_()
    {
    }

    HoldingView(LWMemHolder* holder, Base base) noexcept:
        Base(base),
        mem_holder_(holder)
    {}

    HoldingView& operator=(const HoldingView& other) noexcept
    {
        Base::operator=(other);
        mem_holder_ = other.mem_holder_;
        return *this;
    }

protected:
    LWMemHolder* get_mem_holder() const noexcept {
        return mem_holder_;
    }

    void reset_mem_holder() noexcept {
        mem_holder_ = nullptr;
    }

    void set_mem_holder(LWMemHolder* mem_holder) noexcept {
        mem_holder_ = mem_holder;
    }
};


// FIXME: U8StringOView is unnecessary. Will be removed once
// LD classes are removed.

class U8StringOView: public HoldingView<U8StringOView> {
    using Base = HoldingView<U8StringOView>;
    using StringV = U8StringView;
    using PH = LWMemHolder*;

public:
    U8StringOView() noexcept:
        Base()
    {}

    U8StringOView(PH ph, U8StringView str) noexcept:
        U8StringOView(ph, str.data(), str.size())
    {}

    U8StringOView(PH ph, const value_type* str) noexcept:
        Base(ph, StringV(str))
    {}

    U8StringOView(PH ph, const value_type* str, size_t size) noexcept:
        Base(ph, StringV(str, size))
    {}

    U8StringOView(const U8StringOView& other) noexcept :
        Base(other.mem_holder_, *static_cast<const StringV*>(&other))
    {}

    U8StringOView(U8StringOView&& other) noexcept:
        Base(other.mem_holder_, std::move(*static_cast<StringV*>(&other)))
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

    U8StringOView& operator=(const U8StringOView& other) noexcept
    {
        Base::operator=(other);
        return *this;
    }

    U8StringOView& operator=(U8StringOView&& other) noexcept
    {

        Base::operator=(std::move(other));
        other.reset_mem_holder();
        return *this;
    }
};

template <>
struct ViewToDTMapping<U8StringView>: HasType<Varchar> {};

template <>
struct ViewToDTMapping<boost::string_view>: HasType<Varchar> {};

template <>
struct ViewToDTMapping<std::string_view>: HasType<Varchar> {};

template <>
struct ViewToDTMapping<std::string>: HasType<Varchar> {};

template <>
struct ViewToDTMapping<U8String>: HasType<Varchar> {};


template <>
struct ViewToDTMapping<U8StringOView>: HasType<Varchar> {};

template <OwningKind OK>
struct ViewToDTMapping<Own<U8StringOView, OK>>: HasType<Varchar> {};

template <>
struct ViewToDTMapping<const char*>: HasType<Varchar> {};

template <size_t N>
struct ViewToDTMapping<const char(&)[N]>: HasType<Varchar> {};

template <size_t N>
struct ViewToDTMapping<char[N]>: HasType<Varchar> {};

}

namespace fmt {

template <>
struct formatter<memoria::U8StringOView> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::U8StringOView& d, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", d.to_string());
    }
};

template <>
struct formatter<memoria::U8StringView> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::U8StringView& d, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", d.to_string());
    }
};


template <memoria::OwningKind KT>
struct formatter<memoria::Own<memoria::U8StringOView, KT>> {
    using ArgT = memoria::Own<memoria::U8StringOView, KT>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ArgT& d, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", d.to_string());
    }
};

}
