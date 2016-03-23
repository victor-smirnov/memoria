
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

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "packed_codecs_test_base.hpp"

#include <memoria/v1/core/tools/strings/string_codec.hpp>

#include <memory>

namespace memoria {
namespace v1 {

class PackedStringCodecTest: public PackedCodecsTestBase<String> {

    using MyType = PackedStringCodecTest;
    using Base   = PackedCodecsTestBase<String>;

public:

    PackedStringCodecTest(StringRef name): Base(name)
    {
        MEMORIA_ADD_TEST(testShortStrings);
        MEMORIA_ADD_TEST(testLongStrings);
    }

    virtual ~PackedStringCodecTest() throw() {}

    void testShortStrings()
    {
        size_t block_size = 1024*1024ull;

        unique_ptr<UByte[]> buf = make_unique<UByte[]>(block_size);

        ValueCodec<String> codec;

        size_t cnt = 0;
        for (size_t pos = 0; pos < block_size - 100; cnt++)
        {
            String value = toString(pos);

            auto len = codec.encode(buf.get(), value, pos);

            try {
                AssertEQ(MA_SRC, len, codec.length(value));
                AssertEQ(MA_SRC, len, codec.length(buf.get(), pos, -1ull));
            }
            catch(...) {
                cout << "Value: " << value << endl;
                throw;
            }

            pos += len;
        }

        for (size_t pos = 0, c = 0; c < cnt; c++)
        {
            String val;
            auto len = codec.decode(buf.get(), val, pos);

            AssertEQ(MA_SRC, val, toString(pos));

            pos += len;
        }
    }


    void testLongStrings()
    {
        size_t block_size = 1024*1024ull;

        unique_ptr<UByte[]> buf = make_unique<UByte[]>(block_size);

        ValueCodec<String> codec;

        size_t cnt = 0;
        for (size_t pos = 0; pos < block_size - 1000; cnt++)
        {
            String value = replicate(toString(pos), 20);

            auto len = codec.encode(buf.get(), value, pos);

            try {
                AssertEQ(MA_SRC, len, codec.length(value));
                AssertEQ(MA_SRC, len, codec.length(buf.get(), pos, -1ull));
            }
            catch(...) {
                cout << "Value: " << value << endl;
                throw;
            }

            pos += len;
        }

        for (size_t pos = 0, c = 0; c < cnt; c++)
        {
            String val;
            auto len = codec.decode(buf.get(), val, pos);

            AssertEQ(MA_SRC, val, replicate(toString(pos), 20));

            pos += len;
        }
    }

private:
    String replicate(const String& st, size_t times)
    {
        String val;

        for (size_t c = 0; c < times; c++) {
            val += st;
        }

        return val;
    }
};


}}