
// Copyright 2013 Victor Smirnov
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

#include "bitmap_test_base.hpp"

#include <vector>
#include <limits>
#include <functional>

namespace memoria {
namespace tests {

template <typename T>
class BitmapRankTest: public BitmapTestBase<T> {

    using MyType = BitmapRankTest;
    using Base = BitmapTestBase<T>;

    using Base::getRandom;
    using Base::getBIRandom;
    using Base::out;
    using Base::clearBitmap;

public:

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testPopCnt, testPopCount);
    }

    void testPopCnt()
    {
        for (size_t c = 0; c < 100000; c++)
        {
            T v = sizeof(T) == 8 ? getBIRandom() : getRandom();

            size_t cnt = PopCnt(v);

            if (sizeof(T) == 8)
            {
                assert_equals(cnt, __builtin_popcountl(v));
            }
            else {
                assert_equals(cnt, __builtin_popcount(v));
            }
        }
    }

    void testPopCount()
    {
        T bitmap[10];
        size_t bitsize     = sizeof(bitmap) * 8;

        clearBitmap(bitmap, bitsize, 0);

        for (size_t length = 1; length <= bitsize; length++)
        {
            for (size_t start = 0; start < bitsize - length; start++)
            {
                size_t cnt = PopCount(bitmap, start, start + length);

                assert_equals(cnt, length, "{} {}", start, length);
            }
        }
    }
};


#define MMA_BITMAP_RANK_SUITE(Name, Type) \
using Name = BitmapRankTest<Type>;\
MMA_CLASS_SUITE(Name, #Name)

}}
