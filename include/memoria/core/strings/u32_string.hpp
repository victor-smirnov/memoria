
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

#include <boost/utility/string_view.hpp>

#include <string>
#include <sstream>

#include <unicode/ustring.h>
#include "u_string_capi.hpp"

namespace memoria {

using U32StringView = boost::u32string_view;

class U8String;
class U16String;
class UWString;

class U32String {
public:
    using CharT = char32_t;
private:
    using ContentT = std::basic_string<CharT>;

    ContentT content_;

    friend class U8String;
    friend class U16String;

    template <typename T1, typename T2>
    friend std::basic_ostream<T1, T2>& operator<<(std::basic_ostream<T1, T2>&, const U32String&);


    friend void swap(U32String&, U32String&) noexcept;

public:
    U32String() = default;
    U32String(const U32String&) = default;
    U32String(U32String&&) = default;

    U32String(const CharT* data): content_(data){}
    U32String(const CharT* data, size_t size): content_(data, size){}

    U32String(const ContentT& other): content_(other) {}
    U32String(ContentT&& other): content_(std::move(other)) {}

    explicit U32String(const U16String& other);

    bool operator==(const U32String& other) const {
        return content_ == other.content_;
    }

    U32String& operator=(const U32String& other) {
        content_ = other.content_;
        return *this;
    }

    U32String& operator=(U32String&& other) {
        content_ = std::move(other.content_);
        return *this;
    }

    U32String& operator+=(const U32String& other)
    {
        content_ += other.content_;
        return *this;
    }

    U32String operator+(const U32String& other) const
    {
        return U32String(content_ + other.content_);
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

    CharT code_point_at(size_t idx) const {
        return content_[idx];
    }

    size_t size() const {
        return content_.size();
    }

    size_t code_points() const {
        return content_.size();
    }

    int32_t compare(const U32String& other) const {
        return content_.compare(other.content_);
    }

    U16String to_u16() const;
    U8String to_u8() const;
    UWString to_uwstring() const;

    ContentT& to_std_string() {
        return content_;
    }

    const ContentT& to_std_string() const {
        return content_;
    }

    CharT* data() {
        return &content_[0];
    }

    const CharT* data() const {
        return content_.data();
    }
};

template <typename CharTraits>
inline std::basic_ostream<char, CharTraits>& operator<<(std::basic_ostream<char, CharTraits>& out, const U32String& str)
{
    out << str.to_u8();
    return out;
}



using U32StringRef = const U32String&;


inline bool compare_gt(const U32String& first, const U32String& second) {
    return first.compare(second) > 0;
}


inline bool compare_eq(const U32String& first, const U32String& second) {
    return first.compare(second) == 0;
}


inline bool compare_lt(const U32String& first, const U32String& second) {
    return first.compare(second) > 0;
}


inline bool compare_ge(const U32String& first, const U32String& second) {
    return first.compare(second) >= 0;
}


inline bool compare_le(const U32String& first, const U32String& second) {
    return first.compare(second) <= 0;
}


inline void swap(memoria::U32String& one, memoria::U32String& two) noexcept {
    std::swap(one.to_std_string(), two.to_std_string());
}



}

namespace std {


template <>
struct hash<memoria::U32String> {
    size_t operator()(const memoria::U32String& str) const noexcept {
        return hash<std::u32string>()(str.to_std_string());
    }
};

}
