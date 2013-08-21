
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BIT_vctr_BITMAP_SPEED_TEST_HPP_
#define MEMORIA_TESTS_BIT_vctr_BITMAP_SPEED_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>
#include <memoria/core/tools/bitmap_select.hpp>
#include <memoria/core/tools/i64_codec.hpp>

#include "bitmap_test_base.hpp"

#include <vector>
#include <limits>
#include <functional>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename T>
class BitmapSpeedTest: public BitmapTestBase<T> {
    typedef BitmapSpeedTest<T>                                                  MyType;
    typedef BitmapTestBase<T>                                                   Base;



public:
    BitmapSpeedTest(StringRef name):
        Base(name)
    {
        MEMORIA_ADD_TEST(testSpeed1);
        MEMORIA_ADD_TEST(testSpeed2);

        MEMORIA_ADD_TEST(testI64Codec);
        MEMORIA_ADD_TEST(testEliasCodec);
    }

    void testSpeed1()
    {
    	Int bufsize = 1024*1024*128;
    	Int bitsize = bufsize * 8;

    	T* buf = T2T<T*>(malloc(bufsize));

    	BigInt t0 = getTimeInMillis();

    	MoveBits(buf, buf, 18, 29, bitsize - 100);

    	BigInt t1 = getTimeInMillis();

    	this->out()<<"Move time: "<<FormatTime(t1 - t0)<<endl;

    	free(buf);
    }

    void testSpeed2()
    {
    	Int bufsize = 1024*1024*128;
    	Int bitsize = bufsize * 8;

    	T* buf = T2T<T*>(malloc(bufsize));

    	BigInt t0 = getTimeInMillis();

    	Int pos = 0;
    	Int length = 17;

    	BigInt sum = 0;

    	Int cnt = 0;

    	while(pos < bitsize)
    	{
    		sum += GetBits(buf, pos, length);

    		pos += length;
    		cnt++;
    	}

    	BigInt t1 = getTimeInMillis();

    	this->out()<<"GetBits Time: "<<FormatTime(t1 - t0)<<" "<<sum<<" "<<cnt<<endl;

    	free(buf);
    }

    void testI64Codec()
    {
    	testCodec<I64Codec>(64);
    	testCodec<I64Codec>(1<<24);
    }

    void testEliasCodec()
    {
    	testCodec<EliasDeltaCodec>(64);
    	testCodec<EliasDeltaCodec>(1<<24);
    }

    template <
    	template <typename, typename> class Codec
    >
    void testCodec(Int max)
    {
    	std::vector<Int> values(1024*1024*4);
    	std::vector<Int> values2(1024*1024*4);

    	for (auto& v: values)
    	{
    		v = getRandom(max);
    	}

    	T* buf = T2T<T*>(malloc(1024*1024*16));

    	Codec<T,BigInt> codec;

    	BigInt t0 = getTimeInMillis();

    	for (Int c = 0, pos = 0; c < values.size(); c++)
    	{
    		pos += codec.encode(buf, values[c], pos);
    	}

    	BigInt t1 = getTimeInMillis();

    	for (Int c = 0, pos = 0; c < values2.size(); c++)
    	{
    		BigInt value;

    		pos += codec.decode(buf, value, pos);

    		values2[c] = value;
    	}

    	BigInt t2 = getTimeInMillis();

    	for (Int c = 0; c < values.size(); c++)
    	{
    		AssertEQ(MA_SRC, values[c], values2[c]);
    	}

    	this->out()<<"write: "<<FormatTime(t1 - t0)<<" read: "<<FormatTime(t2 - t1)<<endl;

    	free(buf);
    }
};


}
#endif
