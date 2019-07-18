
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
#include <memoria/v1/core/strings/format.hpp>
#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>



#include <string>
#include <sstream>
#include <type_traits>

#ifdef __GNUC__
#include <cxxabi.h>
#else
#endif

#include <typeinfo>

namespace memoria {
namespace v1 {

namespace _ {
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
    static U16String name()
    {
        UniquePtr<char> buf {
            abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
            ::free
        };

        if (buf)
        {
            return U8String(std::string(buf.get())).to_u16();
        }
        else {
            MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Demaingling failed for type {}", cname());
        }
    }

    static const char* cname() {
        return typeid(T).name();
    }
};

static inline U16String demangle(const char* name)
{
	char buf[40960];
	size_t len = sizeof(buf);
	abi::__cxa_demangle(name, buf, &len, NULL);
    return U8String(buf).to_u16();
}

#else
template<typename T>
struct TypeNameFactory
{
    static U16String name() {
        return U8String(typeid(T).name()).to_u16();
    }

    static const char* cname() {
        return typeid(T).name();
    }
};

static inline U16String demangle(const char* name)
{
    return U8String(name).to_u16();
}

#endif



template <typename T>
struct HasStandardTypeName: _::tn::STTNHelper<T> {};

template<typename T, bool Special = HasStandardTypeName<T>::value>
struct StandardTypeName;


template<typename T>
struct StandardTypeName<T, true>
{
    static U16String name()
    {
        return T::standard_type_name();
    }
};

template<typename T>
struct StandardTypeName<T, false>
{
    static U16String name()
    {
        return TypeNameFactory<T>::name();
    }
};


template <typename T>
U16String Type2Str() {
    return TypeNameFactory<T>::name();
}


template <typename T>
U16String StandardType2Str() {
    return StandardTypeName<T>::name();
}



template<>
struct StandardTypeName<uint8_t, false> {
    static U16String name() {
        return u"UInt8";
    }
};

template<>
struct StandardTypeName<int8_t, false> {
    static U16String name() {
        return u"Int8";
    }
};


template<>
struct StandardTypeName<uint16_t, false> {
    static U16String name() {
        return u"UInt16";
    }
};

template<>
struct StandardTypeName<int16_t, false> {
    static U16String name() {
        return u"Int16";
    }
};


template<>
struct StandardTypeName<uint32_t, false> {
    static U16String name() {
        return u"UInt32";
    }
};

template<>
struct StandardTypeName<int32_t, false> {
    static U16String name() {
        return u"Int32";
    }
};



template<>
struct StandardTypeName<uint64_t, false> {
    static U16String name() {
        return u"UInt64";
    }
};

template<>
struct StandardTypeName<int64_t, false> {
    static U16String name() {
        return u"Int64";
    }
};


template<>
struct StandardTypeName<double, false> {
    static U16String name() {
        return u"Double";
    }
};

template<>
struct StandardTypeName<float, false> {
    static U16String name() {
        return u"Float";
    }
};

template<>
struct StandardTypeName<bool, false> {
    static U16String name() {
        return u"Boolean";
    }
};

template<>
struct StandardTypeName<char, false> {
    static U16String name() {
        return u"Char8";
    }
};

template<>
struct StandardTypeName<char16_t, false> {
    static U16String name() {
        return u"Char16";
    }
};

template<>
struct StandardTypeName<char32_t, false> {
    static U16String name() {
        return u"Char32";
    }
};



}}
