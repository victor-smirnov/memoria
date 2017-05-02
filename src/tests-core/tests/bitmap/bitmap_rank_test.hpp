
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

#include <memoria/v1/memoria.hpp>
#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/core/tools/bitmap_select.hpp>

#include "bitmap_test_base.hpp"

#include <vector>
#include <limits>
#include <functional>

namespace memoria {
namespace v1 {

using namespace std;

template <typename T>
class BitmapRankTest: public BitmapTestBase<T> {
    typedef BitmapRankTest<T>                                                       MyType;
    typedef BitmapTestBase<T>                                                       Base;

    using Base::getRandom;
    using Base::getBIRandom;

public:
    BitmapRankTest(StringRef name):
        Base(name)
    {
        MEMORIA_ADD_TEST(testPopCnt);
        MEMORIA_ADD_TEST(testPopCount);
    }


    void testPopCnt()
    {
        for (int32_t c = 0; c < 100000; c++)
        {
            T v = sizeof(T) == 8 ? getBIRandom() : getRandom();

            int32_t cnt = PopCnt(v);

            if (sizeof(T) == 8)
            {
                AssertEQ(MA_SRC, cnt, __builtin_popcountl(v));
            }
            else {
                AssertEQ(MA_SRC, cnt, __builtin_popcount(v));
            }
        }
    }

    void testPopCount()
    {
        T bitmap[10];
        int32_t bitsize     = sizeof(bitmap) * 8;

        MyType::clearBitmap(bitmap, bitsize, 0);

        for (int32_t length = 1; length <= bitsize; length++)
        {
            for (int32_t start = 0; start < bitsize - length; start++)
            {
                int32_t cnt = PopCount(bitmap, start, start + length);

                AssertEQ(MA_SRC, cnt, length, SBuf()<<start<<" "<<length);
            }
        }
    }
};


}}
