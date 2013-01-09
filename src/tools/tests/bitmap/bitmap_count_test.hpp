
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BIT_VECTOR_BITMAP_COUNT_TEST_HPP_
#define MEMORIA_TESTS_BIT_VECTOR_BITMAP_COUNT_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>
#include <memoria/core/tools/bitmap_select.hpp>

#include "bitmap_test_base.hpp"

#include <vector>
#include <limits>
#include <functional>

namespace memoria {

using namespace memoria::vapi;
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

    void testCountOneFW(ostream& out) {
    	testCountFW(out, 1);
    }


    void testCountZeroFW(ostream& out) {
    	testCountFW(out, 0);
    }


    void testCountFW(ostream& out, Int value)
    {
    	out<<"TestCountFw: "<<TypeNameFactory<T>::name()<<" "<<value<<endl;

    	T values[10];

    	Int bitsize 	= sizeof(values) * 8;

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



    void testCountOneBW(ostream& out) {
    	testCountBW(out, 1);
    }


    void testCountZeroBW(ostream& out) {
    	testCountBW(out, 0);
    }


    void testCountBW(ostream& out, Int value)
    {
    	out<<"TestCountBw: "<<TypeNameFactory<T>::name()<<" "<<value<<endl;

    	T values[10];

    	Int bitsize 	= sizeof(values) * 8;

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
#endif
