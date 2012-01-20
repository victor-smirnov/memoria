
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/tools/tools.hpp>

#include <sys/timeb.h>
#include <stdlib.h>
#include <stdio.h>

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
	return generator() % max;
}

void Seed(Int value)
{
	engine.seed(value);
}


BigInt GetBIRandom()
{
	return generator_bi();
}

BigInt GetBIRandom(BigInt max)
{
	return generator_bi() % max;
}

void SeedBI(BigInt value)
{
	engine_bi.seed(value);
}

BigInt	GetTimeInMillis()
{
	struct timeb tm;
	ftime(&tm);
	return tm.time * 1000 + tm.millitm;
}

} //memoria
