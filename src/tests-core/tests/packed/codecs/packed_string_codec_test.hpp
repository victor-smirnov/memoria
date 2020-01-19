
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

#include <memoria/core/strings/string_codec.hpp>

#include <memory>

namespace memoria {
namespace tests {

class PackedStringCodecTest: public PackedCodecsTestBase<U8String> {

    using MyType = PackedStringCodecTest;
    using Base   = PackedCodecsTestBase<U8String>;

    using Base::out;

public:

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TESTS(suite, testShortStrings, testLongStrings);
    }

    void testShortStrings()
    {
        size_t block_size = 1024*1024ull;

        std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(block_size);

        ValueCodec<U8String> codec;

        size_t cnt = 0;
        for (size_t pos = 0; pos < block_size - 100; cnt++)
        {
            U8String value = toString(pos);

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
            U8String val;
            auto len = codec.decode(buf.get(), val, pos);

            assert_equals(val, toString(pos));

            pos += len;
        }
    }


    void testLongStrings()
    {
        size_t block_size = 1024*1024ull;

        std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(block_size);

        ValueCodec<U8String> codec;

        size_t cnt = 0;
        for (size_t pos = 0; pos < block_size - 1000; cnt++)
        {
            U8String value = replicate(toString(pos), 20);

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
            U8String val;
            auto len = codec.decode(buf.get(), val, pos);

            assert_equals(val, replicate(toString(pos), 20));

            pos += len;
        }
    }

private:
    U8String replicate(const U8String& st, size_t times)
    {
        U8String val;

        for (size_t c = 0; c < times; c++) {
            val += st;
        }

        return val;
    }
};

#define MMA1_STRING_CODEC_SUITE() \
MMA1_CLASS_SUITE(PackedStringCodecTest, "PackedStringCodecSuite")


}}
