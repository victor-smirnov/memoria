
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/params.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria {



Int GetValueMultiplier(const char* chars, const char* ptr)
{
	if (*ptr == 0) {
		return 1;
	}
	else if (*ptr == 'K' || *ptr == 'k') {
		return 1024;
	}
	else if (*ptr == 'M' || *ptr == 'm') {
		return 1024*1024;
	}
	else if (*ptr == 'G' || *ptr == 'g') {
		return 1024*1024*1024;
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


void TaskParametersSet::Process()
{
	for (AbstractParamDescriptor* d: descriptors_)
	{
		d->Process();
	}
}

void TaskParametersSet::DumpProperties(std::ostream& os)
{
	for (AbstractParamDescriptor* d: descriptors_)
	{
		d->Dump(os);
	}
}






}
