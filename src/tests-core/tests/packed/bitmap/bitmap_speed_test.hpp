
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

#include <memoria/core/tools/i64_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/memory/malloc.hpp>

#include "bitmap_test_base.hpp"

#include <vector>
#include <limits>
#include <functional>

namespace memoria {
namespace tests {


template <typename T>
class BitmapSpeedTest: public BitmapTestBase<T> {
    using MyType = BitmapSpeedTest<T>;
    using Base = BitmapTestBase<T>;

    using Base::getRandom;
    using Base::getBIRandom;
    using Base::out;

public:
    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testSpeed1, testSpeed2, testI64Codec, testEliasCodec);
    }

    void testSpeed1()
    {
        int32_t bufsize = 1024*1024*128;
        int32_t bitsize = bufsize * 8;

        T* buf = allocate_system<T>(bufsize).release();

        int64_t t0 = getTimeInMillis();

        MoveBits(buf, buf, 18, 29, bitsize - 100);

        int64_t t1 = getTimeInMillis();

        out() << "Move time: " << FormatTime(t1 - t0) << std::endl;

        free_system(buf);
    }

    void testSpeed2()
    {
        int32_t bufsize = 1024*1024*128;
        int32_t bitsize = bufsize * 8;

        T* buf = allocate_system<T>(bufsize).release();

        int64_t t0 = getTimeInMillis();

        int32_t pos = 0;
        int32_t length = 17;

        int64_t sum = 0;

        int32_t cnt = 0;

        while(pos < bitsize)
        {
            sum += GetBits(buf, pos, length);

            pos += length;
            cnt++;
        }

        int64_t t1 = getTimeInMillis();

        out() << "GetBits Time: " << FormatTime(t1 - t0) << " " << sum << " " << cnt << std::endl;

        free_system(buf);
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
    void testCodec(int32_t max)
    {
        std::vector<int32_t> values(1024*1024*4);
        std::vector<int32_t> values2(1024*1024*4);

        for (auto& v: values)
        {
            v = getRandom(max);
        }

        T* buf = allocate_system<T>(1024*1024*16).release();

        Codec<T,int64_t> codec;

        int64_t t0 = getTimeInMillis();

        for (int32_t c = 0, pos = 0; c < values.size(); c++)
        {
            pos += codec.encode(buf, values[c], pos);
        }

        int64_t t1 = getTimeInMillis();

        for (int32_t c = 0, pos = 0; c < values2.size(); c++)
        {
            int64_t value;

            pos += codec.decode(buf, value, pos);

            values2[c] = value;
        }

        int64_t t2 = getTimeInMillis();

        for (int32_t c = 0; c < values.size(); c++)
        {
            assert_equals(values[c], values2[c]);
        }

        out() << "write: " << FormatTime(t1 - t0) << " read: " << FormatTime(t2 - t1) << std::endl;

        free_system(buf);
    }
};



#define MMA_BITMAP_SPEED_SUITE(Name, Type) \
using Name = BitmapSpeedTest<Type>;\
MMA_CLASS_SUITE(Name, #Name)

}}
