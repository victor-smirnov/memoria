
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BIT_vctr_BITMAP_RANK_TEST_HPP_
#define MEMORIA_TESTS_BIT_vctr_BITMAP_RANK_TEST_HPP_

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
class BitmapRankTest: public BitmapTestBase<T> {
    typedef BitmapRankTest<T>                                                       MyType;
    typedef BitmapTestBase<T>                                                       Base;


public:
    BitmapRankTest(StringRef name):
        Base(name)
    {
        MEMORIA_ADD_TEST(testPopCnt);
        MEMORIA_ADD_TEST(testPopCount);
    }


    void testPopCnt()
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

    void testPopCount()
    {
        T bitmap[10];
        Int bitsize     = sizeof(bitmap) * 8;

        MyType::clearBitmap(bitmap, bitsize, 0);

        for (Int length = 1; length <= bitsize; length++)
        {
            for (Int start = 0; start < bitsize - length; start++)
            {
                Int cnt = PopCount(bitmap, start, start + length);

                AssertEQ(MA_SRC, cnt, length, SBuf()<<start<<" "<<length);
            }
        }
    }
};


}
#endif
