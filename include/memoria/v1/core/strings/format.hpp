
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/exceptions/core.hpp>

#include <memoria/v1/filesystem/path.hpp>

#include <string>
#include <sstream>
#include <memory>
#include <ostream>
#include <functional>

#include <fmt/format.h>

namespace memoria {
namespace v1 {

template <typename... Args>
U8String format_u8(const char16_t* fmt_str, Args&&... args)
{
    U16String fmt16(fmt_str);
    return ::fmt::format(fmt16.to_u8().to_std_string(), std::forward<Args>(args)...);
}

template <typename... Args>
U8String format_u8(const char* fmt_str, Args&&... args)
{
    return ::fmt::format(fmt_str, std::forward<Args>(args)...);
}

template <typename... Args>
WhatInfo format_ex(const char16_t* fmt, Args&&... args) {
    return WhatInfo(format_u8(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
WhatInfo format_ex(const char* fmt, Args&&... args) {
    return WhatInfo(format_u8(fmt, std::forward<Args>(args)...));
}


}}

namespace fmt {

template <>
struct formatter<memoria::v1::U8String> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::v1::U8String& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_std_string());
    }
};

template <>
struct formatter<memoria::v1::U8StringView> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::v1::U8StringView& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.data());
    }
};

template <>
struct formatter<memoria::v1::U16String> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::v1::U16String& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_u8().to_std_string());
    }
};


template <>
struct formatter<memoria::v1::U32String> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::v1::U32String& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_u8().to_std_string());
    }
};

template <>
struct formatter<memoria::v1::filesystem::path> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::v1::filesystem::path& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.string());
    }
};


}
