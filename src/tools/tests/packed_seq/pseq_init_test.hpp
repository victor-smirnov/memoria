
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQ_PSEQ_INIT_TEST_HPP_
#define MEMORIA_TESTS_PACKED_SEQ_PSEQ_INIT_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_seq.hpp>

#include <memory>

namespace memoria {

using namespace std;


class PSeqInitTest: public TestTask {

    typedef PSeqInitTest MyType;

public:

    PSeqInitTest(): TestTask("Init")
    {
        MEMORIA_ADD_TEST(runTest<1>);
        MEMORIA_ADD_TEST(runTest<2>);
        MEMORIA_ADD_TEST(runTest<3>);
        MEMORIA_ADD_TEST(runTest<4>);
        MEMORIA_ADD_TEST(runTest<5>);
        MEMORIA_ADD_TEST(runTest<6>);
        MEMORIA_ADD_TEST(runTest<7>);
        MEMORIA_ADD_TEST(runTest<8>);

    }

    virtual ~PSeqInitTest() throw() {}

    template <Int Bits>
    void runTest(ostream& out)
    {
    	typedef PackedSeqTypes<Int, UBigInt, Bits> Types;
    	PackedSeq<Types> seq;

    	for (Int block_size = 512 * Bits; block_size < 100000; block_size++)
    	{
    		seq.initByBlock(block_size);

    		AssertLE(MA_SRC, seq.getBlockSize(), block_size);
    	}
    }
};


}


#endif
