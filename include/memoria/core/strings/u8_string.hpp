
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
#include <memoria/core/linked/common/linked_hash.hpp>

#include <boost/utility/string_view.hpp>

#include <unicode/ustring.h>
#include "u_string_capi.hpp"

#include <string>
#include <sstream>
#include <ostream>

namespace memoria {

class U16String;
class U32String;
class UWString;


using U8StringView = boost::string_view;

inline bool is_unicode_space_u8(char codeunit)
{
    switch (codeunit) {
        case '\r':
        case '\n':
        case '\t':
        case 0x20: return true;
    }

    return false;
}

class U8String {
public:
    using CharT = char;
    using value_type = CharT;
private:
    using ContentT = std::basic_string<CharT>;

    ContentT content_;

    friend class U16String;
    friend class U32String;

    template <typename T2>
    friend std::basic_ostream<char, T2>& operator<<(std::basic_ostream<char, T2>&, const U8String&);




public:
    static const size_t NPOS = ContentT::npos;

    U8String() = default;
    U8String(const U8String&) = default;
    U8String(U8String&&) = default;

    U8String(const CharT* data): content_(data){}
    U8String(const CharT* data, size_t size): content_(data, size){}

    U8String(const ContentT& other): content_(other) {}
    U8String(ContentT&& other): content_(std::move(other)) {}

    U8String(size_t size, CharT code_unit): content_(size, code_unit) {}

    U8String(U8StringView str): content_(str.data(), str.size()) {}

    explicit U8String(const U16String& other);

    bool operator==(const U8String& other) const {
        return content_ == other.content_;
    }

    bool operator!=(const U8String& other) const {
        return content_ != other.content_;
    }

    bool operator>(const U8String& other) const {
        return content_ > other.content_;
    }

    bool operator<(const U8String& other) const {
        return content_ < other.content_;
    }

    bool operator>=(const U8String& other) const {
        return content_ >= other.content_;
    }

    bool operator<=(const U8String& other) const {
        return content_ <= other.content_;
    }

    bool operator!=(const CharT* other) const {
        return content_ != other;
    }

    bool operator==(const CharT* other) const {
        return content_ == other;
    }

    bool operator>(const CharT* other) const {
        return content_ > other;
    }

    bool operator==(const U8StringView& other) const {
        return content_ == other;
    }

    bool operator<(const CharT* other) const {
        return content_ < other;
    }

    bool operator>=(const CharT* other) const {
        return content_ >= other;
    }

    bool operator<=(const CharT* other) const {
        return content_ <= other;
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

    size_t length() const {
        return content_.length();
    }

    int32_t compare(const U8String& other) const {
        return content_.compare(other.content_);
    }

    const U8String& to_u8() const noexcept {return *this;}

    U16String to_u16() const;
    U32String to_u32() const;
    UWString to_uwstring() const;

    const ContentT& to_std_string() const {
        return content_;
    }

    ContentT& to_std_string() {
        return content_;
    }

    operator ContentT&() {
        return content_;
    }

    operator const ContentT&() const {
        return content_;
    }

    operator U8StringView() const
    {
        return U8StringView(content_);
    }

    U8StringView view() const {
        return U8StringView(content_);
    }

    CharT* data() {
        return &content_[0];
    }

    const CharT* data() const {
        return content_.data();
    }

    size_t find_first_of(const U8String& str) const {
        return content_.find_first_of(str.content_);
    }

    size_t find_first_of(const char* str, size_t pos, size_t count) const {
        return content_.find_first_of(str, pos, count);
    }

    size_t find_first_of(const char* str, size_t pos = 0) const {
        return content_.find_first_of(str, pos);
    }

    size_t find_first_of(char codeunit, size_t pos = 0) const {
        return content_.find_first_of(codeunit, pos);
    }


    size_t find_first_not_of(const U8String& str) const {
        return content_.find_first_not_of(str.content_);
    }

    size_t find_first_not_of(const char* str, size_t pos, size_t count) const {
        return content_.find_first_not_of(str, pos, count);
    }

    size_t find_first_not_of(const char* str, size_t pos = 0) const {
        return content_.find_first_not_of(str, pos);
    }

    size_t find_first_not_of(char codeunit, size_t pos = 0) const {
        return content_.find_first_not_of(codeunit, pos);
    }


    size_t find_last_of(const U8String& str) const {
        return content_.find_last_of(str.content_);
    }

    size_t find_last_of(const char* str, size_t pos, size_t count) const {
        return content_.find_last_of(str, pos, count);
    }

    size_t find_last_of(const char* str, size_t pos = 0) const {
        return content_.find_last_of(str, pos);
    }

    size_t find_last_of(char codeunit, size_t pos = 0) const {
        return content_.find_last_of(codeunit, pos);
    }


    size_t find_last_not_of(const U8String& str) const {
        return content_.find_last_not_of(str.content_);
    }

    size_t find_last_not_of(const char* str, size_t pos, size_t count) const {
        return content_.find_last_not_of(str, pos, count);
    }

    size_t find_last_not_of(const char* str, size_t pos = 0) const {
        return content_.find_last_not_of(str, pos);
    }

    size_t find_last_not_of(char codeunit, size_t pos = 0) const {
        return content_.find_last_not_of(codeunit, pos);
    }

    U8String& trim()
    {
        trim_end();
        trim_start();

        return *this;
    }

    U8String trim_copy() const
    {
        U8String copy = *this;

        copy.trim_end();
        copy.trim_start();

        return copy;
    }

    U8String& trim_start()
    {
        size_t length = content_.size();
        size_t start_idx = 0;

        while (start_idx < length)
        {
            if (!is_unicode_space_u8(content_[start_idx])) {
                break;
            }
            ++start_idx;
        }

        if (start_idx > 0)
        {
            content_.erase(0, start_idx);
        }

        return *this;
    }

    U8String& trim_end()
    {
        size_t length = content_.size();
        int64_t end_idx = length - 1;

        while (end_idx >= 0)
        {
            if (!is_unicode_space_u8(content_[end_idx])) {
                break;
            }
            --end_idx;
        }

        if (end_idx < length - 1){
            content_.erase(end_idx + 1);
        }

        return *this;
    }


    size_t find(const U8String& other, size_t pos = 0) const
    {
        return content_.find(other.content_, pos);
    }

    size_t find(const CharT* other, size_t pos, size_t count) const
    {
        return content_.find(other, pos, count);
    }

    size_t find(const CharT* other, size_t pos = 0) const
    {
        return content_.find(other, pos);
    }

    size_t find(CharT codeunit, size_t pos = 0) const
    {
        return content_.find(codeunit, pos);
    }


    size_t rfind(const U8String& other, size_t pos = 0) const
    {
        return content_.rfind(other.content_, pos);
    }

    size_t rfind(const CharT* other, size_t pos, size_t count) const
    {
        return content_.rfind(other, pos, count);
    }

    size_t rfind(const CharT* other, size_t pos = 0) const
    {
        return content_.rfind(other, pos);
    }

    size_t rfind(CharT codeunit, size_t pos = 0) const
    {
        return content_.rfind(codeunit, pos);
    }

    bool contains(const U8String& other) {
        return find(other) != NPOS;
    }

    bool contains(const CharT* other) {
        return find(other) != NPOS;
    }

    bool contains(CharT other) {
        return find(other) != NPOS;
    }

    U8String substring(size_t start, size_t count = NPOS) const {
        return content_.substr(start, count);
    }

    bool starts_with(const char* str) const
    {
        size_t str_length = std::char_traits<char>::length(str);
        if (size() <= str_length)
        {
            for (size_t c = 0; c < str_length; c++) {
                if (content_[c] != str[c]) {
                    return false;
                }
            }

            return true;
        }
        else {
            return false;
        }
    }

    bool starts_with(const U8String& str) const
    {
        if (size() <= str.size())
        {
            for (size_t c = 0; c < str.size(); c++) {
                if (content_[c] != str.content_[c]) {
                    return false;
                }
            }

            return true;
        }
        else {
            return false;
        }
    }

    bool ends_with(const U8String& str) const
    {
        if (size() <= str.size())
        {
            size_t offset = size() - str.size();
            for (size_t c = 0; c < str.size(); c++) {
                if (content_[c + offset] != str.content_[c]) {
                    return false;
                }
            }

            return true;
        }
        else {
            return false;
        }
    }

    bool ends_with(const char* str) const
    {
        size_t str_length = std::char_traits<char>::length(str);
        if (size() <= str_length)
        {
            size_t offset = size() - str_length;
            for (size_t c = 0; c < str_length; c++) {
                if (content_[c + offset] != str[c]) {
                    return false;
                }
            }

            return true;
        }
        else {
            return false;
        }
    }

    bool is_empty() const {
        return size() == 0;
    }

};


template <typename CharTraits>
inline std::basic_ostream<char, CharTraits>& operator<<(std::basic_ostream<char, CharTraits>& out, const U8String& str)
{
    out << str.content_;
    return out;
}

using U8StringRef = const U8String&;

inline bool compare_gt(const U8StringView& first, const U8StringView& second) {
    return first.compare(second) > 0;
}


inline bool compare_eq(const U8StringView& first, const U8StringView& second) {
    return first.compare(second) == 0;
}


inline bool compare_lt(const U8StringView& first, const U8StringView& second) {
    return first.compare(second) > 0;
}


inline bool compare_ge(const U8StringView& first, const U8StringView& second) {
    return first.compare(second) >= 0;
}


inline bool compare_le(const U8StringView& first, const U8StringView& second) {
    return first.compare(second) <= 0;
}

U8StringView trim_string(U8StringView str);


template <size_t Size, typename Variant>
void append(FNVHasher<Size, Variant>& hasher, const U8String& str)
{
    append(hasher, str.view());
}


inline void swap(memoria::U8String& one, memoria::U8String& two) noexcept {
    std::swap(one.to_std_string(), two.to_std_string());
}

}



namespace std {

template <>
struct hash<memoria::U8String> {
    size_t operator()(const memoria::U8String& str) const noexcept {
        return hash<std::string>()(str.to_std_string());
    }
};

}
