
// Copyright 2016 Victor Smirnov
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

#include "packed_codecs_test_base.hpp"

#include <memoria/core/bignum/bigint.hpp>

#include <memory>

namespace memoria {
namespace tests {

class PackedBigIntegerCodecTest: public PackedCodecsTestBase<BigInteger> {

    using MyType = PackedBigIntegerCodecTest;
    using Base   = PackedCodecsTestBase<BigInteger>;

public:

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TESTS(suite, testRawValues, testShortValues, testLongValues);
    }

    void testRawValues()
    {
        ValueCodec<BigInteger> codec;
        uint8_t buffer[1000];

        for (int c = -1000000; c < 1000000; c++)
        {
            auto len1 = codec.length(c);
            auto len2 = codec.encode(buffer, c, 0);

            BigInteger value;
            auto len3 = codec.decode(buffer, value, 0);

            assert_equals(c, value);
            assert_equals(len1, len2);
            assert_equals(len1, len3);
        }
    }

    void testShortValues()
    {
        size_t block_size = 1024*1024ull;

        std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(block_size);

        ValueCodec<BigInteger> codec;

        size_t cnt = 0;
        for (size_t pos = 0; pos < block_size - 100; cnt++)
        {
            BigInteger value = pos;

            auto len = codec.encode(buf.get(), value, pos);

            try {
                assert_equals(len, codec.length(value));
                assert_equals(len, codec.length(buf.get(), pos, -1ull));
            }
            catch(...) {
                std::cout << "Value: " << value << std::endl;
                throw;
            }

            pos += len;
        }

        for (size_t pos = 0, c = 0; c < cnt; c++)
        {
            BigInteger val;
            auto len = codec.decode(buf.get(), val, pos);

            assert_equals(val, pos);

            pos += len;
        }
    }


    void testLongValues()
    {
        size_t block_size = 1024*1024ull;

        std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(block_size);

        ValueCodec<BigInteger> codec;

        size_t cnt = 0;
        for (size_t pos = 0; pos < block_size - 1000; cnt++)
        {
            BigInteger value = replicate(pos, 20);

            auto len = codec.encode(buf.get(), value, pos);

            try {
                assert_equals(len, codec.length(value));
                assert_equals(len, codec.length(buf.get(), pos, -1ull));
            }
            catch(...) {
                std::cout << "Value: " << value << std::endl;
                throw;
            }

            pos += len;
        }

        for (size_t pos = 0, c = 0; c < cnt; c++)
        {
            BigInteger val;
            auto len = codec.decode(buf.get(), val, pos);

            assert_equals(val, replicate(pos, 20));

            pos += len;
        }
    }

private:
    BigInteger replicate(const BigInteger& st, size_t times)
    {
        BigInteger val;

        for (size_t c = 0; c < times; c++)
        {
            val *= st;
        }

        return val;
    }
};

#define MMA1_BIG_INTEGER_CODEC_SUITE() \
MMA1_CLASS_SUITE(PackedBigIntegerCodecTest, "PackedBigIntegerCodecSuite")

}}
