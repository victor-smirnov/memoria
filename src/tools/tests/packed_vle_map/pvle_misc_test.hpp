
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_MISC_HPP_
#define MEMORIA_TESTS_PMAP_MISC_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_vle_tree.hpp>

#include <memoria/core/tools/exint_codec.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PVLEMapMiscTest: public TestTask {

	typedef PVLEMapMiscTest MyType;

public:

    PVLEMapMiscTest(): TestTask("Misc")
    {
    	MEMORIA_ADD_TEST(testExintCodec);
    }

    virtual ~PVLEMapMiscTest() throw() {}

    void testExintCodec()
    {
    	Int size = 4096;

    	UByte* bytes = T2T<UByte*>(malloc(size));

    	UBigInt value;

    	AssertEQ(MA_SRC, EncodeExint(bytes, 0, 0), 1u);
    	AssertEQ(MA_SRC, DecodeExint(bytes, value, 0), 1u);
    	AssertEQ(MA_SRC, value, 0u);

    	AssertEQ(MA_SRC, EncodeExint(bytes, 1ull, 0), 2u);
    	AssertEQ(MA_SRC, DecodeExint(bytes, value, 0), 2u);
    	AssertEQ(MA_SRC, value, 1u);

    	AssertEQ(MA_SRC, EncodeExint(bytes, 256ull, 0), 3u);
    	AssertEQ(MA_SRC, DecodeExint(bytes, value, 0), 3u);
    	AssertEQ(MA_SRC, value, 256u);


    	AssertEQ(MA_SRC, EncodeExint(bytes, 0x1FFFFull, 0), 4u);
    	AssertEQ(MA_SRC, DecodeExint(bytes, value, 0), 4u);
    	AssertEQ(MA_SRC, value, 0x1FFFFull);

    	AssertEQ(MA_SRC, EncodeExint(bytes, 0xFFFFFFFFFFFFFFFFull, 0), 9u);
    	AssertEQ(MA_SRC, DecodeExint(bytes, value, 0), 9u);
    	AssertEQ(MA_SRC, value, 0xFFFFFFFFFFFFFFFFull);


    	size_t pos = 0;
    	for (UInt c = 0; c < 1000; c++)
    	{
    		pos += EncodeExint(bytes, c, pos);
    	}

    	AssertLE(MA_SRC, pos, 4096u);

    	pos = 0;
    	for (UInt c = 0; c < 1000; c++)
    	{
    		UInt value;
    		pos += DecodeExint(bytes, value, pos);

    		AssertEQ(MA_SRC, value, c);
    	}
    }

};


}


#endif
