
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


    void testCountFW(Int value)
    {
        Base::out()<<"TestCountFw: "<<TypeNameFactory<T>::name()<<" "<<value<<endl;

        T values[10];

        Int bitsize = sizeof(values) * 8;

        for (Int length = 0; length < bitsize; length++)
        {
            for (Int start = 0; start < bitsize - length; start++)
            {
                MyType::makeBitmap(values, bitsize, start, length, value);

                Int count;

                Int limit = start + length + 1;

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


    void testCountBW(Int value)
    {
        Base::out()<<"TestCountBw: "<<TypeNameFactory<T>::name()<<" "<<value<<endl;

        T values[10];

        Int bitsize = sizeof(values) * 8;

        for (Int length = 0; length < bitsize; length++)
        {
            for (Int start = 0; start < bitsize - length; start++)
            {
                MyType::makeBitmap(values, bitsize, start, length, value);

                Int count;

                Int limit = start - 1;

                if (limit < 0)
                {
                    limit = 0;
                }

                Int begin = start + length;

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
