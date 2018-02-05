
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/config.hpp>

#include <memoria/v1/core/tools/strings/strings.hpp>
#include <memoria/v1/core/tools/strings/string_buffer.hpp>

#include <string>
#include <sstream>

#include <unicode/ustring.h>
#include "u_string_capi.hpp"

namespace memoria {
namespace v1 {

class U16String;
class U32String;
class UWString;


class U8String {
public:
    using CharT = char;
private:
    using ContentT = std::basic_string<CharT>;

    ContentT content_;

    friend class U16String;
    friend class U32String;

    template <typename T2>
    friend std::basic_ostream<char, T2>& operator<<(std::basic_ostream<char, T2>&, const U8String&);

public:
    U8String() = default;
    U8String(const U8String&) = default;
    U8String(U8String&&) = default;

    U8String(const CharT* data): content_(data){}
    U8String(const CharT* data, size_t size): content_(data, size){}

    U8String(const ContentT& other): content_(other) {}
    U8String(ContentT&& other): content_(std::move(other)) {}

    explicit U8String(const U16String& other);

    bool operator==(const U8String& other) const {
        return content_ == other.content_;
    }

    U8String& operator=(const U8String& other) {
        content_ = other.content_;
        return *this;
    }

    U8String& operator=(U8String&& other) {
        content_ = std::move(other.content_);
        return *this;
    }

    U8String& operator+=(const U8String& other)
    {
        content_ += other.content_;
        return *this;
    }

    U8String operator+(const U8String& other) const
    {
        return U8String(content_ + other.content_);
    }

    CharT& operator[](size_t idx) {
        return content_[idx];
    }

    const CharT& operator[](size_t idx) const {
        return content_[idx];
    }

    CharT& code_unit_at(size_t idx) {
        return content_[idx];
    }

    const CharT& code_unit_at(size_t idx) const {
        return content_[idx];
    }

    size_t size() const {
        return content_.size();
    }

    int32_t compare(const U8String& other) const {
        return content_.compare(other.content_);
    }

    U16String to_u16() const;
    U32String to_u32() const;
    UWString to_uwstring() const;

    CharT* data() {
        return &content_[0];
    }

    const CharT* data() const {
        return content_.data();
    }
};


template <typename CharTraits>
inline std::basic_ostream<char, CharTraits>& operator<<(std::basic_ostream<char, CharTraits>& out, const U8String& str)
{
    out << str.content_;
    return out;
}


using U8StringRef = const U8String&;

template <typename T> struct TypeHash;

template <>
struct TypeHash<U8String> {
    static const uint64_t Value = 61;
};

inline bool compare_gt(const U8String& first, const U8String& second) {
    return first.compare(second) > 0;
}


inline bool compare_eq(const U8String& first, const U8String& second) {
    return first.compare(second) == 0;
}


inline bool compare_lt(const U8String& first, const U8String& second) {
    return first.compare(second) > 0;
}


inline bool compare_ge(const U8String& first, const U8String& second) {
    return first.compare(second) >= 0;
}


inline bool compare_le(const U8String& first, const U8String& second) {
    return first.compare(second) <= 0;
}


}}

