
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

#include <memoria/core/types.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/memory/malloc.hpp>
#include <memoria/core/exceptions/exceptions.hpp>



#include <string>
#include <sstream>
#include <type_traits>

#ifdef __GNUC__
#include <cxxabi.h>
#else
#endif

#include <typeinfo>

namespace memoria {

namespace detail {
namespace tn {

template<typename>
struct SfinaeTrue : std::true_type{};

template <typename T>
static auto test_for_standard_type_name(int) -> SfinaeTrue<decltype(T::standard_type_name())>;

template <typename T>
static auto test_for_standard_type_name(long) -> std::false_type;

template <typename T>
using STTNHelper = decltype(test_for_standard_type_name<T>(0));

}}





#ifdef __GNUC__
template<typename T>
struct TypeNameFactory
{
    static U8String name()
    {
        UniquePtr<char> buf {
            abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
            ::free
        };

        if (buf)
        {
            return U8String(std::string(buf.get()));
        }
        else {
            MMA_THROW(RuntimeException()) << format_ex("Demaingling failed for type {}", cname());
        }
    }

    static const char* cname() {
        return typeid(T).name();
    }
};

static inline U8String demangle(const char* name)
{
    UniquePtr<char> buf {
        abi::__cxa_demangle(name, nullptr, nullptr, nullptr),
        ::free
    };

    if (buf)
    {
        return U8String(std::string(buf.get()));
    }
    else {
        MMA_THROW(RuntimeException()) << format_ex("Demaingling failed for type {}", name);
    }
}

#else
template<typename T>
struct TypeNameFactory
{
    static U8String name() {
        return U8String(typeid(T).name());
    }

    static const char* cname() {
        return typeid(T).name();
    }
};

static inline U8String demangle(const char* name)
{
    return U8String(name);
}

#endif



template <typename T>
struct HasStandardTypeName: detail::tn::STTNHelper<T> {};

template<typename T, bool Special = HasStandardTypeName<T>::value>
struct StandardTypeName;


template<typename T>
struct StandardTypeName<T, true>
{
    static U8String name()
    {
        return T::standard_type_name();
    }
};

template<typename T>
struct StandardTypeName<T, false>
{
    static U8String name()
    {
        return TypeNameFactory<T>::name().to_u16();
    }
};


template <typename T>
U8String Type2Str() {
    return TypeNameFactory<T>::name();
}


template <typename T>
U8String StandardType2Str() {
    return StandardTypeName<T>::name();
}



template<>
struct StandardTypeName<uint8_t, false> {
    static U8String name() {
        return "UInt8";
    }
};

template<>
struct StandardTypeName<int8_t, false> {
    static U8String name() {
        return "Int8";
    }
};


template<>
struct StandardTypeName<uint16_t, false> {
    static U8String name() {
        return "UInt16";
    }
};

template<>
struct StandardTypeName<int16_t, false> {
    static U8String name() {
        return "Int16";
    }
};


template<>
struct StandardTypeName<uint32_t, false> {
    static U8String name() {
        return "UInt32";
    }
};

template<>
struct StandardTypeName<int32_t, false> {
    static U8String name() {
        return "Int32";
    }
};



template<>
struct StandardTypeName<uint64_t, false> {
    static U8String name() {
        return "UInt64";
    }
};

template<>
struct StandardTypeName<int64_t, false> {
    static U8String name() {
        return "Int64";
    }
};


template<>
struct StandardTypeName<double, false> {
    static U8String name() {
        return "Double";
    }
};

template<>
struct StandardTypeName<float, false> {
    static U8String name() {
        return "Float";
    }
};

template<>
struct StandardTypeName<bool, false> {
    static U8String name() {
        return "Boolean";
    }
};

template<>
struct StandardTypeName<char, false> {
    static U8String name() {
        return "Char8";
    }
};

template<>
struct StandardTypeName<char16_t, false> {
    static U8String name() {
        return "Char16";
    }
};

template<>
struct StandardTypeName<char32_t, false> {
    static U8String name() {
        return "Char32";
    }
};



}
