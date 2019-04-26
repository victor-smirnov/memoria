
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

#ifdef __GNUC__
#include <cxxabi.h>
#else
#endif

#include <typeinfo>

namespace memoria {
namespace v1 {

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
U16String Type2Str = TypeNameFactory<T>::name();

}}
