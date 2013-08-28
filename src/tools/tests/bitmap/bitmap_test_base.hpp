
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BIT_vctr_BITMAP_TEST_BASE_HPP_
#define MEMORIA_TESTS_BIT_vctr_BITMAP_TEST_BASE_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>
#include <memoria/core/tools/bitmap_select.hpp>

#include <vector>
#include <limits>
#include <functional>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename T>
class BitmapTestBase: public TestTask {
    typedef BitmapTestBase<T>                                                       MyType;
    typedef TestTask                                                                Base;



public:
    BitmapTestBase(StringRef name):
        Base(name)
    {}



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
        size_t bitsize  = TypeBitsize<T>();
        size_t mask     = TypeBitmask<T>();
        size_t suffix   = length & mask;

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
};


}
#endif
