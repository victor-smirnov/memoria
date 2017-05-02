
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
class BitmapCountTest: public BitmapTestBase<T> {
    typedef BitmapCountTest<T>                                                  MyType;
    typedef BitmapTestBase<T>                                                   Base;



public:
    BitmapCountTest(StringRef name):
        Base(name)
    {
        MEMORIA_ADD_TEST(testCountOneFW);
        MEMORIA_ADD_TEST(testCountZeroFW);
        MEMORIA_ADD_TEST(testCountOneBW);
        MEMORIA_ADD_TEST(testCountZeroBW);
    }

    void testCountOneFW() {
        testCountFW(1);
    }


    void testCountZeroFW() {
        testCountFW(0);
    }


    void testCountFW(int32_t value)
    {
        Base::out()<<"TestCountFw: "<<TypeNameFactory<T>::name()<<" "<<value<<endl;

        T values[10];

        int32_t bitsize = sizeof(values) * 8;

        for (int32_t length = 0; length < bitsize; length++)
        {
            for (int32_t start = 0; start < bitsize - length; start++)
            {
                MyType::makeBitmap(values, bitsize, start, length, value);

                int32_t count;

                int32_t limit = start + length + 1;

                if (limit > bitsize)
                {
                    limit = bitsize;
                }

                if (value)
                {
                    count = CountOneFw(values, start, limit);
                }
                else {
                    count = CountZeroFw(values, start, limit);
                }

                AssertEQ(MA_SRC, count, length, SBuf()<<start<<" "<<length);
            }
        }
    }



    void testCountOneBW() {
        testCountBW(1);
    }


    void testCountZeroBW() {
        testCountBW(0);
    }


    void testCountBW(int32_t value)
    {
        Base::out()<<"TestCountBw: "<<TypeNameFactory<T>::name()<<" "<<value<<endl;

        T values[10];

        int32_t bitsize = sizeof(values) * 8;

        for (int32_t length = 0; length < bitsize; length++)
        {
            for (int32_t start = 0; start < bitsize - length; start++)
            {
                MyType::makeBitmap(values, bitsize, start, length, value);

                int32_t count;

                int32_t limit = start - 1;

                if (limit < 0)
                {
                    limit = 0;
                }

                int32_t begin = start + length;

                if (value)
                {
                    count = CountOneBw(values, begin, limit);
                }
                else {
                    count = CountZeroBw(values, begin, limit);
                }

                AssertEQ(MA_SRC, count, length, SBuf()<<start<<" "<<length);
            }
        }
    }
};


}}
