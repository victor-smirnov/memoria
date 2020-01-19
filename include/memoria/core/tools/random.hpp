
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


#pragma once

#include <memoria/core/types.hpp>
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

    using result_type = T;

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

    void seed(T value)
    {
        std::seed_seq ss({value});
        engine_.seed(ss);
    }

    static constexpr T max() {
        return std::numeric_limits<T>::max();
    }

    static constexpr T min() {
        return 0;
    }
};

using RngInt   = RNG<int32_t, RngEngine32>;
using RngInt64 = RNG<int64_t, RngEngine64>;


RngInt& getGlobalIntGenerator();
RngInt64& getGlobalInt64Generator();

int32_t getRandomG();
int32_t getRandomG(int32_t max);
void    Seed(int32_t value);
int32_t getSeed();

int64_t  getBIRandomG();
int64_t  getBIRandomG(int64_t max);
void     SeedBI(int64_t value);
int64_t  getSeedBI();


int32_t getNonZeroRandomG(int32_t size);


}
