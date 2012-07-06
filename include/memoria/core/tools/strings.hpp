
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


MEMORIA_API String trimString(StringRef str);
//MEMORIA_API String ReplaceFirst(StringRef str, StringRef txt);
//MEMORIA_API String ReplaceLast(StringRef str, StringRef txt);
//MEMORIA_API String ReplaceAll(StringRef str, StringRef txt);
MEMORIA_API bool isEmpty(StringRef str);
MEMORIA_API bool isEmpty(StringRef str, String::size_type start, String::size_type end, StringRef sep);
MEMORIA_API bool isEndsWith(StringRef str, StringRef end);
MEMORIA_API bool isStartsWith(StringRef str, StringRef start);
MEMORIA_API Long strToL(StringRef str);

// FIXME: move it into the string library
template <typename T>
String toString(const T& value, bool hex = false)
{
	std::stringstream str;
	if (hex) {
		str<<hex;
	}
	str<<value;
	return str.str();
}

template <typename T>
struct AsString {
	static String convert(const T& value, bool hex = false)
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
	static String convert(const bool& value)
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
struct AsString<String> {
	static String convert(StringRef value)
	{
		return value;
	}
};


long int ConvertToLongInt(StringRef str);
long long ConvertToLongLong(StringRef str);
double ConvertToDouble(StringRef str);
long double ConvertToLongDouble(StringRef str);
bool ConvertToBool(StringRef str);

template <typename T> struct FromString;

template <>
struct FromString<Int> {
	static Int convert(StringRef str)
	{
		return ConvertToLongInt(str);
	}
};

template <>
struct FromString<BigInt> {
	static BigInt convert(StringRef str)
	{
		return ConvertToLongLong(str);
	}
};

template <>
struct FromString<double> {
	static double convert(StringRef str)
	{
		return ConvertToDouble(str);
	}
};

template <>
struct FromString<bool> {
	static bool convert(StringRef str)
	{
		return ConvertToBool(str);
	}
};

template <>
struct FromString<String> {
	static String convert(StringRef str)
	{
		return str;
	}
};

}}

#endif
