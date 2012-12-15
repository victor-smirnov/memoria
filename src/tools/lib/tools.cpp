
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


std::uniform_int_distribution<int>      distribution;
std::minstd_rand                            engine;
auto                                    generator               = std::bind(distribution, engine);

std::uniform_int_distribution<BigInt>   distribution_bi;
std::mt19937_64                         engine_bi;
auto                                    generator_bi            = std::bind(distribution_bi, engine_bi);


Int getRandom()
{
    return generator();
}

Int getRandom(Int max)
{
    return max > 0 ? generator() % max : 0;
}

void Seed(Int value)
{
    engine.seed(value);
}

Int getSeed() {
    return 0;
}

BigInt getBIRandom()
{
    return generator_bi();
}

BigInt getBIRandom(BigInt max)
{
    return max > 0 ? generator_bi() % max : 0;
}

void SeedBI(BigInt value)
{
    engine_bi.seed(value);
}

BigInt getSeedBI() {
    return 0;
}


BigInt  getTimeInMillis()
{
    struct timeb tm;
    ftime(&tm);
    return tm.time * 1000 + tm.millitm;
}

String getMillisPart(BigInt millis) {
    return millis < 100 ? "0"+toString(millis) : toString(millis);
}

String getTwoDigitsPart(BigInt value) {
    return value < 10 ? "0"+toString(value) : toString(value);
}

String FormatTime(BigInt millis)
{
    if (millis < 1000)
    {
        return "0." + getMillisPart(millis);
    }
    else if (millis < 60000)
    {
        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(seconds) +"." + getMillisPart(millis);
    }
    else if (millis < 60000 * 60)
    {
        BigInt minutes  = millis / 60000;

        millis -= minutes * 60000;

        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(minutes) +":" + getTwoDigitsPart(seconds) +"." + getMillisPart(millis);
    }
    else if (millis < 60000 * 60 * 24)
    {
        BigInt hours    = millis / (60000 * 60);
        millis -= hours * 60000 * 60;

        BigInt minutes  = millis / 60000;

        millis -= minutes * 60000;

        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(hours) + ":" + getTwoDigitsPart(minutes)
                               + ":" + getTwoDigitsPart(seconds)
                               + "." + getMillisPart(millis);
    }
    else
    {
        BigInt days     = millis / (60000 * 60 * 24);
        millis -= days * 60000 * 60 * 24;

        BigInt hours    = millis / (60000 * 60);
        millis -= hours * 60000 * 60;

        BigInt minutes  = millis / 60000;
        millis -= minutes * 60000;

        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(days) + (days == 1? "day " : "days ")
                              + getTwoDigitsPart(hours)
                              + ":"
                              + getTwoDigitsPart(minutes)
                              + ":"
                              + getTwoDigitsPart(seconds)
                              + "."
                              + getMillisPart(millis);
    }
}



void Fill(char* buf, int size, char value)
{
    for (int c  = 0; c < size; c++)
    {
        buf[c] = value;
    }
}



Int getNonZeroRandom(Int size)
{
    Int value = getRandom(size);
    return value != 0 ? value : getNonZeroRandom(size);
}










} //memoria
