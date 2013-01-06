
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BIT_VECTOR_BITMAP_TEST_HPP_
#define MEMORIA_TESTS_BIT_VECTOR_BITMAP_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include <memoria/core/tools/bitmap_select.hpp>

#include <vector>
#include <limits>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename T>
class BitmapTest: public TestTask {
    typedef BitmapTest                                                  		MyType;
    typedef TestTask                                                          	Base;


public:
    BitmapTest(StringRef name):
        Base(name)
    {
//        MEMORIA_ADD_TEST(testPopCnt);
//        MEMORIA_ADD_TEST(testMakeMaskInternal);
//        MEMORIA_ADD_TEST(testMakeMask);
//        MEMORIA_ADD_TEST(testSetBit);
//        MEMORIA_ADD_TEST(testSetBits);
//    	  MEMORIA_ADD_TEST(testPopCount);

//        MEMORIA_ADD_TEST(testCountOneFW);
//        MEMORIA_ADD_TEST(testCountZeroFW);
//        MEMORIA_ADD_TEST(testCountOneBW);
//        MEMORIA_ADD_TEST(testCountZeroBW);
//    	  MEMORIA_ADD_TEST(testMoveBits);

//    	MEMORIA_ADD_TEST(testSelectFWPlain);
    	MEMORIA_ADD_TEST(testSelectBWPlain);
//    	MEMORIA_ADD_TEST(testSelectFW);
    }


    void testPopCnt(ostream& out)
    {
    	for (Int c = 0; c < 100000; c++)
    	{
    		T v = sizeof(T) == 8 ? getBIRandom() : getRandom();

    		Int cnt = PopCnt(v);

    		if (sizeof(T) == 8)
    		{
    			AssertEQ(MA_SRC, cnt, __builtin_popcountl(v));
    		}
    		else {
    			AssertEQ(MA_SRC, cnt, __builtin_popcount(v));
    		}
    	}
    }

    void testPopCount(ostream& out)
    {
    	T bitmap[10];
    	Int bitsize 	= sizeof(bitmap) * 8;

    	clearBitmap(bitmap, bitsize, 0);

    	for (Int length = 1; length <= bitsize; length++)
    	{
    		for (Int start = 0; start < bitsize - length; start++)
    		{
    			Int cnt = PopCount(bitmap, start, start + length);

    			AssertEQ(MA_SRC, cnt, length, SBuf()<<start<<" "<<length);
    		}
    	}
    }

    template <typename TT>
    TT makeMask(int start, int length)
    {
    	TT value = 0;
    	TT one = 1;

    	for (Int c = 0; c < length; c++)
    	{
    		value = value | (one << (start + c));
    	}

    	return value;
    }

    void testMakeMaskInternal(ostream& out)
    {
    	UInt value = 0xF;

    	for (Int c = 0; c < 32 - 5; c++, value <<= 1)
    	{
    		AssertEQ(MA_SRC, makeMask<UInt>(c, 4), value);
    	}
    }

    void testMakeMask(ostream& out)
    {
    	out<<"MakeMask "<<TypeNameFactory<T>::name()<<endl;

    	Int bitsize = TypeBitsize<T>();

    	for (Int size = 0; size < bitsize; size++)
    	{
    		for (Int pos = 0; pos < bitsize - size; pos++)
    		{
    			T mask1 = MakeMask<T>(pos, size);
    			T mask2 = makeMask<T>(pos, size);

    			AssertEQ(MA_SRC, mask1, mask2, SBuf()<<size<<" "<<pos);
    		}
    	}
    }

    void testSetBit(ostream& out)
    {
    	out<<"TestSetBit: "<<TypeNameFactory<T>::name()<<endl;

    	T values[4];
    	Int bitsize = sizeof(values) * 8;

    	for (Int c = 0; c < bitsize; c++)
    	{
    		for (size_t d = 0; d < sizeof(values)/sizeof(T); d++)
    		{
    			values[d] = 0;
    		}

    		SetBit(values, c, 1);
    		Int value = GetBit(values, c);

    		AssertEQ(MA_SRC, value, 1);

    		for (size_t d = 0; d < sizeof(values)/sizeof(T); d++)
    		{
    			values[d] = static_cast<T>(-1);
    		}

			SetBit(values, c, 0);
			value = GetBit(values, c);

			AssertEQ(MA_SRC, value, 0, SBuf()<<c);
    	}
    }


    void testSetBits(ostream& out)
    {
    	out<<"TestSetBits: "<<TypeNameFactory<T>::name()<<endl;

    	T values[4];

    	Int bitsize 	= sizeof(values) * 8;
    	Int atomsize	= sizeof(T) * 8;

    	for (Int length = 1; length <= atomsize; length++)
    	{
    		for (Int start = 0; start < bitsize - length; start++)
    		{
    			for (size_t d = 0; d < sizeof(values)/sizeof(T); d++)
    			{
    				values[d] = sizeof(T) == 8 ? getBIRandom() : getRandom();
    			}

    			T bits1 = (sizeof(T) == 8 ? getBIRandom() : getRandom()) & MakeMask<T>(0, length);

    			SetBits(values, start, bits1, length);

    			for (Int c = 0; c < length; c++)
    			{
    				Int v1 = GetBit(values, start + c);
    				Int v2 = (bits1 >> c) & 0x1;

    				AssertEQ(MA_SRC, v1, v2, SBuf()<<(start + c));
    			}

    			T bits2 = GetBits(values, start, length);

    			for (Int c = 0; c < length; c++)
    			{
    				Int v1 = GetBit(values, start + c);
    				Int v2 = (bits2 >> c) & 0x1;

    				AssertEQ(MA_SRC, v1, v2, SBuf()<<(start + c));
    			}
    		}
    	}

    	for (Int c = 0; c < bitsize; c++)
    	{
    		for (size_t d = 0; d < sizeof(values)/sizeof(T); d++)
    		{
    			values[d] = 0;
    		}

    		SetBit(values, c, 1);
    		Int value = GetBit(values, c);

    		AssertEQ(MA_SRC, value, 1);

    		for (size_t d = 0; d < sizeof(values)/sizeof(T); d++)
    		{
    			values[d] = static_cast<T>(-1);
    		}

    		SetBit(values, c, 0);
    		value = GetBit(values, c);

    		AssertEQ(MA_SRC, value, 0, SBuf()<<c);
    	}
    }


    void clearBitmap(T* bitmap, Int size, Int value = 0)
    {
    	for (size_t c = 0; c < size/sizeof(T)/8; c++)
    	{
    		bitmap[c] = value ? 0 : static_cast<T>(-1);
    	}
    }


    void makeBitmap(T* buffer, Int size, Int start, Int length, Int value)
    {
    	clearBitmap(buffer, size, value);

    	for (Int c = 0; c < length; c++)
    	{
    		SetBit(buffer, c + start, value);
    	}
    }


    void makeRandomBitmap(T* buffer, Int size)
    {
    	for (size_t c = 0; c < size/sizeof(T)/8; c++)
    	{
    		buffer[c] = sizeof(T) == 8 ? getBIRandom() : getRandom();
    	}
    }


    void compareBitmaps(const char* src, const T* bitmap1, const T* bitmap2, Int start1, Int start2, Int length)
    {
    	size_t bitsize 	= TypeBitsize<T>();
    	size_t mask 	= TypeBitmask<T>();
    	size_t suffix 	= length & mask;

    	for (size_t c = 0; c < length - suffix; c += bitsize)
    	{
    		T v1 = GetBits(bitmap1, start1 + c, bitsize);
    		T v2 = GetBits(bitmap2, start2 + c, bitsize);

    		AssertEQ(src, v1, v2);
    	}

    	if (suffix > 0)
    	{
    		size_t c = length - suffix;

    		T v1 = GetBits(bitmap1, start1 + c, suffix);
    		T v2 = GetBits(bitmap2, start2 + c, suffix);

    		AssertEQ(src, v1, v2);
    	}
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
    			makeBitmap(values, bitsize, start, length, value);

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
    			makeBitmap(values, bitsize, start, length, value);

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



    void testMoveBits(ostream& out)
    {
    	T bitmap1[5];
    	T bitmap2[5];

    	Int bitsize 	= sizeof(bitmap1) * 8;

    	for (Int length = 0; length < bitsize; length++)
    	{
    		makeRandomBitmap(bitmap1, bitsize);

    		out<<"length: "<<length<<endl;

    		for (Int src_bit = 0; src_bit < bitsize - length; src_bit++)
    		{
    			for (Int dst_bit = 0; dst_bit < bitsize - length; dst_bit++)
    			{
    				MoveBitsFW(bitmap1, bitmap2, src_bit, dst_bit, length);

    				compareBitmaps(MA_SRC, bitmap1, bitmap2, src_bit, dst_bit, length);

    				clearBitmap(bitmap2, bitsize);

    				MoveBitsBW(bitmap1, bitmap2, src_bit, dst_bit, length);

    				compareBitmaps(MA_SRC, bitmap1, bitmap2, src_bit, dst_bit, length);
    			}
    		}
    	}
    }



    void testSelectFWPlain(ostream& out)
    {
    	UBigInt value = -1;
    	size_t bitsize = TypeBitsize<UBigInt>();
    	value >>= bitsize/2;

    	size_t total 	= 0;
    	size_t pos		= 0;

    	AssertFalse(MA_SRC, intrnl2::SelectFW(value, total, pos, pos));

    	for (size_t c = 1; c < bitsize / 2; c++)
    	{
    		total 	= 0;
    		pos		= c;

    		auto result = intrnl2::SelectFW(value, total, c, pos);

    		AssertTrue(MA_SRC, result);

    		AssertEQ(MA_SRC, total, c);
    		AssertEQ(MA_SRC, pos,   c - 1);
    	}

    	for (size_t c = 1; c < bitsize / 2; c++)
    	{
    		total 	= 0;
    		pos		= bitsize;

    		auto result = intrnl2::SelectFW(value, total, c, pos);

    		AssertTrue(MA_SRC, result);

    		AssertEQ(MA_SRC, total, c);
    		AssertEQ(MA_SRC, pos,   c - 1);
    	}

		total 	= 0;
		pos		= bitsize;

		auto result = intrnl2::SelectFW(value, total, bitsize/2 + 1, pos);

		AssertFalse(MA_SRC, result);
		AssertEQ(MA_SRC, total, bitsize/2);
		AssertEQ(MA_SRC, pos,   bitsize);

		total 	= 0;
		pos		= bitsize;

		result = intrnl2::SelectFW(value, total, bitsize/2, pos);

		AssertTrue(MA_SRC, result);
		AssertEQ(MA_SRC, total, bitsize/2);
		AssertEQ(MA_SRC, pos,   bitsize/2 - 1);
    }


    void testSelectBWPlain(ostream& out)
    {
    	size_t bitsize = TypeBitsize<UBigInt>();

    	UBigInt value = -1;
    	//value <<= bitsize/2;

    	size_t total 	= 0;
    	size_t delta;

    	for (size_t c = 1; c < bitsize; c++)
    	{
    		total 	= 0;

    		auto result = intrnl2::SelectBW(value, total, c, bitsize - 1,  delta);

    		AssertTrue(MA_SRC, result, SBuf()<<c);

    		AssertEQ(MA_SRC, total, c);
    		AssertEQ(MA_SRC, delta, c - 1);
    	}

    	for (size_t c = bitsize; c > 0; c--, value >>= 1)
    	{
    		total 	= 0;

    		auto result = intrnl2::SelectBW(value, total, c, bitsize - 1,  delta);

    		AssertTrue(MA_SRC, result);

    		AssertEQ(MA_SRC, total, c);
    		AssertEQ(MA_SRC, delta, bitsize - 1);
    	}

    	value = -1;

    	for (size_t c = bitsize; c > 0; c--, value >>= 1)
    	{
    		total 	= 0;

    		auto result = intrnl2::SelectBW(value, total, c, c - 1,  delta);

    		AssertTrue(MA_SRC, result);

    		AssertEQ(MA_SRC, total, c);
    		AssertEQ(MA_SRC, delta, c - 1);
    	}

    	value = 1;

    	for (size_t c = 0; c < bitsize; c++, value <<= 1)
    	{
    		total 	= 0;

    		auto result = intrnl2::SelectBW(value, total, 1, bitsize - 1,  delta);

    		AssertTrue(MA_SRC, result);

    		AssertEQ(MA_SRC, total, 1ul);
    		AssertEQ(MA_SRC, delta, bitsize - c - 1);
    	}

//
//    	total 	= 0;
//    	pos		= bitsize;
//
//    	auto result = intrnl2::SelectFW(value, total, bitsize/2 + 1, pos);
//
//    	AssertFalse(MA_SRC, result);
//    	AssertEQ(MA_SRC, total, bitsize/2);
//    	AssertEQ(MA_SRC, pos,   bitsize);
//
//    	total 	= 0;
//    	pos		= bitsize;
//
//    	result = intrnl2::SelectFW(value, total, bitsize/2, pos);
//
//    	AssertTrue(MA_SRC, result);
//    	AssertEQ(MA_SRC, total, bitsize/2);
//    	AssertEQ(MA_SRC, pos,   bitsize/2 - 1);
    }


    size_t select1FW(const T* bitmap, size_t start, size_t& stop, size_t count)
    {
    	size_t total = 0;

    	for (size_t c = start; c < stop; c++)
    	{
    		total += GetBit(bitmap, c);

    		if (total == count)
    		{
    			stop = c;
    			break;
    		}
    	}

    	return total;
    }

    size_t select0FW(const T* bitmap, size_t start, size_t& stop, size_t count)
    {
    	size_t total = 0;

    	for (size_t c = start; c < stop; c++)
    	{
    		total += 1 - GetBit(bitmap, c);

    		if (total == count)
    		{
    			stop = c;
    			break;
    		}
    	}

    	return total;
    }

    void testSelectFW(ostream& out)
    {
    	T bitmap[5];
    	size_t bitsize = sizeof(bitmap) * 8;

    	clearBitmap(bitmap, bitsize, 0);

    	for (size_t start = 0; start < bitsize; start++)
    	{
    		for (size_t count = 1; count < bitsize - start; count++)
    		{
        		auto result = Select1FW(bitmap, start, bitsize, count);

        		AssertEQ(MA_SRC, result.idx(), 	 start + count - 1, SBuf()<<start<<" "<<count);
        		AssertEQ(MA_SRC, result.count(), count, SBuf()<<start<<" "<<count);
    		}

    		for (size_t count = 1; count < bitsize - start; count++)
    		{
    			auto result = Select1FW(bitmap, start, start + count, count);

    			AssertEQ(MA_SRC, result.idx(), 	 start + count - 1, SBuf()<<start<<" "<<count);
    			AssertEQ(MA_SRC, result.count(), count, SBuf()<<start<<" "<<count);
    		}
    	}

    	makeRandomBitmap(bitmap, bitsize);


    	for (size_t start = 0; start < bitsize; start++)
    	{
    		for (size_t count = 1; count < bitsize - start; count++)
    		{
    			auto result = Select1FW(bitmap, start, bitsize, count);

    			size_t pos = bitsize;
    			size_t total = select1FW(bitmap, start, pos, count);

    			AssertEQ(MA_SRC, result.idx(), 	 pos, SBuf()<<start<<" "<<count);
    			AssertEQ(MA_SRC, result.count(), total, SBuf()<<start<<" "<<count);
    		}

    		for (size_t count = 1; count < bitsize - start; count++)
    		{
    			auto result = Select1FW(bitmap, start, start + count, count);

    			size_t pos = start + count;
    			size_t total = select1FW(bitmap, start, pos, count);

    			AssertEQ(MA_SRC, result.idx(), 	 pos, SBuf()<<start<<" "<<count);
    			AssertEQ(MA_SRC, result.count(), total, SBuf()<<start<<" "<<count);
    		}
    	}


    	clearBitmap(bitmap, bitsize, 1);

    	for (size_t start = 0; start < bitsize; start++)
    	{
    		for (size_t count = 1; count < bitsize - start; count++)
    		{
    			auto result = Select0FW(bitmap, start, bitsize, count);

    			AssertEQ(MA_SRC, result.idx(), 	 start + count - 1, SBuf()<<start<<" "<<count);
    			AssertEQ(MA_SRC, result.count(), count, SBuf()<<start<<" "<<count);
    		}

    		for (size_t count = 1; count < bitsize - start; count++)
    		{
    			auto result = Select0FW(bitmap, start, start + count, count);

    			AssertEQ(MA_SRC, result.idx(), 	 start + count - 1, SBuf()<<start<<" "<<count);
    			AssertEQ(MA_SRC, result.count(), count, SBuf()<<start<<" "<<count);
    		}
    	}

    	makeRandomBitmap(bitmap, bitsize);

    	for (size_t start = 0; start < bitsize; start++)
    	{
    		for (size_t count = 1; count < bitsize - start; count++)
    		{
    			auto result = Select0FW(bitmap, start, bitsize, count);

    			size_t pos = bitsize;
    			size_t total = select0FW(bitmap, start, pos, count);

    			AssertEQ(MA_SRC, result.idx(), 	 pos, SBuf()<<start<<" "<<count);
    			AssertEQ(MA_SRC, result.count(), total, SBuf()<<start<<" "<<count);
    		}

    		for (size_t count = 1; count < bitsize - start; count++)
    		{
    			auto result = Select0FW(bitmap, start, start + count, count);

    			size_t pos = start + count;
    			size_t total = select0FW(bitmap, start, pos, count);

    			AssertEQ(MA_SRC, result.idx(), 	 pos, SBuf()<<start<<" "<<count);
    			AssertEQ(MA_SRC, result.count(), total, SBuf()<<start<<" "<<count);
    		}
    	}
    }
};


}
#endif
