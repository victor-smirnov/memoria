
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_PACKED_TREE_FIND_HPP_
#define MEMORIA_TESTS_PACKED_PACKED_TREE_FIND_HPP_

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
class PackedTreeFindTest: public PackedTreeTestBase <
	TreeType,
	CodecType,
	Blocks,
	VPB,
	BF
> {

	typedef PackedTreeFindTest<
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
	typedef typename Base::Value												Value;
	typedef typename Tree::IndexValue											IndexValue;

public:

	Int iterations_ = 1000;

	PackedTreeFindTest(StringRef name): Base(name)
    {
		MEMORIA_ADD_TEST_PARAM(iterations_);

		MEMORIA_ADD_TEST(testFindForward);
		MEMORIA_ADD_TEST(testFindBackward);
    }

    virtual ~PackedTreeFindTest() throw() {}

    template <template <typename, typename> class Comparator>
    IndexValue find_fw(const Tree* tree, Int block, Int start, Value limit)
    {
    	IndexValue sum = 0;

    	Int end = tree->size();
    	Comparator<IndexValue, IndexValue> compare;

    	for (Int c = start; c < end; c++)
    	{
    		Value value = tree->value(block, c);

    		if (compare(value, limit))
    		{
    			sum 	+= value;
    			limit 	-= value;
    		}
    		else {
    			return c;
    		}
    	}

    	return end;
    }

    template <template <typename, typename> class Comparator>
    IndexValue find_bw(const Tree* tree, Int block, Int start, Value limit)
    {
    	IndexValue sum = 0;

    	Comparator<IndexValue, IndexValue> compare;

    	for (Int c = start; c >= 0; c--)
    	{
    		Value value = tree->value(block, c);

    		if (compare(value, limit))
    		{
    			sum 	+= value;
    			limit 	-= value;
    		}
    		else {
    			return c;
    		}
    	}

    	return 0;
    }


    void testFindForward()
    {
    	for (Int c = 1; c <= 10; c++)
    	{
    		testFindForward(c * 1024);
    	}
    }

    void testFindForward(Int block_size)
    {
    	Base::out()<<block_size<<endl;

    	Tree* tree = Base::createEmptyTree(block_size);
    	PARemover remover(tree);

    	if (block_size == 1024) {
    		DebugCounter = 1;
    	}

    	auto values = Base::fillRandom(tree);

    	Int size = tree->size();

    	for (Int c = 0; c < iterations_; c++)
    	{
    		Int start 	= getRandom(size - 2);
    		Int rnd		= getRandom(size - start - 2);
    		Int end		= start + rnd + 2;

    		Int block	= getRandom(Tree::Blocks);

    		Int sum 	= tree->sum(block, start, end);

    		if (sum == 0) continue;

    		auto result1_lt = tree->findLTForward(block, start, sum).idx();
    		auto result1_le = tree->findLEForward(block, start, sum).idx();

    		auto result2_lt = find_fw<PackedCompareLE>(tree, block, start, sum);
    		auto result2_le = find_fw<PackedCompareLT>(tree, block, start, sum);

    		AssertEQ(MA_SRC, result1_lt, result2_lt, SBuf()<<start<<" "<<sum<<" "<<block);
    		AssertEQ(MA_SRC, result1_le, result2_le, SBuf()<<start<<" "<<sum<<" "<<block);
    	}
    }


    void testFindBackward()
    {
    	for (Int c = 1; c <= 10; c++)
    	{
    		testFindBackward(c * 1024);
    	}
    }

    void testFindBackward(Int block_size)
    {
    	Base::out()<<block_size<<endl;

    	Tree* tree = Base::createEmptyTree(block_size);
    	PARemover remover(tree);

    	auto values = Base::fillRandom(tree);

    	Int size = tree->size();

    	for (Int c = 0; c < iterations_; c++)
    	{
    		Int start 	= getRandom(size - 2) + 2;
    		Int rnd		= getRandom(start - 2) + 1;
    		Int end		= start - rnd;
    		Int block	= getRandom(Tree::Blocks);

    		AssertGE(MA_SRC, end, 0);

    		Int sum 	= tree->sum(block, end + 1, start + 1);

    		// we do not handle zero sums correctly in this test yet
    		if (sum == 0) continue;

    		auto result1_lt = tree->findLTBackward(block, start, sum).idx();
    		auto result1_le = tree->findLEBackward(block, start, sum).idx();

    		auto result2_lt = find_bw<PackedCompareLT>(tree, block, start, sum);
    		auto result2_le = find_bw<PackedCompareLE>(tree, block, start, sum);

    		AssertEQ(MA_SRC, result1_lt, result2_lt, SBuf()<<" - "<<start<<" "<<sum<<" "<<block);
    		AssertEQ(MA_SRC, result1_le, result2_le, SBuf()<<" - "<<start<<" "<<sum<<" "<<block);
    	}
    }
};


}


#endif
