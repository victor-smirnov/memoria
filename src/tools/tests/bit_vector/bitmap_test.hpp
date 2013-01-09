
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
#include <functional>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename T = UBigInt>
class BitmapTest: public TestTask {
    typedef BitmapTest<T>                                                  		MyType;
    typedef TestTask                                                          	Base;

    size_t  start_;
    size_t  stop_;

    size_t  target_idx_;
    size_t  target_rank_;
    bool  	target_found_;

    UBigInt value_ = 0;
    UBigInt rank_ = 0;

    T bitmap_[5];


    typedef function<SelectResult (const T*, size_t, size_t, size_t)> 			SelectFn;

    typedef function<void (MyType*, size_t, size_t, size_t)> 							AssertSelectBWFn;

public:
    BitmapTest(StringRef name):
        Base(name)
    {
    	MEMORIA_ADD_TEST_PARAM(value_)->state();
    	MEMORIA_ADD_TEST_PARAM(rank_)->state();
    	MEMORIA_ADD_TEST_PARAM(start_)->state();
    	MEMORIA_ADD_TEST_PARAM(stop_)->state();
    	MEMORIA_ADD_TEST_PARAM(bitmap_)->state();

    	MEMORIA_ADD_TEST_PARAM(target_idx_)->state();
    	MEMORIA_ADD_TEST_PARAM(target_rank_)->state();
    	MEMORIA_ADD_TEST_PARAM(target_found_)->state();

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

    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelectFWPlain, replaySelectFWPlain);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelectBWPlain, replaySelectBWPlain);

    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelect1FW, replaySelect1FW);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelect0FW, replaySelect0FW);

    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelect1BW, replaySelect1BW);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelect0BW, replaySelect0BW);
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

    size_t selectFW(UBigInt x, Int rank)
    {
    	for (size_t c = 0; c < TypeBitsize<UBigInt>(); c++)
    	{
    		if (PopCnt(x & MakeMask<UBigInt>(0, c)) == rank)
    		{
    			return c;
    		}
    	}

    	return 100 + PopCnt(x);
    }

    size_t selectBW(UBigInt x, Int rank)
    {
    	Int bitsize = TypeBitsize<UBigInt>();

    	for (Int c = bitsize; c >= 0; c--)
    	{
    		UBigInt mask = MakeMask<UBigInt>(c, bitsize - c);
    		if (PopCnt(x & mask) == rank)
    		{
    			return c;
    		}
    	}

    	return 100 + PopCnt(x);
    }

    void testSelectFWPlain(ostream& out)
    {
    	size_t bitsize = TypeBitsize<UBigInt>();

    	for (Int c = 0; c < 10000; c++)
    	{
    		value_ = getBIRandom();

    		for (rank_ = 0; rank_ < bitsize; rank_++)
    		{
    			size_t pos1 = SelectFW(value_, rank_);
    			size_t pos2 = selectFW(value_, rank_);

    			AssertEQ(MA_SRC, pos1, pos2, SBuf()<<value_<<" "<<rank_);
    		}
    	}
    }

    void testSelectBWPlain(ostream& out)
    {
    	size_t bitsize = TypeBitsize<UBigInt>();

    	for (Int c = 0; c < 10000; c++)
    	{
    		value_ = getBIRandom();

    		for (rank_ = 0; rank_ < bitsize; rank_++)
    		{
    			size_t pos1 = SelectBW(value_, rank_);
    			size_t pos2 = selectBW(value_, rank_);

    			AssertEQ(MA_SRC, pos1, pos2, SBuf()<<value_<<" "<<rank_);
    		}
    	}
    }

    void dumpRanks(ostream& out, UBigInt value) const
    {
    	for (Int c = 0; c < 64; c++)
    	{
    		out<<c<<" ";
    		out<<PopCnt(value & MakeMask<UBigInt>(0, c))<<endl;
    	}
    }

    void replaySelectFWPlain(ostream& out)
    {
    	dumpRanks(out, value_);

		size_t pos1 = SelectFW(value_, rank_);
		size_t pos2 = selectFW(value_, rank_);

		AssertEQ(MA_SRC, pos1, pos2, SBuf()<<value_<<" "<<rank_);
    }

    void replaySelectBWPlain(ostream& out)
    {
    	dumpRanks(out, value_);

    	size_t pos1 = SelectBW(value_, rank_);
    	size_t pos2 = selectBW(value_, rank_);

    	AssertEQ(MA_SRC, pos1, pos2, SBuf()<<value_<<" "<<rank_);
    }


    SelectResult select1FW(const T* bitmap, size_t start, size_t stop, size_t rank)
    {
    	size_t total = 0;

    	for (size_t c = start; c < stop; c++)
    	{
    		if (total == rank)
    		{
    			return SelectResult(c, rank, true);
    		}

    		total += GetBit(bitmap, c);
    	}

    	return SelectResult(stop, total, false);
    }

    SelectResult select0FW(const T* bitmap, size_t start, size_t stop, size_t rank)
    {
    	size_t total = 0;

    	for (size_t c = start; c < stop; c++)
    	{
    		if (total == rank)
    		{
    			return SelectResult(c, rank, true);
    		}

    		total += 1 - GetBit(bitmap, c);
    	}

    	return SelectResult(stop, total, false);
    }

    SelectResult select1BW(const T* bitmap, size_t start, size_t stop, size_t rank)
    {
    	size_t total = 0;

    	for (size_t c = start; c > stop; c--)
    	{
    		total += GetBit(bitmap, c - 1);

    		if (total == rank)
    		{
    			return SelectResult(c - 1, rank, true);
    		}
    	}

    	return SelectResult(stop, total, false);
    }

    SelectResult select0BW(const T* bitmap, size_t start, size_t stop, size_t rank)
    {
    	size_t total = 0;

    	for (size_t c = start; c > stop; c--)
    	{
    		total += 1 - GetBit(bitmap, c - 1);

    		if (total == rank)
    		{
    			return SelectResult(c - 1, rank, true);
    		}
    	}

    	return SelectResult(stop, total, false);
    }


    void assertSelect1FW(size_t start, size_t stop, size_t rank)
    {
    	start_ = start;
    	stop_  = stop;
    	rank_  = rank;

    	auto result1 = Select1FW(bitmap_, start_, stop_, rank_);
    	auto result2 = select1FW(bitmap_, start_, stop_, rank_);

    	AssertEQ(MA_SRC, result1.is_found(),  result2.is_found(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    	AssertEQ(MA_SRC, result1.idx(),  result2.idx(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    	AssertEQ(MA_SRC, result1.rank(), result2.rank(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    }

    void assertSelect0FW(size_t start, size_t stop, size_t rank)
    {
    	start_ = start;
    	stop_  = stop;
    	rank_  = rank;

    	auto result1 = Select0FW(bitmap_, start_, stop_, rank_);
    	auto result2 = select0FW(bitmap_, start_, stop_, rank_);

    	AssertEQ(MA_SRC, result1.is_found(),  result2.is_found(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    	AssertEQ(MA_SRC, result1.idx(),  result2.idx(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    	AssertEQ(MA_SRC, result1.rank(), result2.rank(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    }


    void testSelect1FW(ostream& out)
    {
    	testSelectBW(out, &MyType::assertSelect1FW);
    }

    void testSelect0FW(ostream& out)
    {
    	testSelectBW(out, &MyType::assertSelect0FW);
    }

    void testSelectFW(ostream& out, AssertSelectBWFn assert_fn)
    {

    	size_t bitsize = sizeof(bitmap_) * 8;

    	clearBitmap(bitmap_, bitsize, 0);

    	for (size_t start = 0; start < bitsize; start++)
    	{
    		for (size_t rank = 0; rank < bitsize - start; rank++)
    		{
    			assert_fn(this, start, bitsize, rank);
    		}

    		for (size_t rank = 0; rank < bitsize - start; rank++)
    		{
    			assert_fn(this, start, start + rank, rank);
    		}
    	}

    	makeRandomBitmap(bitmap_, bitsize);


    	for (size_t start = 0; start < bitsize; start++)
    	{
    		for (size_t rank = 0; rank < bitsize - start; rank++)
    		{
    			assert_fn(this, start, bitsize, rank);
    		}

    		for (size_t rank = 0; rank < bitsize - start; rank++)
    		{
    			assert_fn(this, start, start + rank, rank);
    		}
    	}
    }

    void replaySelect1FW(ostream& out)
    {

    }

    void replaySelect0FW(ostream& out)
    {

    }


    void assertSelect1BW(size_t start, size_t stop, size_t rank)
    {
    	start_ = start;
    	stop_  = stop;
    	rank_  = rank;

    	auto result1 = Select1BW(bitmap_, start_, stop_, rank_);
    	auto result2 = select1BW(bitmap_, start_, stop_, rank_);

    	AssertEQ(MA_SRC, result1.is_found(),  result2.is_found(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    	AssertEQ(MA_SRC, result1.idx(),  result2.idx(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    	AssertEQ(MA_SRC, result1.rank(), result2.rank(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    }

    void assertSelect0BW(size_t start, size_t stop, size_t rank)
    {
    	start_ = start;
    	stop_  = stop;
    	rank_  = rank;

    	auto result1 = Select0BW(bitmap_, start_, stop_, rank_);
    	auto result2 = select0BW(bitmap_, start_, stop_, rank_);

    	AssertEQ(MA_SRC, result1.is_found(),  result2.is_found(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    	AssertEQ(MA_SRC, result1.idx(),  result2.idx(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    	AssertEQ(MA_SRC, result1.rank(), result2.rank(), SBuf()<<start_<<" "<<stop_<<" "<<rank_);
    }

    void testSelect1BW(ostream& out)
    {
    	testSelectBW(out, &MyType::assertSelect1BW);
    }

    void testSelect0BW(ostream& out)
    {
    	testSelectBW(out, &MyType::assertSelect0BW);
    }

    void testSelectBW(ostream& out, AssertSelectBWFn assert_fn)
    {
    	size_t bitsize = sizeof(bitmap_) * 8;

    	clearBitmap(bitmap_, bitsize, 0);

    	size_t bitmap_rank = PopCount(bitmap_, 0, bitsize);

    	for (size_t start = bitsize; start > 0; start--)
    	{
    		for (size_t rank = bitmap_rank; rank > 0; rank--)
    		{
    			assert_fn(this, start, 0, rank);
    		}
    	}

    	for (size_t start = bitsize; start > 0; start--)
    	{
    		for (size_t rank = bitmap_rank; rank > 0; rank--)
    		{
    			size_t stop  = start >= rank? start - rank : 0;

    			assert_fn(this, start, stop, rank);
    		}
    	}

    	for (size_t start = bitsize; start > 0; start--)
    	{
    		for (size_t rank = bitmap_rank; rank > 0; rank--)
    		{
    			assert_fn(this, start, 0, bitmap_rank + 10);
    		}
    	}

    	for (size_t start = bitsize; start > 0; start--)
    	{
    		for (size_t rank = bitmap_rank; rank > 0; rank--)
    		{
    			size_t stop  = start >= rank? start - rank : 0;

    			assert_fn(this, start, stop, bitmap_rank + 10);
    		}
    	}


    	makeRandomBitmap(bitmap_, bitsize);

    	bitmap_rank = PopCount(bitmap_, 0, bitsize);

    	for (size_t start = bitsize; start > 0; start--)
    	{
    		for (size_t rank = bitmap_rank; rank > 0; rank--)
    		{
    			assert_fn(this, start, 0, rank);
    		}
    	}

    	for (size_t start = bitsize; start > 0; start--)
    	{
    		for (size_t rank = bitmap_rank; rank > 0; rank--)
    		{
    			size_t stop  = start >= rank? start - rank : 0;

    			assert_fn(this, start, stop, rank);
    		}
    	}


    	for (size_t start = bitsize; start > 0; start--)
    	{
    		for (size_t rank = bitmap_rank; rank > 0; rank--)
    		{
    			assert_fn(this, start, 0, bitmap_rank + 10);
    		}
    	}

    	for (size_t start = bitsize; start > 0; start--)
    	{
    		for (size_t rank = bitmap_rank; rank > 0; rank--)
    		{
    			size_t stop  = start >= rank? start - rank : 0;

    			assert_fn(this, start, stop, bitmap_rank + 10);
    		}
    	}
    }

    void replaySelect1BW(ostream& out)
    {
		auto result1 = Select1BW(bitmap_, start_, stop_, rank_);
		auto result2 = select1BW(bitmap_, start_, stop_, rank_);

		AssertEQ(MA_SRC, result1.is_found(), result2.is_found());
		AssertEQ(MA_SRC, result1.idx(), 	 result2.idx());
		AssertEQ(MA_SRC, result1.rank(), 	 result2.rank());
    }

    void replaySelect0BW(ostream& out)
    {
		auto result1 = Select0BW(bitmap_, start_, stop_, rank_);
		auto result2 = select0BW(bitmap_, start_, stop_, rank_);

		AssertEQ(MA_SRC, result1.is_found(), result2.is_found());
		AssertEQ(MA_SRC, result1.idx(), 	 result2.idx());
		AssertEQ(MA_SRC, result1.rank(), 	 result2.rank());
    }
};


}
#endif
