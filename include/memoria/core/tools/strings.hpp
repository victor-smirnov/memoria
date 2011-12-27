
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_TOOLS_STRINGS_HPP
#define	_MEMORIA_VAPI_TOOLS_STRINGS_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>

#include <string>

namespace memoria { namespace vapi {


//MEMORIA_API String TrimString(StringRef str);
//MEMORIA_API String ReplaceFirst(StringRef str, StringRef txt);
//MEMORIA_API String ReplaceLast(StringRef str, StringRef txt);
//MEMORIA_API String ReplaceAll(StringRef str, StringRef txt);
MEMORIA_API bool IsEmpty(StringRef str);
MEMORIA_API bool IsEmpty(StringRef str, String::size_type start, String::size_type end, StringRef sep);
//MEMORIA_API bool IsEndsWith(StringRef str, StringRef end);
//MEMORIA_API bool IsStartsWith(StringRef str, StringRef start);
//MEMORIA_API Long StrToL(StringRef str);

// FIXME: move it into the string library
template <typename T>
String ToString(const T& value, bool hex = false)
{
	std::stringstream str;
	if (hex) {
		str<<hex;
	}
	str<<value;
	return str.str();
}


}}

#endif
