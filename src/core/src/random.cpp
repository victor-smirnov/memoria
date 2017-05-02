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


#include <memoria/v1/core/tools/random.hpp>

namespace memoria {
namespace v1 {

RngInt      int_generator;
RngInt64   bigint_generator;


RngInt& getGlobalIntGenerator() {
    return int_generator;
}

RngInt64& getGlobalInt64Generator() {
    return bigint_generator;
}

int32_t getRandomG()
{
    return int_generator();
}

int32_t getRandomG(int32_t max)
{
    return max > 0 ? int_generator() % max : 0;
}

void Seed(int32_t value)
{
    std::seed_seq ss({value});
    int_generator.engine().seed(ss);
}


int64_t getBIRandomG()
{
    return bigint_generator();
}

int64_t getBIRandomG(int64_t max)
{
    return max > 0 ? bigint_generator() % max : 0;
}

void SeedBI(int64_t value)
{
    std::seed_seq ss({value});
    bigint_generator.engine().seed(ss);
}


int32_t getNonZeroRandomG(int32_t size)
{
    int32_t value = getRandomG(size);
    return value != 0 ? value : getNonZeroRandomG(size);
}



}}