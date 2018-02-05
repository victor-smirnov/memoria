
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
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/tools/config.hpp>

#include <memoria/v1/core/tools/strings/strings.hpp>
#include <memoria/v1/core/tools/strings/string_buffer.hpp>

#include <string>
#include <memory>
#include <ostream>

#include <unicode/ustring.h>
#include <unicode/unistr.h>
#include <unicode/utext.h>
#include "u_string_capi.hpp"

namespace memoria {
namespace v1 {

class U8String;
class U32String;
class UWString;

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


public:
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

    size_t code_points() const {
        return u_countChar32(castChars(content_.data()), content_.size());
    }

    int32_t compare(const U16String& other) const {
        return u_strCompare(castChars(content_.data()), content_.size(), castChars(other.content_.data()), other.content_.size(), true);
    }

    U8String to_u8() const;
    U32String to_u32() const;
    UWString to_uwstring() const;

    icu::UnicodeString to_icu_string() const {
        return icu::UnicodeString(castChars(data()), size());
    }

    CharT* data() {
        return &content_[0];
    }

    const CharT* data() const {
        return content_.data();
    }
};

template <typename CharTraits>
inline std::basic_ostream<char, CharTraits>& operator<<(std::basic_ostream<char, CharTraits>& out, const U16String& str)
{
    out << U8String(str);
    return out;
}

using U16StringRef = const U16String&;

using UTextUniquePtr = std::unique_ptr<UText, void(*)(UText*)>;


UTextUniquePtr make_utext(const U16String& str);
UTextUniquePtr make_utext(U16String&& str);
UTextUniquePtr make_utext_ref(memoria::v1::U16String &str);

U16String to_u16string(const UTextUniquePtr& utext);
U16String to_u16string(const UTextUniquePtr& utext, int64_t start, int64_t length);



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

CU16ProviderPtr wrap_utext(UTextUniquePtr&& utext_ptr);

int64_t get_u16_index_for(UText* ut, int64_t native_index);
int64_t get_chunk_index_for(UText* ut, int64_t native_index);
int64_t get_native_index_for(UText* ut);

}}
