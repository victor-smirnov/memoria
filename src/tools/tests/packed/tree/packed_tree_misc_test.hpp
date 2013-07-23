
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

	Int iterations_ = 10;

public:


	PackedTreeMiscTest(StringRef name): Base(name)
    {
		MEMORIA_ADD_TEST_PARAM(iterations_);

		MEMORIA_ADD_TEST(testInsertVector);

		MEMORIA_ADD_TEST(testFillTree);

		MEMORIA_ADD_TEST(testAddValue);

		MEMORIA_ADD_TEST(testMerge);

		MEMORIA_ADD_TEST(testSplitToEmpty);
		MEMORIA_ADD_TEST(testSplitToPreFilled);

		MEMORIA_ADD_TEST(testRemoveMulti);
		MEMORIA_ADD_TEST(testRemoveAll);

		MEMORIA_ADD_TEST(testClear);
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
    	for (int c = 1; c < 128; c*=2)
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

    void testAddValue()
    {
    	for (int c = 1; c <= 32*1024; c*=2)
    	{
    		testAddValue(c);
    	}
    }

    void addValues(vector<Values>& values, Int idx, const Values v)
    {
    	for (Int c = 0; c< Blocks; c++)
    	{
    		values[idx][c] += v[c];
    	}
    }


    void testAddValue(Int size)
    {
    	Base::out()<<size<<std::endl;

    	Tree* tree = Base::createEmptyTree(1024*1024);
    	PARemover remover(tree);

    	auto tree_values = Base::createRandomValuesVector(size);

    	Base::fillVector(tree, tree_values);

    	for (Int c = 0; c < iterations_; c++)
    	{
    		Values value = Base::createRandom();
    		Int idx = getRandom(tree->size());

    		tree->addValues(idx, value);
    		Base::assertIndexCorrect(MA_SRC, tree);

    		addValues(tree_values, idx, value);

    		Base::assertEqual(tree, tree_values);
    	}
    }


    void testSplitToEmpty()
    {
    	for (int c = 2; c <= 32*1024; c*=2)
    	{
    		testSplitToEmpty(c);
    	}
    }

    void testSplitToEmpty(Int size)
    {
    	Base::out()<<size<<std::endl;

    	Tree* tree1 = Base::createEmptyTree(16*1024*1024);
    	PARemover remover1(tree1);

    	Tree* tree2 = Base::createEmptyTree(16*1024*1024);
    	PARemover remover2(tree2);

    	auto tree_values1 = Base::createRandomValuesVector(size);

    	Base::fillVector(tree1, tree_values1);

    	Int idx = getRandom(size);

    	tree1->splitTo(tree2, idx);

    	vector<Values> tree_values2(tree_values1.begin() + idx, tree_values1.end());

    	tree_values1.erase(tree_values1.begin() + idx, tree_values1.end());

    	Base::assertEqual(tree1, tree_values1);
    	Base::assertEqual(tree2, tree_values2);
    }

    void testSplitToPreFilled()
    {
    	for (int c = 2; c <= 32*1024; c*=2)
    	{
    		testSplitPreFilled(c);
    	}
    }

    void testSplitPreFilled(Int size)
    {
    	Base::out()<<size<<std::endl;

    	Tree* tree1 = Base::createEmptyTree(16*1024*1024);
    	PARemover remover1(tree1);

    	Tree* tree2 = Base::createEmptyTree(16*1024*1024);
    	PARemover remover2(tree2);

    	auto tree_values1 = Base::createRandomValuesVector(size);
    	auto tree_values2 = Base::createRandomValuesVector(size < 100 ? size : 100);

    	Base::fillVector(tree1, tree_values1);
    	Base::fillVector(tree2, tree_values2);

    	Int idx = getRandom(size);

    	tree1->splitTo(tree2, idx);

    	tree_values2.insert(tree_values2.begin(), tree_values1.begin() + idx, tree_values1.end());

    	tree_values1.erase(tree_values1.begin() + idx, tree_values1.end());

    	Base::assertEqual(tree1, tree_values1);
    	Base::assertEqual(tree2, tree_values2);
    }


    void testRemoveMulti()
    {
    	for (Int size = 1; size <= 32768; size*=2)
    	{
    		Tree* tree = Base::createEmptyTree(16*1024*1024);
    		auto tree_values = Base::createRandomValuesVector(size);

    		Base::fillVector(tree, tree_values);

    		this->assertEqual(tree, tree_values);

    		for (Int c = 0; c < this->iterations_; c++)
    		{
    			Int start 	= getRandom(tree->size());
    			Int end 	= start + getRandom(tree->size() - start);

    			Int block_size = tree->block_size();

    			tree->removeSpace(start, end);

    			tree_values.erase(tree_values.begin() + start, tree_values.begin() + end);

    			this->assertIndexCorrect(MA_SRC, tree);
    			this->assertEqual(tree, tree_values);

    			AssertLE(MA_SRC, tree->block_size(), block_size);
    		}
    	}
    }

    void testRemoveAll()
    {
    	for (Int size = 1; size <= 32768; size*=2)
    	{
    		this->out()<<size<<std::endl;

    		Tree* tree = Base::createEmptyTree(16*1024*1024);
    		auto tree_values = Base::createRandomValuesVector(size);
    		Base::fillVector(tree, tree_values);

    		this->assertEqual(tree, tree_values);

    		tree->removeSpace(0, tree->size());

    		this->assertEmpty(tree);
    	}
    }



    void testMerge()
    {
    	for (int c = 1; c <= 32*1024; c*=2)
    	{
    		testMerge(c);
    	}
    }

    void testMerge(Int size)
    {
    	Base::out()<<size<<std::endl;

    	Tree* tree1 = Base::createEmptyTree(16*1024*1024);
    	PARemover remover1(tree1);

    	Tree* tree2 = Base::createEmptyTree(16*1024*1024);
    	PARemover remover2(tree2);

    	auto tree_values1 = Base::createRandomValuesVector(size);
    	auto tree_values2 = Base::createRandomValuesVector(size);

    	Base::fillVector(tree1, tree_values1);
    	Base::fillVector(tree2, tree_values2);

    	tree1->mergeWith(tree2);

    	tree_values2.insert(tree_values2.end(), tree_values1.begin(), tree_values1.end());

    	Base::assertEqual(tree2, tree_values2);
    }


    void testClear()
    {
    	for (int c = 1; c <= 32*1024; c*=2)
    	{
    		testClear(c);
    	}
    }

    void testClear(Int size)
    {
    	Base::out()<<size<<std::endl;

    	Tree* tree = Base::createEmptyTree(16*1024*1024);
    	PARemover remover(tree);

    	auto tree_values = Base::createRandomValuesVector(size);
    	Base::fillVector(tree, tree_values);

    	Base::assertEqual(tree, tree_values);

    	tree->clear();

    	Base::assertEmpty(tree);

    	Base::fillVector(tree, tree_values);

    	Base::assertEqual(tree, tree_values);

    	tree->clear();
    	Base::assertEmpty(tree);
    }

};


}


#endif
