
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BIT_VECTOR_BITMAP_SELECT_TEST_HPP_
#define MEMORIA_TESTS_BIT_VECTOR_BITMAP_SELECT_TEST_HPP_

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


class BitmapSelectTest: public BitmapTestBase<UBigInt> {

	typedef UBigInt T;

    typedef BitmapSelectTest                                                 	MyType;
    typedef BitmapTestBase<T>                                             		Base;

    size_t  start_;
    size_t  stop_;

    size_t  target_idx_;
    size_t  target_rank_;
    bool  	target_found_;

    UBigInt value_ = 0;
    UBigInt rank_ = 0;

    T bitmap_[5];


    typedef function<void (MyType*, size_t, size_t, size_t)> 					AssertSelectBWFn;

public:
    BitmapSelectTest(StringRef name):
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

    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelectFWPlain, replaySelectFWPlain);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelectBWPlain, replaySelectBWPlain);

    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelect1FW, replaySelect1FW);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelect0FW, replaySelect0FW);

    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelect1BW, replaySelect1BW);
    	MEMORIA_ADD_TEST_WITH_REPLAY(testSelect0BW, replaySelect0BW);
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
