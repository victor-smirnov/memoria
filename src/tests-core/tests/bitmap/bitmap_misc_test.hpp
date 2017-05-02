
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
#include <memoria/v1/core/tools/i64_codec.hpp>

#include "bitmap_test_base.hpp"

#include <vector>
#include <limits>
#include <functional>

namespace memoria {
namespace v1 {

using namespace std;

template <typename T>
class BitmapMiscTest: public BitmapTestBase<T> {
    typedef BitmapMiscTest<T>                                                   MyType;
    typedef BitmapTestBase<T>                                                   Base;

    using Base::getRandom;
    using Base::getBIRandom;


public:
    BitmapMiscTest(StringRef name):
        Base(name)
    {
        MEMORIA_ADD_TEST(testMakeMaskInternal);
        MEMORIA_ADD_TEST(testMakeMask);
        MEMORIA_ADD_TEST(testSetBit);
        MEMORIA_ADD_TEST(testSetBits);
        MEMORIA_ADD_TEST(testFillOne);
        MEMORIA_ADD_TEST(testFillZero);
        MEMORIA_ADD_TEST(testCTZ);
    }

    template <typename TT>
    TT makeMask(int start, int length)
    {
        TT value = 0;
        TT one = 1;

        for (int32_t c = 0; c < length; c++)
        {
            value = value | (one << (start + c));
        }

        return value;
    }

    void testMakeMaskInternal()
    {
        uint32_t value = 0xF;

        for (int32_t c = 0; c < 32 - 5; c++, value <<= 1)
        {
            AssertEQ(MA_SRC, makeMask<uint32_t>(c, 4), value);
        }
    }

    void testMakeMask()
    {
        Base::out()<<"MakeMask "<<TypeNameFactory<T>::name()<<endl;

        int32_t bitsize = TypeBitsize<T>();

        for (int32_t size = 0; size < bitsize; size++)
        {
            for (int32_t pos = 0; pos < bitsize - size; pos++)
            {
                T mask1 = MakeMask<T>(pos, size);
                T mask2 = makeMask<T>(pos, size);

                AssertEQ(MA_SRC, mask1, mask2, SBuf()<<size<<" "<<pos);
            }
        }
    }

    void testSetBit()
    {
        Base::out()<<"TestSetBit: "<<TypeNameFactory<T>::name()<<endl;

        T values[4];
        int32_t bitsize = sizeof(values) * 8;

        for (int32_t c = 0; c < bitsize; c++)
        {
            for (size_t d = 0; d < sizeof(values)/sizeof(T); d++)
            {
                values[d] = 0;
            }

            SetBit(values, c, 1);
            int32_t value = GetBit(values, c);

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


    void testSetBits()
    {
        Base::out()<<"TestSetBits: "<<TypeNameFactory<T>::name()<<endl;

        T values[4];

        int32_t bitsize     = sizeof(values) * 8;
        int32_t atomsize    = sizeof(T) * 8;

        for (int32_t length = 1; length <= atomsize; length++)
        {
            for (int32_t start = 0; start < bitsize - length; start++)
            {
                for (size_t d = 0; d < sizeof(values)/sizeof(T); d++)
                {
                    values[d] = sizeof(T) == 8 ? getBIRandom() : getRandom();
                }

                T bits1 = (sizeof(T) == 8 ? getBIRandom() : getRandom()) & MakeMask<T>(0, length);

                SetBits(values, start, bits1, length);

                for (int32_t c = 0; c < length; c++)
                {
                    int32_t v1 = GetBit(values, start + c);
                    int32_t v2 = (bits1 >> c) & 0x1;

                    AssertEQ(MA_SRC, v1, v2, SBuf()<<(start + c));
                }

                T bits2 = GetBits(values, start, length);

                for (int32_t c = 0; c < length; c++)
                {
                    int32_t v1 = GetBit(values, start + c);
                    int32_t v2 = (bits2 >> c) & 0x1;

                    AssertEQ(MA_SRC, v1, v2, SBuf()<<(start + c));
                }
            }
        }

        for (int32_t c = 0; c < bitsize; c++)
        {
            for (size_t d = 0; d < sizeof(values)/sizeof(T); d++)
            {
                values[d] = 0;
            }

            SetBit(values, c, 1);
            int32_t value = GetBit(values, c);

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

    void testMoveBits()
    {
        T bitmap1[5];
        T bitmap2[5];

        int32_t bitsize     = sizeof(bitmap1) * 8;

        for (int32_t length = 0; length < bitsize; length++)
        {
            makeRandomBitmap(bitmap1, bitsize);

            Base::out()<<"length: "<<length<<endl;

            for (int32_t src_bit = 0; src_bit < bitsize - length; src_bit++)
            {
                for (int32_t dst_bit = 0; dst_bit < bitsize - length; dst_bit++)
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

    void testFillOne()
    {
        T bitmap[10];

        size_t bitsize = sizeof(bitmap) * 8;

        for (int32_t c = 0; c < 1000; c++)
        {
            size_t start = getRandom(bitsize - 1);
            size_t stop  = getRandom(bitsize - start);

            MyType::clearBitmap(bitmap, bitsize, 1);

            FillOne(bitmap, start, stop);

            for (size_t idx = 0; idx < bitsize; idx++)
            {
                if (idx >= start && idx < stop)
                {
                    AssertEQ(MA_SRC, GetBit(bitmap, idx), 1);
                }
                else {
                    AssertEQ(MA_SRC, GetBit(bitmap, idx), 0);
                }
            }
        }
    }


    void testFillZero()
    {
        T bitmap[10];

        size_t bitsize = sizeof(bitmap) * 8;

        for (int32_t c = 0; c < 1000; c++)
        {
            size_t start = getRandom(bitsize - 1);
            size_t stop  = getRandom(bitsize - start);

            MyType::clearBitmap(bitmap, bitsize, 0);

            FillZero(bitmap, start, stop);

            for (size_t idx = 0; idx < bitsize; idx++)
            {
                if (idx >= start && idx < stop)
                {
                    AssertEQ(MA_SRC, GetBit(bitmap, idx), 0);
                }
                else {
                    AssertEQ(MA_SRC, GetBit(bitmap, idx), 1);
                }
            }
        }
    }

    void testCTZ()
    {
        uint64_t bitmap[5];

        for (int32_t c = 0; c < 1000000; c++)
        {
            memset(bitmap, 0, sizeof(bitmap));

            int32_t bit = getRandom(sizeof(bitmap)*8);
            int32_t start = getRandom(bit + 1);

            SetBit(bitmap, bit, 1);

            int32_t length = CountTrailingZeroes(bitmap, start, sizeof(bitmap) * 8);

            AssertEQ(MA_SRC, length, bit - start, SBuf()<<c);
        }
    }
};


}}
