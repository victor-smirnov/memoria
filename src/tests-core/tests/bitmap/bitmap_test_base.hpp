
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

#include <vector>
#include <limits>
#include <functional>

namespace memoria {
namespace v1 {

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


}}