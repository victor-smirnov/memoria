
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_RANDOM_HPP_
#define MEMORIA_CORE_TOOLS_RANDOM_HPP_

#include <memoria/core/types/types.hpp>
#include <random>
#include <functional>
#include <limits>
#include <iostream>

namespace memoria {

//FIXME: this RNGs are single-threaded!

using RngEngine64 = std::mt19937_64;
using RngEngine32 = std::mt19937;


template <typename T, typename Engine>
class RNG {
	Engine engine_;

	std::uniform_int_distribution<T> distribution_;

public:
	RNG(){}

	auto operator()()
	{
		return distribution_(engine_);
	}


	auto operator()(T max)
	{
		std::uniform_int_distribution<T> distribution(0, max > 0 ? max - 1 : 0);
		return distribution(engine_);
	}

	Engine& engine() {
		return engine_;
	}

	const Engine& engine() const {
		return engine_;
	}

	void seed(T value) {
		std::seed_seq ss({value});
		engine_.seed(ss);
	}
};

using RngInt 	= RNG<Int, RngEngine32>;
using RngBigInt = RNG<BigInt, RngEngine64>;


RngInt& getGlobalIntGenerator();
RngBigInt& getGlobalBigIntGenerator();

Int     getRandomG();
Int     getRandomG(Int max);
void    Seed(Int value);
Int     getSeed();

BigInt  getBIRandomG();
BigInt  getBIRandomG(BigInt max);
void    SeedBI(BigInt value);
BigInt  getSeedBI();


Int getNonZeroRandomG(Int size);


}

#endif
