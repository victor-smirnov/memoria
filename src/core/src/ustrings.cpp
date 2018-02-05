
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

#include <memoria/v1/core/tools/strings/string.hpp>

namespace memoria {
namespace v1 {

U8String U16String::to_u8() const
{
    return U8String(*this);
}

U32String U16String::to_u32() const
{
    return U32String(*this);
}

UWString U16String::to_uwstring() const {
    return UWString(*this);
}



U16String U32String::to_u16() const
{
    return U16String(*this);
}

U8String U32String::to_u8() const
{
    return U8String(U16String(*this));
}

UWString U32String::to_uwstring() const
{
    return UWString(U16String(*this));
}



U16String U8String::to_u16() const
{
    return U16String(*this);
}


U32String U8String::to_u32() const
{
    return U32String(U16String(*this));
}


UWString U8String::to_uwstring() const
{
    return UWString(U16String(*this));
}


U8String UWString::to_u8() const
{
    return U8String(U16String(*this));
}

U16String UWString::to_u16() const
{
    return U16String(*this);
}

U32String UWString::to_u32() const
{
    return U32String(U16String(*this));
}




U16String::U16String(const U8String& other):
    content_((size_t)UStrUTF8Length(other.content_.data()), (CharT)0)
{
    UErrorCode error_code{};
    u_strFromUTF8(
            castChars(&content_[0]),
            (int32_t)content_.size() + 1,
            nullptr,
            castChars(other.content_.data()),
            -1, &error_code
    );
    StringException::assertOk(error_code);
}

U16String::U16String(const U32String& other):
    content_((size_t)UStrUTF32Length(other.content_.data()), (CharT)0)
{
    UErrorCode error_code{};
    u_strFromUTF32(
            castChars(&content_[0]),
            (int32_t)content_.size() + 1,
            nullptr,
            castChars(other.content_.data()),
            -1, &error_code
    );
    StringException::assertOk(error_code);
}


U16String::U16String(const UWString& other):
    content_((size_t)UStrUWLength(other.content_.data()), (CharT)0)
{
    UErrorCode error_code{};
    u_strFromWCS(
            castChars(&content_[0]),
            (int32_t)content_.size() + 1,
            nullptr,
            castChars(other.content_.data()),
            -1, &error_code
    );
    StringException::assertOk(error_code);
}



U8String::U8String(const U16String& other):
    content_((size_t)UStrUTF8Length(other.content_.data()), (CharT)0)
{
    UErrorCode error_code{};
    u_strToUTF8(
            castChars(&content_[0]),
            (int32_t)content_.size() + 1,
            nullptr,
            castChars(other.content_.data()),
            -1, &error_code
    );
    StringException::assertOk(error_code);
}

U32String::U32String(const U16String& other):
    content_((size_t)UStrUTF32Length(other.content_.data()), (CharT)0)
{
    UErrorCode error_code{};
    u_strToUTF32(
            castChars(&content_[0]),
            (int32_t)content_.size() + 1,
            nullptr,
            castChars(other.content_.data()),
            -1, &error_code
    );
    StringException::assertOk(error_code);
}


UWString::UWString(const U16String& other):
    content_((size_t)UStrUWLength(other.content_.data()), (CharT)0)
{
    UErrorCode error_code{};
    u_strToWCS(
            castChars(&content_[0]),
            (int32_t)content_.size() + 1,
            nullptr,
            castChars(other.content_.data()),
            -1, &error_code
    );
    StringException::assertOk(error_code);
}

}}
