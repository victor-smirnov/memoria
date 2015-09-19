
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_RANDOM_HPP_
#define MEMORIA_CORE_TOOLS_RANDOM_HPP_

#include <memoria/core/types/types.hpp>
#include <random>
#include <functional>

namespace memoria {

//FIXME: this RNGs are single-threaded!

using RngEngine = std::mt19937_64;

template <typename T>
using UniformDistribution = std::uniform_int_distribution<T>;

template <typename T, typename Engine = RngEngine>
using RngT = decltype(std::bind(std::declval<UniformDistribution<T>>(), std::declval<Engine>()));

using RngInt 	= RngT<Int>;
using RngBigInt = RngT<BigInt>;

extern RngEngine 	rng_engine;
extern RngInt		int_generator;
extern RngBigInt	bigint_generator;

Int     getRandom();
Int     getRandom(Int max);
void    Seed(Int value);
Int     getSeed();

BigInt  getBIRandom();
BigInt  getBIRandom(BigInt max);
void    SeedBI(BigInt value);
BigInt  getSeedBI();


Int getNonZeroRandom(Int size);


}

#endif
