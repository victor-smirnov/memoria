
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_PACKED_TREE_MISC_HPP_
#define MEMORIA_TESTS_PACKED_PACKED_TREE_MISC_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_tree_test_base.hpp"

namespace memoria {

using namespace std;

template <
	template <typename> class TreeType,
	template <typename> class CodecType = ValueFSECodec,
	Int Blocks		= 1,
	Int VPB 		= PackedTreeBranchingFactor,
	Int BF 			= PackedTreeBranchingFactor
>
class PackedTreeMiscTest: public PackedTreeTestBase <
	TreeType,
	CodecType,
	Blocks,
	VPB,
	BF
> {

	typedef PackedTreeMiscTest<
			TreeType,
			CodecType,
			Blocks,
			VPB,
			BF
	> 																			MyType;

	typedef PackedTreeTestBase <
		TreeType,
		CodecType,
		Blocks,
		VPB,
		BF
	>																			Base;

	typedef typename Base::Tree													Tree;
	typedef typename Base::Values												Values;

public:


	PackedTreeMiscTest(StringRef name): Base(name)
    {
//		MEMORIA_ADD_TEST(testInsertVector);

		MEMORIA_ADD_TEST(testFillTree);

    }

    virtual ~PackedTreeMiscTest() throw() {}

    void testInsertVector()
    {
    	for (int c = 1; c < 100; c+=10)
    	{
    		testInsertVector(c * 100);
    	}
    }

    void testInsertVector(Int size)
    {
    	Base::out()<<size<<std::endl;

    	Tree* tree = Base::createEmptyTree(1024*1024);
    	PARemover remover(tree);

    	vector<Values> v = Base::createRandomValuesVector(size);

    	Base::fillVector(tree, v);

    	Base::assertIndexCorrect(MA_SRC, tree);

    	Base::assertEqual(tree, v);
    }

    void testFillTree()
    {
    	for (int c = 4; c < 128; c*=2)
    	{
    		testFillTree(c * 1024);
    	}
    }

    void testFillTree(Int block_size)
    {
    	Base::out()<<block_size/1024<<"K"<<std::endl;

    	Tree* tree = Base::createEmptyTree(block_size);
    	PARemover remover(tree);

    	vector<Values> v = Base::fillRandom(tree);

    	Base::assertIndexCorrect(MA_SRC, tree);

    	Base::assertEqual(tree, v);
    }
};


}


#endif
