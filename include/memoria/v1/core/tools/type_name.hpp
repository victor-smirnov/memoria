
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

#include <memoria/v1/core/tools/strings/string.hpp>

#include <string>
#include <sstream>

#ifdef __GNUC__
#include <cxxabi.h>
#else
#endif

#include <typeinfo>
#include <memoria/v1/core/types/types.hpp>

namespace memoria {
namespace v1 {

#ifdef __GNUC__
template<typename T, int BufferSize = 40960>
struct TypeNameFactory
{
    static U8String name() {
        char buf[BufferSize];
        size_t len = sizeof(buf);
        abi::__cxa_demangle(typeid(T).name(), buf, &len, NULL);
        return std::string(buf);
    }

    static const char* cname() {
        return typeid(T).name();
    }
};

static inline U8String demangle(const char* name)
{
	char buf[40960];
	size_t len = sizeof(buf);
	abi::__cxa_demangle(name, buf, &len, NULL);
    return U8String(buf);
}

#else
template<typename T, int BufferSize = 40960>
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


}}
