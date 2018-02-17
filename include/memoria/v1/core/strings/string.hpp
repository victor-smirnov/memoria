
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/config.hpp>

#include <memoria/v1/core/strings/string_buffer.hpp>

#include <memoria/v1/core/strings/u8_string.hpp>
#include <memoria/v1/core/strings/u16_string.hpp>
#include <memoria/v1/core/strings/u32_string.hpp>
#include <memoria/v1/core/strings/uw_string.hpp>

#include <string>
#include <sstream>

#include <unicode/ustring.h>

namespace memoria {
namespace v1 {

using StdString    = std::string;
using StdStringRef = const StdString&;

template <typename T> struct TypeHash;

template <>
struct TypeHash<StdString> {
    static const uint64_t Value = 65;
};

inline bool compare_gt(const StdString& first, const StdString& second) {
    return first.compare(second) > 0;
}


inline bool compare_eq(const StdString& first, const StdString& second) {
    return first.compare(second) == 0;
}


inline bool compare_lt(const StdString& first, const StdString& second) {
    return first.compare(second) > 0;
}


inline bool compare_ge(const StdString& first, const StdString& second) {
    return first.compare(second) >= 0;
}


inline bool compare_le(const StdString& first, const StdString& second) {
    return first.compare(second) <= 0;
}


}}
