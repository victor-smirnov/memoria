
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

#include <memoria/v1/core/bignum/int64_codec.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace tests {

class PackedInt64TCodecTest: public PackedCodecsTestBase<int64_t> {

    using MyType = PackedInt64TCodecTest;
    using Base   = PackedCodecsTestBase<int64_t>;

    using Base::out;

public:

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TESTS(suite, testRawValues);
    }

    void testRawValues()
    {
        ValueCodec<int64_t> codec;
        uint8_t buffer[1000];

        for (int c = -1000000; c < 1000000; c++)
        {
            auto len1 = codec.length(c);
            auto len2 = codec.encode(buffer, c, 0);

            int64_t value;
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

        ValueCodec<int64_t> codec;

        size_t cnt = 0;
        for (size_t pos = 0; pos < block_size - 100; cnt++)
        {
            int64_t value = pos;

            auto len = codec.encode(buf.get(), value, pos);

            try {
                assert_equals(len, codec.length(value));
                assert_equals(len, codec.length(buf.get(), pos, -1ull));
            }
            catch(...) {
                out() << "Value: " << value << std::endl;
                throw;
            }

            pos += len;
        }

        for (size_t pos = 0, c = 0; c < cnt; c++)
        {
            int64_t val;
            auto len = codec.decode(buf.get(), val, pos);

            assert_equals(val, pos);

            pos += len;
        }
    }
};

#define MMA1_INT64_CODEC_SUITE() \
MMA1_CLASS_SUITE(PackedInt64TCodecTest, "PackedInt64TCodecSuite")

}}}
