
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




#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/format.hpp>

#include <errno.h>

namespace memoria {

U8StringView trim_string(U8StringView str)
{
    size_t start;
    size_t end;

    for (start = 0; start < str.length(); start++) {
        if (str[start] != ' ') {
            break;
        }
    }

    for (end = str.length(); end > 0; end--) {
        if (str[end - 1] != ' ') {
            break;
        }
    }

    return str.substr(start, end - start);
}


U8String trimString(U8StringRef str)
{
    if (str.length() == 0)
    {
        return "";
    }
    else {
        size_t begin = str.to_std_string().find_first_not_of("\n\r\t ");
        if (begin == StdString::npos)
        {
            return "";
        }
        else {
            size_t end = str.to_std_string().find_last_not_of("\n\r\t ");
            if (end != StdString::npos)
            {
                return str.to_std_string().substr(begin, end - begin + 1);
            }
            else {
                return str.to_std_string().substr(begin, str.length() - begin);
            }
        }
    }
}

bool isEndsWith(U8StringRef str, U8StringRef end) {
    if (end.length() > str.length())
    {
        return false;
    }
    else {
        uint32_t l0 = str.length();
        uint32_t l1 = end.length();
        for (unsigned c = 0; c < end.length(); c++)
        {
            if (str[l0 - l1 + c] != end[c])
            {
                return false;
            }
        }
        return true;
    }
}

bool isStartsWith(U8StringRef str, U8StringRef start) {
    if (start.length() > str.length())
    {
        return false;
    }
    else {
        for (unsigned c = 0; c < start.length(); c++)
        {
            if (str[c] != start[c])
            {
                return false;
            }
        }
        return true;
    }
}

bool isEmpty(U8StringRef str) {
    return str.to_std_string().find_first_not_of("\r\n\t ") == StdString::npos;
}

bool isEmpty(U8StringRef str, size_t start, size_t end, U8StringRef sep)
{
    if (end == StdString::npos) end = str.length();

    if (start != StdString::npos && start < str.length() && start < end - 1)
    {
        size_t idx = str.to_std_string().find_first_not_of((sep+"\t ").data(), start);

        if (idx != StdString::npos)
        {
            return idx >= end;
        }
        else {
            return true;
        }
    }
    else {
        return true;
    }
}

bool isEmpty(StdStringRef str, size_t start, size_t end, StdStringRef sep)
{
    if (end == StdString::npos) end = str.length();

    if (start != StdString::npos && start < str.length() && start < end - 1)
    {
        size_t idx = str.find_first_not_of((sep+"\t ").data(), start);

        if (idx != StdString::npos)
        {
            return idx >= end;
        }
        else {
            return true;
        }
    }
    else {
        return true;
    }
}

U8String ReplaceFirst(U8StringRef str, U8StringRef txt) {
    return str;
}

U8String ReplaceLast(U8StringRef str, U8StringRef txt) {
    return str;
}

U8String ReplaceAll(U8StringRef str, U8StringRef txt) {
    return str;
}





int32_t getValueMultiplier(const char* chars, const char* ptr)
{
    if (*ptr == 0) {
        return 1;
    }
    else if (*ptr == 'K') {
        return 1024;
    }
    else if (*ptr == 'M') {
        return 1024*1024;
    }
    else if (*ptr == 'G') {
        return 1024*1024*1024;
    }
    else if (*ptr == 'k') {
        return 1000;
    }
    else if (*ptr == 'm') {
        return 1000*1000;
    }
    else if (*ptr == 'g') {
        return 1000*1000*1000;
    }
    else {
        MMA_THROW(Exception()) << WhatInfo(format_u8("Invalid number format: {}", chars));
    }
}

void checkError(const char* chars, const char* ptr)
{
    if (*ptr != 0) {
        MMA_THROW(Exception()) << WhatInfo(format_u8("Invalid number format: {}", chars));
    }
}


long int ConvertToLongInt(U8StringRef str)
{
    const char* chars = str.data();
    char* endptr;

    long int value = strtol(chars, &endptr, 0);

    return value * getValueMultiplier(chars, endptr);
}

unsigned long int ConvertToULongInt(U8StringRef str)
{
    const char* chars = str.data();
    char* endptr;

    unsigned long int value = strtoul(chars, &endptr, 0);

    return value * getValueMultiplier(chars, endptr);
}


long long ConvertToLongLong(U8StringRef str)
{
    const char* chars = str.data();
    char* endptr;

    long long int value = strtoll(chars, &endptr, 0);

    return value * getValueMultiplier(chars, endptr);
}

unsigned long long ConvertToULongLong(U8StringRef str)
{
    const char* chars = str.data();
    char* endptr;

    unsigned long long int value = strtoull(chars, &endptr, 0);

    return value * getValueMultiplier(chars, endptr);
}

double ConvertToDouble(U8StringRef str)
{
    const char* chars = str.data();
    char* endptr;

    double value = strtod(chars, &endptr);

    checkError(chars, endptr);

    return value;
}

long double ConvertToLongDouble(U8StringRef str)
{
    const char* chars = str.data();
    char* endptr;

    long double value = strtod(chars, &endptr);

    checkError(chars, endptr);

    return value;
}

bool ConvertToBool(U8StringRef str)
{
    if (str == "true" || str == "True" || str == "Yes" || str == "yes" || str == "1")
    {
        return true;
    }
    else if (str == "false" || str == "False" || str == "No" || str == "no" || str == "0")
    {
        return false;
    }
    else {
        MMA_THROW(Exception()) << WhatInfo(format_u8("Invalid boolean format: {}", str));
    }
}

}
