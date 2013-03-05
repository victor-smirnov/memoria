
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_INIT_HPP_
#define MEMORIA_TESTS_PVLE_INIT_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_vle_tree.hpp>
#include <memoria/core/packed/packed_sum_tree.hpp>

#include <memoria/core/tools/exint_codec.hpp>

namespace memoria {

using namespace std;

class PVLEMapInitTest: public TestTask {

	typedef PVLEMapInitTest 				MyType;
public:

    PVLEMapInitTest(): TestTask("Init")
    {
    	MEMORIA_ADD_TEST(testInit);
    }

    virtual ~PVLEMapInitTest() throw() {}

    template <Int BF, Int VPB>
    void testInitByBlock(Int block_size)
    {
    	typedef PackedVLETreeTypes<Int, Int, Int, EmptyResizeHandler, BF, VPB>	Types;
    	typedef PackedVLETree<Types> 											Tree;

    	Tree tree;

    	Int size = block_size - sizeof(Tree);
    	tree.initByBlock(size);

    	AssertLE(MA_SRC, tree.max_size(), size);
    	AssertGE(MA_SRC, tree.index_size(), 0);

    	AssertGE(MA_SRC, tree.getValueBlocks(), 0);
    }

    template <Int BF, Int VPB>
    void testInitByBlock()
    {
    	typedef PackedVLETreeTypes<Int, Int, Int, EmptyResizeHandler, BF, VPB>	Types;
    	typedef PackedVLETree<Types> 											Tree;

    	Tree tree;

    	tree.initByBlock(0);

    	AssertEQ(MA_SRC, tree.max_size(), 0);
    	AssertEQ(MA_SRC, tree.index_size(), 0);

    	AssertEQ(MA_SRC, tree.getValueBlocks(), 0);
    }

    void testInit()
    {
    	for (Int block_size = 16; block_size < 128*1024; block_size += getRandom(10)+1)
    	{
    		testInitByBlock<32, 32>(block_size);
    		testInitByBlock<32, 256>(block_size);
    		testInitByBlock<32, 16>(block_size);

    		testInitByBlock<12, 15>(block_size);
    		testInitByBlock<82, 11>(block_size);
    		testInitByBlock<9, 77>(block_size);
    	}

    	testInitByBlock<32, 32>();
    	testInitByBlock<32, 256>();
    	testInitByBlock<32, 16>();

    	testInitByBlock<12, 15>();
    	testInitByBlock<82, 11>();
    	testInitByBlock<9, 77>();
    }
};


}


#endif
