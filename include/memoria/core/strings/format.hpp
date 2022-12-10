
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

#include <memoria/core/types.hpp>

#include <memoria/core/strings/string.hpp>
#include <memoria/core/exceptions/core.hpp>
#include <memoria/filesystem/path.hpp>

#include <string>
#include <sstream>
#include <memory>
#include <iostream>
#include <functional>

#include <fmt/format.h>

namespace memoria {

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

template <typename... Args>
std::ostream& println(const char* fmt_str, Args&&... args)
{
    std::cout << format_u8(fmt_str, std::forward<Args>(args)...) << std::endl;
    return std::cout;
}

template <typename... Args>
std::ostream& print(const char* fmt_str, Args&&... args)
{
    std::cout << format_u8(fmt_str, std::forward<Args>(args)...);
    return std::cout;
}

template <typename... Args>
std::ostream& print(std::ostream& out, const char* fmt_str, Args&&... args)
{
    out << format_u8(fmt_str, std::forward<Args>(args)...);
    return out;
}

template <typename... Args>
std::ostream& println(std::ostream& out, const char* fmt_str, Args&&... args)
{
    out << format_u8(fmt_str, std::forward<Args>(args)...) << std::endl;
    return out;
}

}

namespace fmt {

template <>
struct formatter<memoria::U8String> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::U8String& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_std_string());
    }
};

template <>
struct formatter<memoria::U16String> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::U16String& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_u8().to_std_string());
    }
};


template <>
struct formatter<memoria::U32String> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::U32String& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_u8().to_std_string());
    }
};

template <>
struct formatter<memoria::filesystem::path> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::filesystem::path& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.string());
    }
};

template <typename T>
struct formatter<memoria::NamedTypedCode<T>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::NamedTypedCode<T>& d, FormatContext& ctx) {
        if (MMA_LIKELY(!d.name().empty())) {
            return format_to(ctx.out(), "<{}:{}>", d.code(), d.name().to_string());
        }
        else {
            return format_to(ctx.out(), "{}", d.code());
        }
    }
};

}
