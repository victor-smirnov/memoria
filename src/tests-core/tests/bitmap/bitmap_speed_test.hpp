
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
class BitmapSpeedTest: public BitmapTestBase<T> {
    typedef BitmapSpeedTest<T>                                                  MyType;
    typedef BitmapTestBase<T>                                                   Base;

    using Base::getRandom;
    using Base::getBIRandom;

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


}}
