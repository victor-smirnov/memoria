
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <sstream>

#ifdef __GNUC__
#include <cxxabi.h>
#else
#endif

#include <typeinfo>
#include <memoria/v1/core/types/types.hpp>

namespace memoria    {

#ifdef __GNUC__
template<typename T, int BufferSize = 40960>
struct TypeNameFactory
{
    static std::string name() {
        char buf[BufferSize];
        size_t len = sizeof(buf);
        abi::__cxa_demangle(typeid(T).name(), buf, &len, NULL);
        return std::string(buf);
    }

    static const char* cname() {
        return typeid(T).name();
    }
};
#else
template<typename T, int BufferSize = 40960>
struct TypeNameFactory
{
    static std::string name() {
        return std::string(typeid(T).name());
    }

    static const char* cname() {
        return typeid(T).name();
    }
};
#endif


}
