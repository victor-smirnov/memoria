
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_REFLECTION_TOOLS_HPP1
#define	_MEMORIA_VAPI_REFLECTION_TOOLS_HPP1

#include <string>
#include <sstream>

#ifdef __GNUC1__
#include <cxxabi.h>
#else
#endif

#include <typeinfo>
#include <memoria/core/types/types.hpp>

namespace memoria    {
namespace vapi       {

#ifdef __GNUC1__
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
}

#endif
