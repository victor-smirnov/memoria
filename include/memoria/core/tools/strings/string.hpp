
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_TOOLS_STRINGS_STRING_HPP
#define _MEMORIA_VAPI_TOOLS_STRINGS_STRING_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>

#include <memoria/core/tools/strings/strings.hpp>
#include <memoria/core/tools/strings/string_buffer.hpp>

#include <string>
#include <sstream>

namespace memoria {

using String 	= std::string;
using StringRef = const String&;

template <typename T> struct TypeHash;

template <>
struct TypeHash<String> {
    static const UInt Value = 60;
};

}

#endif
