
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/tools/strings/string.hpp>

#include <string>
#include <sstream>

namespace memoria {
namespace v1 {



String trimString(StringRef str);
//String ReplaceFirst(StringRef str, StringRef txt);
//String ReplaceLast(StringRef str, StringRef txt);
//String ReplaceAll(StringRef str, StringRef txt);
bool isEmpty(StringRef str);
bool isEmpty(StringRef str, String::size_type start, String::size_type end, StringRef sep);
bool isEndsWith(StringRef str, StringRef end);
bool isStartsWith(StringRef str, StringRef start);
Long strToL(StringRef str);

// FIXME: move it into the string library
template <typename T>
String toString(const T& value, bool hex = false)
{
    std::stringstream str;
    if (hex) {
        str<<hex;
    }
    str<<value;
    return str.str();
}


static inline String toString(const uint8_t value, bool hex = false)
{
    std::stringstream str;
    if (hex) {
        str<<hex;
    }
    str<<(int32_t)value;
    return str.str();
}

static inline String toString(const int8_t value, bool hex = false)
{
    std::stringstream str;
    if (hex) {
        str<<hex;
    }
    str<<(int32_t)value;
    return str.str();
}


template <typename T>
struct AsString {
    static String convert(const T& value, bool hex = false)
    {
        std::stringstream str;
        if (hex) {
            str<<hex;
        }
        str<<value;
        return str.str();
    }
};

template <>
struct AsString<bool> {
    static String convert(const bool& value)
    {
        std::stringstream str;

        if (value)
        {
            str<<"True";
        }
        else
        {
            str<<"False";
        }

        return str.str();
    }
};

template <>
struct AsString<String> {
    static String convert(StringRef value)
    {
        return value;
    }
};


long int ConvertToLongInt(StringRef str);
unsigned long int ConvertToULongInt(StringRef str);
long long ConvertToLongLong(StringRef str);
unsigned long long ConvertToULongLong(StringRef str);
double ConvertToDouble(StringRef str);
long double ConvertToLongDouble(StringRef str);
bool ConvertToBool(StringRef str);

template <typename T> struct FromString;

template <>
struct FromString<int32_t> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8().to_std_string());
    }

    static int32_t convert(StringRef str)
    {
        return ConvertToLongInt(str);
    }
};

template <>
struct FromString<int64_t> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8().to_std_string());
    }

    static int64_t convert(StringRef str)
    {
        return ConvertToLongLong(str);
    }
};

template <>
struct FromString<uint64_t> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8().to_std_string());
    }

    static uint64_t convert(StringRef str)
    {
        return ConvertToULongLong(str);
    }
};

#ifdef __APPLE__

template <>
struct FromString<unsigned long> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8().to_std_string());
    }

    static unsigned long convert(StringRef str)
    {
        return ConvertToULongInt(str);
    }
};

#endif

template <>
struct FromString<double> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8().to_std_string());
    }

    static double convert(StringRef str)
    {
        return ConvertToDouble(str);
    }
};

template <>
struct FromString<bool> {
    static bool convert(U16StringRef str) {
        return convert(str.to_u8().to_std_string());
    }

    static bool convert(StringRef str)
    {
        return ConvertToBool(str);
    }
};

template <>
struct FromString<String> {
    static String convert(StringRef str)
    {
        return str;
    }

    static String convert(U16StringRef str)
    {
        return str.to_u8().to_std_string();
    }
};

template <>
struct FromString<U16String> {
    static U16String convert(U16StringRef str)
    {
        return str;
    }

    static U16String convert(StringRef str)
    {
        return U8String(str).to_u16();
    }

};

template <typename T, size_t Size>
struct FromString<T[Size]> {

    static void convert(T* values, U16StringRef str) {
        return convert(values, str.to_u8().to_std_string());
    }

    static void convert(T* values, StringRef str)
    {
        size_t start = 0;

        for (size_t c = 0; c < Size; c++)
        {
            values[c] = 0;
        }

        for (size_t c = 0; c < Size; c++)
        {
            size_t pos = str.find_first_of(",", start);

            String value = trimString(str.substr(start, pos != String::npos ? pos - start : pos));

            if (!isEmpty(value))
            {
                values[c] = FromString<T>::convert(value);
            }
            else {
                values[c] = 0;
            }

            if (pos != String::npos && pos < str.length())
            {
                start = pos + 1;
            }
            else {
                break;
            }
        }
    }
};



}}
