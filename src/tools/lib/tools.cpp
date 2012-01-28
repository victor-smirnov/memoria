
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/tools/tools.hpp>

#include <sys/timeb.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include <random>
#include <functional>


namespace memoria {

using namespace std;


std::uniform_int_distribution<int> 		distribution;
std::mt19937 							engine;
auto 									generator 				= std::bind(distribution, engine);

std::uniform_int_distribution<BigInt> 	distribution_bi;
std::mt19937_64 						engine_bi;
auto 									generator_bi 			= std::bind(distribution_bi, engine_bi);


Int GetRandom()
{
	return generator();
}

Int GetRandom(Int max)
{
	return max > 0 ? generator() % max : 0;
}

void Seed(Int value)
{
	engine.seed(value);
}

Int GetSeed() {
	return 0;
}

BigInt GetBIRandom()
{
	return generator_bi();
}

BigInt GetBIRandom(BigInt max)
{
	return max > 0 ? generator_bi() % max : 0;
}

void SeedBI(BigInt value)
{
	engine_bi.seed(value);
}

BigInt GetSeedBI() {
	return 0;
}


BigInt	GetTimeInMillis()
{
	struct timeb tm;
	ftime(&tm);
	return tm.time * 1000 + tm.millitm;
}

String GetMillisPart(BigInt millis) {
	return millis < 100 ? "0"+ToString(millis) : ToString(millis);
}

String GetTwoDigitsPart(BigInt value) {
	return value < 10 ? "0"+ToString(value) : ToString(value);
}

String FormatTime(BigInt millis)
{
	if (millis < 1000)
	{
		return "0." + GetMillisPart(millis);
	}
	else if (millis < 60000)
	{
		BigInt seconds 	= millis / 1000;
		millis = millis % 1000;

		return ToString(seconds) +"." + GetMillisPart(millis);
	}
	else if (millis < 60000 * 60)
	{
		BigInt minutes 	= millis / 60000;

		millis -= minutes * 60000;

		BigInt seconds 	= millis / 1000;
		millis = millis % 1000;

		return ToString(minutes) +":" + GetTwoDigitsPart(seconds) +"." + GetMillisPart(millis);
	}
	else if (millis < 60000 * 60 * 24)
	{
		BigInt hours 	= millis / (60000 * 60);
		millis -= hours * 60000 * 60;

		BigInt minutes 	= millis / 60000;

		millis -= minutes * 60000;

		BigInt seconds 	= millis / 1000;
		millis = millis % 1000;

		return ToString(hours) + ":" + GetTwoDigitsPart(minutes) +":" + GetTwoDigitsPart(seconds) +"." + GetMillisPart(millis);
	}
	else
	{
		BigInt days 	= millis / (60000 * 60 * 24);
		millis -= days * 60000 * 60 * 24;

		BigInt hours 	= millis / (60000 * 60);
		millis -= hours * 60000 * 60;

		BigInt minutes 	= millis / 60000;
		millis -= minutes * 60000;

		BigInt seconds 	= millis / 1000;
		millis = millis % 1000;

		return ToString(days) + (days == 1? "day " : "days ") + GetTwoDigitsPart(hours) + ":" + GetTwoDigitsPart(minutes) +":" + GetTwoDigitsPart(seconds) +"." + GetMillisPart(millis);
	}
}



void Fill(char* buf, int size, char value)
{
	for (int c 	= 0; c < size; c++)
	{
		buf[c] = value;
	}
}

ArrayData CreateBuffer(Int size, UByte value)
{
	char* buf = (char*)malloc(size);

	for (Int c = 0;c < size; c++)
	{
		buf[c] = value;
	}

	return ArrayData(size, buf, true);
}


Int GetNonZeroRandom(Int size)
{
	Int value = GetRandom(size);
	return value != 0 ? value : GetNonZeroRandom(size);
}

ArrayData CreateRandomBuffer(UByte fill_value, Int max_size)
{
	return CreateBuffer(GetNonZeroRandom(max_size), fill_value);
}








} //memoria
