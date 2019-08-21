
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
#include <memory>
#include <ostream>

#include <unicode/ustring.h>
#include <unicode/unistr.h>
#include <unicode/utext.h>
#include "u_string_capi.hpp"

namespace memoria {
namespace v1 {

using UnicodeString = MMA1_ICU_CXX_NS::UnicodeString;

class U8String;
class U32String;
class UWString;

inline bool is_unicode_space(char16_t codeunit)
{
    switch (codeunit) {
        case u'\u0020':
        case u'\u00a0':
        case u'\u1680':
        case u'\u2000':
        case u'\u202f':
        case u'\u2050':
        case u'\u3000':
        case u'\ufeff': return true;
    }

    if (codeunit > u'\u2000' && codeunit <= u'\u200b') {
        return true;
    }

    return false;
}

class U16String {
public:
    using CharT = char16_t; 
private:
    using ContentT = std::basic_string<CharT>;

    ContentT content_;

    friend class U8String;
    friend class U32String;
    friend class UWString;

    template <typename T1, typename T2>
    friend std::basic_ostream<T1, T2>& operator<<(std::basic_ostream<T1, T2>&, const U16String&);

    template <typename T>
    friend void std::swap(T&, T&) noexcept;

public:
    static const size_t NPOS = ContentT::npos;

    U16String() = default;
    U16String(const U16String&) = default;
    U16String(U16String&&) = default;

    U16String(size_t size, char16_t fill_cu): content_(size, fill_cu) {}

    U16String(const char* data): content_((size_t)UStrUTF8Length(data), (CharT)0)
    {
        UErrorCode error_code{};
        u_strFromUTF8(castChars(&content_[0]), (int32_t)content_.size() + 1, nullptr, data, -1, &error_code);
        StringException::assertOk(error_code);
    }

    U16String(const char* data, size_t size): content_((size_t)UStrUTF8Length(data, size), (CharT)0)
    {
        UErrorCode error_code{};
        u_strFromUTF8(castChars(&content_[0]), (int32_t)content_.size() + 1, nullptr, data, size, &error_code);
        StringException::assertOk(error_code);
    }

    U16String(const char16_t* data): content_(data){}
    U16String(const char16_t* data, size_t size): content_(data, size){}

    U16String(const ContentT& other): content_(other) {}
    U16String(ContentT&& other): content_(std::move(other)) {}

    U16String(const UnicodeString& icu_string):
        content_(T2T<const char16_t*>(icu_string.getBuffer()), icu_string.length())
    {}

    explicit U16String(const U8String& other);
    explicit U16String(const U32String& other);
    explicit U16String(const UWString& other);

    bool operator==(const U16String& other) const {
        return content_ == other.content_;
    }

    bool operator!=(const U16String& other) const {
        return content_ != other.content_;
    }

    bool operator!=(const char16_t* other) const {
        return content_ != other;
    }

    bool operator==(const char16_t* other) const {
        return content_ == other;
    }

    bool operator>=(const char16_t* other) const {
        return content_ >= other;
    }

    bool operator<=(const char16_t* other) const {
        return content_ <= other;
    }

    bool operator<(const char16_t* other) const {
        return content_ < other;
    }

    bool operator>(const char16_t* other) const {
        return content_ > other;
    }


    bool operator<(const U16String& other) const {
        return content_ < other.content_;
    }

    bool operator<=(const U16String& other) const {
        return content_ <= other.content_;
    }

    bool operator>(const U16String& other) const {
        return content_ > other.content_;
    }

    bool operator>=(const U16String& other) const {
        return content_ >= other.content_;
    }

    U16String& operator=(const U16String& other) {
        content_ = other.content_;
        return *this;
    }

    U16String& operator=(U16String&& other) {
        content_ = std::move(other.content_);
        return *this;
    }

    U16String& operator+=(const U16String& other)
    {
        content_ += other.content_;
        return *this;
    }

    U16String operator+(const U16String& other) const
    {
        return U16String(content_ + other.content_);
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


    size_t code_points() const {
        return u_countChar32(castChars(content_.data()), content_.size());
    }

    int32_t compare(const U16String& other) const {
        return u_strCompare(castChars(content_.data()), content_.size(), castChars(other.content_.data()), other.content_.size(), true);
    }

    U16String& trim()
    {
        trim_end();
        trim_start();

        return *this;
    }

    U16String trim_copy() const
    {
        U16String copy = *this;

        copy.trim_end();
        copy.trim_start();

        return copy;
    }

    U16String& trim_start()
    {
        size_t length = content_.size();
        size_t start_idx = 0;

        while (start_idx < length)
        {
            if (!is_unicode_space(content_[start_idx])) {
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

    U16String& trim_end()
    {
        size_t length = content_.size();
        int64_t end_idx = length - 1;

        while (end_idx >= 0)
        {
            if (!is_unicode_space(content_[end_idx])) {
                break;
            }
            --end_idx;
        }

        if (end_idx < length - 1){
            content_.erase(end_idx + 1);
        }

        return *this;
    }


    size_t find(const U16String& other, size_t pos = 0) const
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


    size_t rfind(const U16String& other, size_t pos = 0) const
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

    bool contains(const U16String& other) {
        return find(other) != NPOS;
    }

    bool contains(const CharT* other) {
        return find(other) != NPOS;
    }

    bool contains(CharT other) {
        return find(other) != NPOS;
    }

    U16String substring(size_t start, size_t count = NPOS) const {
        return content_.substr(start, count);
    }

    bool starts_with(const char16_t* str) const
    {
        size_t str_length = std::char_traits<char16_t>::length(str);
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

    bool starts_with(const U16String& str) const
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

    bool ends_with(const U16String& str) const
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

    bool ends_with(const char16_t* str) const
    {
        size_t str_length = std::char_traits<char16_t>::length(str);
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

    size_t find_first_of(const U16String& str) const {
        return content_.find_first_of(str.content_);
    }

    size_t find_first_of(const char16_t* str, size_t pos, size_t count) const {
        return content_.find_first_of(str, pos, count);
    }

    size_t find_first_of(const char16_t* str, size_t pos = 0) const {
        return content_.find_first_of(str, pos);
    }

    size_t find_first_of(char16_t codeunit, size_t pos = 0) const {
        return content_.find_first_of(codeunit, pos);
    }


    size_t find_first_not_of(const U16String& str) const {
        return content_.find_first_not_of(str.content_);
    }

    size_t find_first_not_of(const char16_t* str, size_t pos, size_t count) const {
        return content_.find_first_not_of(str, pos, count);
    }

    size_t find_first_not_of(const char16_t* str, size_t pos = 0) const {
        return content_.find_first_not_of(str, pos);
    }

    size_t find_first_not_of(char16_t codeunit, size_t pos = 0) const {
        return content_.find_first_not_of(codeunit, pos);
    }


    size_t find_last_of(const U16String& str) const {
        return content_.find_last_of(str.content_);
    }

    size_t find_last_of(const char16_t* str, size_t pos, size_t count) const {
        return content_.find_last_of(str, pos, count);
    }

    size_t find_last_of(const char16_t* str, size_t pos = 0) const {
        return content_.find_last_of(str, pos);
    }

    size_t find_last_of(char16_t codeunit, size_t pos = 0) const {
        return content_.find_last_of(codeunit, pos);
    }


    size_t find_last_not_of(const U16String& str) const {
        return content_.find_last_not_of(str.content_);
    }

    size_t find_last_not_of(const char16_t* str, size_t pos, size_t count) const {
        return content_.find_last_not_of(str, pos, count);
    }

    size_t find_last_not_of(const char16_t* str, size_t pos = 0) const {
        return content_.find_last_not_of(str, pos);
    }

    size_t find_last_not_of(char16_t codeunit, size_t pos = 0) const {
        return content_.find_last_not_of(codeunit, pos);
    }



    U8String to_u8() const;
    U32String to_u32() const;
    UWString to_uwstring() const;

    icu::UnicodeString to_icu_string() const {
        return icu::UnicodeString(castChars(data()), size());
    }

    const ContentT& to_std_string() const {
        return content_;
    }

    ContentT& to_std_string() {
        return content_;
    }

    CharT* data() {
        return &content_[0];
    }

    const CharT* data() const {
        return content_.data();
    }
};





using U16StringRef = const U16String&;

using UTextUniquePtr = std::unique_ptr<UText, void(*)(UText*)>;


UTextUniquePtr make_utext(const U16String& str);
UTextUniquePtr make_utext(U16String&& str);
UTextUniquePtr make_utext_ref(memoria::v1::U16String &str);

U16String to_u16string(UText* utext);
U16String to_u16string(UText* utext, int64_t start, int64_t length);

class UTextScope {
    UText* ut_;
public:
    UTextScope(UText* ut): ut_(ut) {}
    ~UTextScope() noexcept {
        if (ut_) {
            utext_close(ut_);
        }
    }
};

template <typename T> struct TypeHash;

template <>
struct TypeHash<U16String> {
    static const uint64_t Value = 62;
};

inline bool compare_gt(const U16String& first, const U16String& second) {
    return first.compare(second) > 0;
}


inline bool compare_eq(const U16String& first, const U16String& second) {
    return first.compare(second) == 0;
}


inline bool compare_lt(const U16String& first, const U16String& second) {
    return first.compare(second) > 0;
}


inline bool compare_ge(const U16String& first, const U16String& second) {
    return first.compare(second) >= 0;
}


inline bool compare_le(const U16String& first, const U16String& second) {
    return first.compare(second) <= 0;
}



class ICodeUnit16Provider {
public:
    virtual ~ICodeUnit16Provider() noexcept {}

    virtual int64_t read_to(int64_t position, char16_t* data, size_t size) = 0;
    virtual int64_t length() const = 0;
    virtual int64_t align_to_code_unit_start(int64_t position) = 0;

    virtual std::shared_ptr<ICodeUnit16Provider> clone() const = 0;
};


using CU16ProviderPtr = std::shared_ptr<ICodeUnit16Provider>;

U_CAPI UText*
utext_open_codepoint_accessor(UText *ut, const CU16ProviderPtr& ci,int32_t buffer_size, UErrorCode *status);

CU16ProviderPtr as_cu16_provider(UTextUniquePtr&& utext_ptr);

CU16ProviderPtr as_cu16_provider(U16String&& str);

int64_t get_u16_index_for(UText* ut, int64_t native_index);
int64_t get_chunk_index_for(UText* ut, int64_t native_index);
int64_t get_native_index_for(UText* ut);

}}

namespace std {

template <>
inline void swap(memoria::v1::U16String& one, memoria::v1::U16String& two) noexcept {
    std::swap(one.content_, two.content_);
}

template<>
struct hash<memoria::v1::U16String> {
    size_t operator()(const memoria::v1::U16String& str) const noexcept {
        return hash<std::u16string>()(str.to_std_string());
    }
};

template <typename CharTraits>
inline basic_ostream<char, CharTraits>& operator<<(basic_ostream<char, CharTraits>& out, const char16_t* str)
{
    out << memoria::v1::U16String(str).to_u8();
    return out;
}

template <typename CharTraits>
inline basic_ostream<char, CharTraits>& operator<<(basic_ostream<char, CharTraits>& out, const memoria::v1::U16String& str)
{
    out << str.to_u8();
    return out;
}



}

