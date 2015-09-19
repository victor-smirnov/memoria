// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/tools/random.hpp>

namespace memoria   {

RngEngine 	int_rng_engine;
RngEngine 	bigint_rng_engine;

RngInt		int_generator 		= std::bind(UniformDistribution<Int>(), int_rng_engine);
RngBigInt	bigint_generator	= std::bind(UniformDistribution<BigInt>(), bigint_rng_engine);

Int getRandom()
{
    return int_generator();
}

Int getRandom(Int max)
{
    return max > 0 ? int_generator() % max : 0;
}

void Seed(Int value)
{
    int_rng_engine.seed(value);
}

Int getSeed() {
    return 0;
}

BigInt getBIRandom()
{
    return bigint_generator();
}

BigInt getBIRandom(BigInt max)
{
    return max > 0 ? bigint_generator() % max : 0;
}

void SeedBI(BigInt value)
{
    bigint_rng_engine.seed(value);
}

BigInt getSeedBI() {
    return 0;
}

Int getNonZeroRandom(Int size)
{
    Int value = getRandom(size);
    return value != 0 ? value : getNonZeroRandom(size);
}



}
