
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_codecs_test_base.hpp"

#include <memoria/core/tools/strings/string_codec.hpp>

#include <memory>

namespace memoria {

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


}
