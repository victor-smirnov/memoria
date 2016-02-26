
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_BIGINTEGER_CODEC_HPP_
#define MEMORIA_TESTS_PACKED_BIGINTEGER_CODEC_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_codecs_test_base.hpp"

#include <memoria/core/tools/strings/string_codec.hpp>

#include <memory>

namespace memoria {

class PackedBigIntegerCodecTest: public PackedCodecsTestBase<BigInteger> {

    using MyType = PackedBigIntegerCodecTest;
    using Base 	 = PackedCodecsTestBase<BigInteger>;

public:

    PackedBigIntegerCodecTest(StringRef name): Base(name)
    {
    	MEMORIA_ADD_TEST(testRawValues);
        MEMORIA_ADD_TEST(testShortValues);
        MEMORIA_ADD_TEST(testLongValues);
    }

    virtual ~PackedBigIntegerCodecTest() throw() {}

    void testRawValues()
    {
    	ValueCodec<BigInteger> codec;
    	UByte buffer[1000];

    	for (int c = -1000000; c < 1000000; c++)
    	{
    		auto len1 = codec.length(c);
    		auto len2 = codec.encode(buffer, c, 0);

    		BigInteger value;
    		auto len3 = codec.decode(buffer, value, 0);

    		AssertEQ(MA_RAW_SRC, c, value);
    		AssertEQ(MA_RAW_SRC, len1, len2);
    		AssertEQ(MA_RAW_SRC, len1, len3);
    	}
    }

    void testShortValues()
    {
    	size_t block_size = 1024*1024ull;

    	unique_ptr<UByte[]> buf = make_unique<UByte[]>(block_size);

    	ValueCodec<BigInteger> codec;

    	size_t cnt = 0;
    	for (size_t pos = 0; pos < block_size - 100; cnt++)
    	{
    		BigInteger value = pos;

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
    		BigInteger val;
    		auto len = codec.decode(buf.get(), val, pos);

    		AssertEQ(MA_SRC, val, pos);

    		pos += len;
    	}
    }


    void testLongValues()
    {
    	size_t block_size = 1024*1024ull;

    	unique_ptr<UByte[]> buf = make_unique<UByte[]>(block_size);

    	ValueCodec<BigInteger> codec;

    	size_t cnt = 0;
    	for (size_t pos = 0; pos < block_size - 1000; cnt++)
    	{
    		BigInteger value = replicate(pos, 20);

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
    		BigInteger val;
    		auto len = codec.decode(buf.get(), val, pos);

    		AssertEQ(MA_SRC, val, replicate(pos, 20));

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


}


#endif
