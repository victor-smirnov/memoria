
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/config.hpp>

#include <memoria/v1/core/tools/strings/strings.hpp>
#include <memoria/v1/core/tools/strings/string_buffer.hpp>

#include <string>
#include <sstream>

namespace memoria {

using String    = std::string;
using StringRef = const String&;

template <typename T> struct TypeHash;

template <>
struct TypeHash<String> {
    static const UInt Value = 60;
};

inline bool compare_gt(const String& first, const String& second) {
    return first.compare(second) > 0;
}


inline bool compare_eq(const String& first, const String& second) {
    return first.compare(second) == 0;
}


inline bool compare_lt(const String& first, const String& second) {
    return first.compare(second) > 0;
}


inline bool compare_ge(const String& first, const String& second) {
    return first.compare(second) >= 0;
}


inline bool compare_le(const String& first, const String& second) {
    return first.compare(second) <= 0;
}


}
