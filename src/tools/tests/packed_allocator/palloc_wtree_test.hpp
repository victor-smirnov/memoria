
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_WAVELETTREE_HPP_
#define MEMORIA_TESTS_PALLOC_WAVELETTREE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_dynamic_allocator.hpp>
#include <memoria/core/packed2/packed_bitvector.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PackedWaveletTreeTest: public TestTask {

	typedef TestTask														Base;
	typedef PackedWaveletTreeTest 											MyType;

	typedef PackedWaveletTreeTypes<> 										Types;

	typedef PackedWaveletTree<Types>										Tree;

	typedef shared_ptr<Tree>												TreePtr;

public:

	PackedWaveletTreeTest(): TestTask("WaveletTree")
    {
		MEMORIA_ADD_TEST(testAllocator);
    }

    virtual ~PackedWaveletTreeTest() throw() {}

    TreePtr createSequence(Int block_size, Int free_space)
    {
    	Int tree_block_size = Tree::block_size(block_size) + free_space;

    	void* memory_block = malloc(tree_block_size);
    	memset(memory_block, 0, tree_block_size);

    	Tree* tree = T2T<Tree*>(memory_block);

    	tree->init(tree_block_size);

    	return TreePtr(tree);
    }

    void testAllocator()
    {
    	TreePtr tree_ptr = createSequence(64*1024, 40*1024);
    	Tree* tree = tree_ptr.get();
    	tree->dump();

    	tree->bit_vector()->dump();

    	tree->bit_vector()->enlarge(100);

    	tree->dump();
    	tree->bit_vector()->dump();
    }

};


}


#endif
