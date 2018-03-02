
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

#include <string>
#include <ostream>

#include <unicode/ustring.h>
#include "u_string_capi.hpp"

namespace memoria {
namespace v1 {

class U16String;
class U8String;
class U32String;

class UWString {
public:
    using CharT = wchar_t;
private:
    using ContentT = std::basic_string<CharT>;

    ContentT content_;


    friend class U16String;

    template <typename T2>
    friend std::basic_ostream<wchar_t, T2>& operator<<(std::basic_ostream<wchar_t, T2>&, const UWString&);


public:
    UWString() = default;
    UWString(const UWString&) = default;
    UWString(UWString&&) = default;

    UWString(const wchar_t* data): content_(data){}
    UWString(const wchar_t* data, size_t size): content_(data, size){}

    UWString(const ContentT& other): content_(other) {}
    UWString(ContentT&& other): content_(std::move(other)) {}

    explicit UWString(const U16String& other);

    bool operator==(const UWString& other) const {
        return content_ == other.content_;
    }

    UWString& operator=(const UWString& other) {
        content_ = other.content_;
        return *this;
    }

    UWString& operator=(UWString&& other) {
        content_ = std::move(other.content_);
        return *this;
    }

    UWString& operator+=(const UWString& other)
    {
        content_ += other.content_;
        return *this;
    }

    UWString operator+(const UWString& other) const
    {
        return UWString(content_ + other.content_);
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

    U16String to_u16() const;
    U8String  to_u8() const;
    U32String to_u32() const;

	const CharT* data() const { 
		return content_.data();
	}

	CharT* data() {
		return &content_[0];
	}

	ContentT& to_std_string() { return content_; }
	const ContentT& to_std_string() const { return content_; }
};

template <typename CharTraits>
inline std::basic_ostream<char, CharTraits>& operator<<(std::basic_ostream<char, CharTraits>& out, const UWString& str)
{
    out << str.to_u8();
    return out;
}

template <typename CharTraits>
inline std::basic_ostream<wchar_t, CharTraits>& operator<<(std::basic_ostream<wchar_t, CharTraits>& out, const UWString& str)
{
    out << str.content_;
    return out;
}


using UWStringRef = const UWString&;



}}
