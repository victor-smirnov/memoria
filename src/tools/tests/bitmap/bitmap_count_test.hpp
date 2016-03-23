
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/memoria.hpp>
#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/core/tools/bitmap_select.hpp>

#include "bitmap_test_base.hpp"

#include <vector>
#include <limits>
#include <functional>

namespace memoria {

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

        Int bitsize     = sizeof(values) * 8;

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

        Int bitsize     = sizeof(values) * 8;

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


}
