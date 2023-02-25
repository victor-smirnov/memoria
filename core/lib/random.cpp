// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <memoria/core/tools/random.hpp>

#include <memoria/core/exceptions/exceptions.hpp>

#include <sstream>

namespace memoria {

RngInt& getGlobalIntGenerator() {

    thread_local RngInt int_generator;

    return int_generator;
}

RngInt64& getGlobalInt64Generator() {
    thread_local RngInt64 bigint_generator;
    return bigint_generator;
}

int32_t getRandomG()
{
    return getGlobalIntGenerator()();
}

int32_t getRandomG(int32_t max_v)
{
    return max_v > 0 ? getGlobalIntGenerator()(max_v) : 0;
}

void Seed(int32_t value)
{
    std::seed_seq ss({value});
    getGlobalIntGenerator().engine().seed(ss);
}


int64_t getBIRandomG()
{
    return getGlobalInt64Generator()();
}

int64_t getBIRandomG(int64_t max)
{
    return max > 0 ? getGlobalInt64Generator()(max) : 0;
}

void SeedBI(int64_t value)
{
    std::seed_seq ss({value});
    getGlobalInt64Generator().engine().seed(ss);
}


int32_t getNonZeroRandomG(int32_t size)
{
    if (size > 1)
    {
        int32_t value;
        while ((value = getRandomG(size)) == 0) {}
        return value;
    }
    else {
        MMA_THROW(RuntimeException()) << WhatCInfo("Invalid argument");
    }
}

U8String create_random_string(size_t length)
{
    std::stringstream ss;

    for (size_t c = 0; c < length; c++) {
        char ch = static_cast<char>(33 + getRandomG(26 * 2));
        ss << ch;
    }

    return ss.str();
}

}
