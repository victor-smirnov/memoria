
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
namespace v1 {
namespace tests {



class BitmapSelectTest: public BitmapTestBase<uint64_t> {

    using T = uint64_t;

    using MyType = BitmapSelectTest;
    using Base = BitmapTestBase<T>;

    size_t  start_{};
    size_t  stop_{};

    size_t  target_idx_{};
    size_t  target_rank_{};
    bool    target_found_{};

    uint64_t value_{};
    uint64_t rank_{};

    T bitmap_[5];

    using AssertSelectBWFn = std::function<void (MyType*, size_t, size_t, size_t)>;

public:
    MMA1_STATE_FILEDS(start_, stop_, target_idx_, target_rank_, target_found_, value_, rank_); //bitmap_

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TESTS(suite, testSelectFWPlain, testSelectBWPlain, testSelect1FW, testSelect0FW, testSelect1BW, testSelect0BW)\
    }


    size_t selectFW(uint64_t x, int32_t rank)
    {
        for (size_t c = 0; c <= TypeBitsize<uint64_t>(); c++)
        {
            if (PopCnt(x & MakeMask<uint64_t>(0, c)) == rank)
            {
                return c - 1;
            }
        }

        return 100 + PopCnt(x);
    }

    size_t selectBW(uint64_t x, int32_t rank)
    {
        size_t bitsize = TypeBitsize<uint64_t>();

        for (size_t c = 1; c <= bitsize; c++)
        {
            uint64_t mask = MakeMask<uint64_t>(bitsize - c, c);
            if (PopCnt(x & mask) == rank)
            {
                return bitsize - c;
            }
        }

        return 100 + PopCnt(x);
    }


    void AssertSelectFW(uint64_t value, size_t rank)
    {
        value_ = value;
        rank_  = rank;

        size_t pos1 = SelectFW(value_, rank_);
        size_t pos2 = selectFW(value_, rank_);

        assert_equals(pos1, pos2, u"{} {}", value_, rank_);
    }

    void AssertSelectBW(uint64_t value, size_t rank)
    {
        value_ = value;
        rank_  = rank;

        size_t pos1 = SelectBW(value_, rank_);
        size_t pos2 = selectBW(value_, rank_);

        assert_equals(pos1, pos2, u"{} {}", value_, rank_);
    }


    void testSelectFWPlain()
    {
        AssertSelectFW(-1ull, 64);

        size_t bitsize = TypeBitsize<uint64_t>();

        for (int32_t c = 0; c < 10000; c++)
        {
            uint64_t value = getBIRandom();

            for (size_t rank = 1; rank <= bitsize; rank++)
            {
                AssertSelectFW(value, rank);
            }
        }
    }



    void testSelectBWPlain()
    {
        AssertSelectBW(-1ull, 64);
        AssertSelectBW(-1ull, 1);

        size_t bitsize = TypeBitsize<uint64_t>();

        for (int32_t c = 0; c < 10000; c++)
        {
            uint64_t value = getBIRandom();

            for (size_t rank = 1; rank <= bitsize; rank++)
            {
                AssertSelectBW(value, rank);
            }
        }
    }

    void dumpRanks(uint64_t value)
    {
        for (int32_t c = 0; c < 64; c++)
        {
            out() << c << " ";
            out() << PopCnt(value & MakeMask<uint64_t>(0, c)) << std::endl;
        }
    }

    void replaySelectFWPlain()
    {
        dumpRanks(value_);

        size_t pos1 = SelectFW(value_, rank_);
        size_t pos2 = selectFW(value_, rank_);

        assert_equals(pos1, pos2, u"{} {}", value_, rank_);
    }

    void replaySelectBWPlain()
    {
        dumpRanks(value_);

        size_t pos1 = SelectBW(value_, rank_);
        size_t pos2 = selectBW(value_, rank_);

        assert_equals(pos1, pos2, u"{} {}", value_, rank_);
    }


    SelectResult select1FW(const T* bitmap, size_t start, size_t stop, size_t rank)
    {
        size_t total = 0;

        for (size_t c = start; c < stop; c++)
        {
            total += GetBit(bitmap, c);

            if (total == rank)
            {
                return SelectResult(c, rank, true);
            }
        }

        return SelectResult(stop, total, false);
    }

    SelectResult select0FW(const T* bitmap, size_t start, size_t stop, size_t rank)
    {
        size_t total = 0;

        for (size_t c = start; c < stop; c++)
        {
            total += 1 - GetBit(bitmap, c);

            if (total == rank)
            {
                return SelectResult(c, rank, true);
            }
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

        assert_equals(result1.is_found(),  result2.is_found(), u"{} {} {}", start_, stop_, rank_);
        assert_equals(result1.local_pos(),  result2.local_pos(), u"{} {} {}", start_, stop_, rank_);
        assert_equals(result1.rank(), result2.rank(), u"{} {} {}", start_, stop_, rank_);
    }

    void assertSelect0FW(size_t start, size_t stop, size_t rank)
    {
        start_ = start;
        stop_  = stop;
        rank_  = rank;

        auto result1 = Select0FW(bitmap_, start_, stop_, rank_);
        auto result2 = select0FW(bitmap_, start_, stop_, rank_);

        assert_equals(result1.is_found(),  result2.is_found(), u"{} {} {}", start_, stop_, rank_);
        assert_equals(result1.local_pos(),  result2.local_pos(), u"{} {} {}", start_, stop_, rank_);
        assert_equals(result1.rank(), result2.rank(), u"{} {} {}", start_, stop_, rank_);
    }


    void testSelect1FW()
    {
        testSelectFW(&MyType::assertSelect1FW);
    }

    void testSelect0FW()
    {
        testSelectBW(&MyType::assertSelect0FW);
    }

    void testSelectFW(AssertSelectBWFn assert_fn)
    {

        size_t bitsize = sizeof(bitmap_) * 8;

        clearBitmap(bitmap_, bitsize, 0);

        for (size_t start = 0; start < bitsize; start++)
        {
            for (size_t rank = 1; rank < bitsize - start; rank++)
            {
                assert_fn(this, start, bitsize, rank);
            }

            for (size_t rank = 1; rank < bitsize - start; rank++)
            {
                assert_fn(this, start, start + rank, rank);
            }
        }

        makeRandomBitmap(bitmap_, bitsize);


        for (size_t start = 0; start < bitsize; start++)
        {
            for (size_t rank = 1; rank < bitsize - start; rank++)
            {
                assert_fn(this, start, bitsize, rank);
            }

            for (size_t rank = 1; rank < bitsize - start; rank++)
            {
                assert_fn(this, start, start + rank, rank);
            }
        }
    }

    void replaySelect1FW()
    {

    }

    void replaySelect0FW()
    {

    }


    void assertSelect1BW(size_t start, size_t stop, size_t rank)
    {
        start_ = start;
        stop_  = stop;
        rank_  = rank;

        auto result1 = Select1BW(bitmap_, start_, stop_, rank_);
        auto result2 = select1BW(bitmap_, start_, stop_, rank_);

        assert_equals(result1.is_found(),  result2.is_found(), u"{} {} {}", start_, stop_, rank_);
        assert_equals(result1.local_pos(),  result2.local_pos(), u"{} {} {}", start_, stop_, rank_);
        assert_equals(result1.rank(), result2.rank(), u"{} {} {}", start_, stop_, rank_);
    }

    void assertSelect0BW(size_t start, size_t stop, size_t rank)
    {
        start_ = start;
        stop_  = stop;
        rank_  = rank;

        auto result1 = Select0BW(bitmap_, start_, stop_, rank_);
        auto result2 = select0BW(bitmap_, start_, stop_, rank_);

        assert_equals(result1.is_found(),  result2.is_found(), u"{} {} {}", start_, stop_, rank_);
        assert_equals(result1.local_pos(),  result2.local_pos(), u"{} {} {}", start_, stop_, rank_);
        assert_equals(result1.rank(), result2.rank(), u"{} {} {}", start_, stop_, rank_);
    }

    void testSelect1BW()
    {
        testSelectBW(&MyType::assertSelect1BW);
    }

    void testSelect0BW()
    {
        testSelectBW(&MyType::assertSelect0BW);
    }

    void testSelectBW(AssertSelectBWFn assert_fn)
    {
        size_t bitsize = sizeof(bitmap_) * 8;

        clearBitmap(bitmap_, bitsize, 0);

        size_t bitmaprank_ = PopCount(bitmap_, 0, bitsize);

        for (size_t start = bitsize; start > 0; start--)
        {
            for (size_t rank = bitmaprank_; rank > 0; rank--)
            {
                assert_fn(this, start, 0, rank);
            }
        }

        for (size_t start = bitsize; start > 0; start--)
        {
            for (size_t rank = bitmaprank_; rank > 0; rank--)
            {
                size_t stop  = start >= rank? start - rank : 0;

                assert_fn(this, start, stop, rank);
            }
        }

        for (size_t start = bitsize; start > 0; start--)
        {
            for (size_t rank = bitmaprank_; rank > 0; rank--)
            {
                assert_fn(this, start, 0, bitmaprank_ + 10);
            }
        }

        for (size_t start = bitsize; start > 0; start--)
        {
            for (size_t rank = bitmaprank_; rank > 0; rank--)
            {
                size_t stop  = start >= rank? start - rank : 0;

                assert_fn(this, start, stop, bitmaprank_ + 10);
            }
        }


        makeRandomBitmap(bitmap_, bitsize);

        bitmaprank_ = PopCount(bitmap_, 0, bitsize);

        for (size_t start = bitsize; start > 0; start--)
        {
            for (size_t rank = bitmaprank_; rank > 0; rank--)
            {
                assert_fn(this, start, 0, rank);
            }
        }

        for (size_t start = bitsize; start > 0; start--)
        {
            for (size_t rank = bitmaprank_; rank > 0; rank--)
            {
                size_t stop  = start >= rank? start - rank : 0;

                assert_fn(this, start, stop, rank);
            }
        }


        for (size_t start = bitsize; start > 0; start--)
        {
            for (size_t rank = bitmaprank_; rank > 0; rank--)
            {
                assert_fn(this, start, 0, bitmaprank_ + 10);
            }
        }

        for (size_t start = bitsize; start > 0; start--)
        {
            for (size_t rank = bitmaprank_; rank > 0; rank--)
            {
                size_t stop  = start >= rank? start - rank : 0;

                assert_fn(this, start, stop, bitmaprank_ + 10);
            }
        }
    }

    void replaySelect1BW()
    {
        auto result1 = Select1BW(bitmap_, start_, stop_, rank_);
        auto result2 = select1BW(bitmap_, start_, stop_, rank_);

        assert_equals(result1.is_found(), result2.is_found());
        assert_equals(result1.local_pos(), result2.local_pos());
        assert_equals(result1.rank(),     result2.rank());
    }

    void replaySelect0BW()
    {
        auto result1 = Select0BW(bitmap_, start_, stop_, rank_);
        auto result2 = select0BW(bitmap_, start_, stop_, rank_);

        assert_equals(result1.is_found(), result2.is_found());
        assert_equals(result1.local_pos(), result2.local_pos());
        assert_equals(result1.rank(),     result2.rank());
    }
};


#define MMA1_BITMAP_SELECT_SUITE() \
MMA1_CLASS_SUITE(BitmapSelectTest, u"BitmapSelectSuite")


}}}
