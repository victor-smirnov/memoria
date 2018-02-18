
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/strings/string.hpp>

#include <string>
#include <sstream>

namespace memoria {
namespace v1 {



U8String trimString(U8StringRef str);
bool isEmpty(U8StringRef str);
bool isEmpty(U8StringRef str, size_t start, size_t end, U8StringRef sep);
bool isEndsWith(U8StringRef str, U8StringRef end);
bool isStartsWith(U8StringRef str, U8StringRef start);
Long strToL(U8StringRef str);

// FIXME: move it into the string library
template <typename T>
U8String toString(const T& value, bool hex = false)
{
    std::stringstream str;
    if (hex) {
        str<<hex;
    }
    str<<value;
    return str.str();
}


static inline U8String toString(const uint8_t value, bool hex = false)
{
    std::stringstream str;
    if (hex) {
        str<<hex;
    }
    str<<(int32_t)value;
    return str.str();
}

static inline U8String toString(const int8_t value, bool hex = false)
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
    static U8String convert(const T& value, bool hex = false)
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
    static U8String convert(const bool& value)
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
struct AsString<U8String> {
    static U8String convert(U8StringRef value)
    {
        return value;
    }
};


long int ConvertToLongInt(U8StringRef str);
unsigned long int ConvertToULongInt(U8StringRef str);
long long ConvertToLongLong(U8StringRef str);
unsigned long long ConvertToULongLong(U8StringRef str);
double ConvertToDouble(U8StringRef str);
long double ConvertToLongDouble(U8StringRef str);
bool ConvertToBool(U8StringRef str);

template <typename T> struct FromString;

template <>
struct FromString<int32_t> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8());
    }

    static int32_t convert(U8StringRef str)
    {
        return ConvertToLongInt(str);
    }
};

template <>
struct FromString<int64_t> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8());
    }

    static int64_t convert(U8StringRef str)
    {
        return ConvertToLongLong(str);
    }
};

template <>
struct FromString<uint64_t> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8());
    }

    static uint64_t convert(U8StringRef str)
    {
        return ConvertToULongLong(str);
    }
};

#ifdef __APPLE__

template <>
struct FromString<unsigned long> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8());
    }

    static unsigned long convert(U8StringRef str)
    {
        return ConvertToULongInt(str);
    }
};

#endif

template <>
struct FromString<double> {
    static auto convert(U16StringRef str) {
        return convert(str.to_u8());
    }

    static double convert(U8StringRef str)
    {
        return ConvertToDouble(str);
    }
};

template <>
struct FromString<bool> {
    static bool convert(U16StringRef str) {
        return convert(str.to_u8());
    }

    static bool convert(U8StringRef str)
    {
        return ConvertToBool(str);
    }
};

template <>
struct FromString<U8String> {
    static U8String convert(U8StringRef str)
    {
        return str;
    }

    static U8String convert(U16StringRef str)
    {
        return str.to_u8();
    }
};

template <>
struct FromString<U16String> {
    static U16String convert(U16StringRef str)
    {
        return str;
    }

    static U16String convert(U8StringRef str)
    {
        return U8String(str).to_u16();
    }

};

template <typename T, size_t Size>
struct FromString<T[Size]> {

    static void convert(T* values, U16StringRef str) {
        return convert(values, str.to_u8());
    }

    static void convert(T* values, U8StringRef str)
    {
        size_t start = 0;

        for (size_t c = 0; c < Size; c++)
        {
            values[c] = 0;
        }

        for (size_t c = 0; c < Size; c++)
        {
            size_t pos = str.to_std_string().find_first_of(",", start);

            U8String value = trimString(str.to_std_string().substr(start, pos != StdString::npos ? pos - start : pos));

            if (!isEmpty(value.to_std_string()))
            {
                values[c] = FromString<T>::convert(value);
            }
            else {
                values[c] = 0;
            }

            if (pos != StdString::npos && pos < str.length())
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
