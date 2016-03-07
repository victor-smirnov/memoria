// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/tools/random.hpp>

namespace memoria   {

RngInt      int_generator;
RngBigInt   bigint_generator;


RngInt& getGlobalIntGenerator() {
    return int_generator;
}

RngBigInt& getGlobalBigIntGenerator() {
    return bigint_generator;
}

Int getRandomG()
{
    return int_generator();
}

Int getRandomG(Int max)
{
    return max > 0 ? int_generator() % max : 0;
}

void Seed(Int value)
{
    std::seed_seq ss({value});
    int_generator.engine().seed(ss);
}


BigInt getBIRandomG()
{
    return bigint_generator();
}

BigInt getBIRandomG(BigInt max)
{
    return max > 0 ? bigint_generator() % max : 0;
}

void SeedBI(BigInt value)
{
    std::seed_seq ss({value});
    bigint_generator.engine().seed(ss);
}


Int getNonZeroRandomG(Int size)
{
    Int value = getRandomG(size);
    return value != 0 ? value : getNonZeroRandomG(size);
}



}
