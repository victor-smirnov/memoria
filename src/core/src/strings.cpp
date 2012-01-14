
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/strings.hpp>



#include <errno.h>

namespace memoria{ namespace vapi {

using namespace std;

String TrimString(StringRef str)
{
	if (str.length() == 0)
	{
		return "";
	}
	else {
		unsigned begin = str.find_first_not_of("\n\r\t ");
		if (begin == String::npos)
		{
			return "";
		}
		else {
			unsigned end = str.find_last_not_of("\n\r\t ");
			if (end != String::npos)
			{
				return str.substr(begin, end - begin + 1);
			}
			else {
				return str.substr(begin, str.length() - begin);
			}
		}
	}
}

bool IsEndsWith(StringRef str, StringRef end) {
	if (end.length() > str.length())
	{
		return false;
	}
	else {
		UInt l0 = str.length();
		UInt l1 = end.length();
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

bool IsStartsWith(StringRef str, StringRef start) {
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

bool IsEmpty(StringRef str) {
	return str.find_first_not_of("\r\n\t ") == String::npos;
}

bool IsEmpty(StringRef str, String::size_type start, String::size_type end, StringRef sep)
{
	if (end == String::npos) end = str.length();

	if (start != String::npos && start < str.length() && start < end - 1)
	{
		String::size_type idx = str.find_first_not_of((sep+"\t ").data(), start);

		if (idx != String::npos)
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

Long StrToL(StringRef value) {
	if (!IsEmpty(value))
	{
		const char* ptr = TrimString(value).c_str();
		char* end_ptr;

		errno = 0;
		Long v = strtol(ptr, &end_ptr, 0);

		if (errno == 0)
		{
			if (*end_ptr == '\0')
			{
				return v;
			}
			else {
				throw MemoriaException(MEMORIA_SOURCE, "Invalid integer value: " + value);
			}
		}
		else {
			throw MemoriaException(MEMORIA_SOURCE, "Invalid integer value: " + value);
		}
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, "Invalid integer value: " + value);
	}
}

String ReplaceFirst(StringRef str, StringRef txt) {
	return str;
}

String ReplaceLast(StringRef str, StringRef txt) {
	return str;
}

String ReplaceAll(StringRef str, StringRef txt) {
	return str;
}





Int GetValueMultiplier(const char* chars, const char* ptr)
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
		throw ::memoria::vapi::MemoriaException(MEMORIA_SOURCE, "Invalid number format: "+String(chars));
	}
}

void CheckError(const char* chars, const char* ptr)
{
	if (*ptr != 0) {
		throw ::memoria::vapi::MemoriaException(MEMORIA_SOURCE, "Invalid number format: "+String(chars));
	}
}


long int ConvertToLongInt(StringRef str)
{
	const char* chars = str.c_str();
	char* endptr;

	long int value = strtol(chars, &endptr, 0);

	return value * GetValueMultiplier(chars, endptr);
}

long long ConvertToLongLong(StringRef str)
{
	const char* chars = str.c_str();
	char* endptr;

	long int value = strtoll(chars, &endptr, 0);

	return value * GetValueMultiplier(chars, endptr);
}

double ConvertToDouble(StringRef str)
{
	const char* chars = str.c_str();
	char* endptr;

	double value = strtod(chars, &endptr);

	CheckError(chars, endptr);

	return value;
}

long double ConvertToLongDouble(StringRef str)
{
	const char* chars = str.c_str();
	char* endptr;

	long double value = strtold(chars, &endptr);

	CheckError(chars, endptr);

	return value;
}

bool ConvertToBool(StringRef str)
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
		throw ::memoria::vapi::MemoriaException(MEMORIA_SOURCE, "Invalid boolean format: "+str);
	}
}

}}


