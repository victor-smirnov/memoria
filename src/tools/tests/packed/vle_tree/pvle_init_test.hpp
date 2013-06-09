
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_INIT_HPP_
#define MEMORIA_TESTS_PVLE_INIT_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_vle_tree.hpp>

#include <memoria/core/tools/exint_codec.hpp>

namespace memoria {

using namespace std;

class PVLEMapInitTest: public TestTask {

	typedef PVLEMapInitTest 				MyType;

	template <Int BF, Int VPB>
	using TreeType = PackedVLETree<PackedVLETreeTypes<Int, Int, UByteExintCodec, 2, BF, VPB>>;

public:

    PVLEMapInitTest(): TestTask("Init")
    {
    	MEMORIA_ADD_TEST(testInit);
    }

    virtual ~PVLEMapInitTest() throw() {}

    template <Int BF, Int VPB>
    void assertTree()
    {
    	typedef TreeType<BF, VPB> Tree;

    	for (Int max_size = 0; max_size < 128*1024; max_size += getRandom(10) + 1)
    	{
    		Int block_size = Tree::block_size(max_size);

    		Tree* tree = T2T<Tree*>(malloc(block_size));
    		memset(tree, 0, block_size);

    		tree->init(block_size);

    		AssertGE(MA_SRC, tree->max_data_size(), max_size);

    		free(tree);
    	}
    }

    void testInit()
    {
    	assertTree<32, 32>();
    	assertTree<32, 256>();
    	assertTree<32, 16>();

    	assertTree<12, 15>();
    	assertTree<82, 11>();
    	assertTree<9, 77>();
    }
};


}


#endif
