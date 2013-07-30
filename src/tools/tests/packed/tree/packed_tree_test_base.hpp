
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_TREE_TEST_BASE_HPP_
#define MEMORIA_TESTS_PACKED_TREE_TEST_BASE_HPP_

#include "../../tests_inc.hpp"


#include <memoria/core/packed/packed_fse_tree.hpp>
#include <memoria/core/packed/packed_vle_tree.hpp>
#include <memoria/core/packed/packed_allocator.hpp>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <
	template <typename> class TreeType,
	template <typename> class CodecType = ValueFSECodec,
	Int Blocks		= 1,
	Int VPB 		= PackedTreeBranchingFactor,
	Int BF 			= PackedTreeBranchingFactor
>
class PackedTreeTestBase: public TestTask {
protected:
	typedef Packed2TreeTypes<
			Int,
			Int,
			Blocks,
			CodecType,
			BF,
			VPB
	>																			Types;

	typedef TreeType<Types>														Tree;

	typedef typename Tree::Value												Value;
	typedef typename Tree::IndexValue											IndexValue;
	typedef typename Tree::Values												Values;

public:

    PackedTreeTestBase(StringRef name): TestTask(name)
    {}

    Tree* createEmptyTree(Int block_size = 65536)
    {
    	void* block = malloc(block_size);
    	PackedAllocator* allocator = T2T<PackedAllocator*>(block);
    	allocator->init(block_size, 1);

    	Tree* tree = allocator->template allocateEmpty<Tree>(0);

    	return tree;
    }

    Tree* createTree(Int tree_capacity, Int free_space = 0)
    {
    	Int tree_block_size = Tree::block_size(tree_capacity);

    	Int allocator_size 	= PackedAllocator::block_size(tree_block_size + free_space, 1);

    	void* block = malloc(allocator_size);
    	PackedAllocator* allocator = T2T<PackedAllocator*>(block);
    	allocator->init(tree_block_size, 1);

    	Tree* tree = allocator->allocate<Tree>(0, tree_block_size);

    	return tree;
    }

    void truncate(vector<Values>& v, Int size) {
    	Int delta = v.size() - size;

    	for (Int idx = 0; idx < delta; idx++)
    	{
    		v.erase(v.end() - 1);
    	}
    }

    vector<Values> fillRandom(Tree* tree, Int max_value = 100)
    {
    	vector<Values> vals;

    	Int size = tree->insert(0, [&](Values& values) -> bool {
    		for (Int b = 0; b < Blocks; b++) {
    			values[b] = getRandom(max_value);
    		}

    		vals.push_back(values);
    		return true;
    	});

    	truncate(vals, size);

    	return vals;
    }

    vector<Values> fillSolid(Tree* tree, const Values& values)
    {
    	vector<Values> vals;

    	Int size = tree->insert(0, [&](Values& v) -> bool {
    		v = values;
    		vals.push_back(values);

    		return true;
    	});

    	truncate(vals, size);

    	return vals;
    }

    vector<Values> fillSolid(Tree* tree, Int value)
    {
    	vector<Values> vals;

    	Int size = tree->insert(0, [&](Values& v) -> bool {
    		for (Int b = 0; b < Blocks; b++) {
    			v[b] = value;
    		}

    		vals.push_back(v);

    		return true;
    	});

    	truncate(vals, size);

    	return vals;
    }

    void fillVector(Tree* tree, const vector<Values>& vals)
    {
    	Int cnt = 0;
    	tree->insert(0, vals.size(), [&]() {
    		return vals[cnt++];
    	});
    }

    Values createRandom(Int max = 100)
    {
    	Values values;

    	for (Int c = 0; c < Blocks; c++) {
    		values[c] = getRandom(max);
    	}

    	return values;
    }

    vector<Values> createRandomValuesVector(Int size, Int max_value = 100)
    {
    	vector<Values> vals(size);

    	for (Int c = 0; c < size; c++)
    	{
    		for (Int b = 0; b < Blocks; b++)
    		{
    			vals[c][b] = getRandom(max_value);
    		}
    	}

    	return vals;
    }

    void assertEqual(const Tree* tree, const vector<Values>& vals)
    {
    	AssertEQ(MA_SRC, tree->size(), (Int)vals.size());

    	for (Int c = 0; c < tree->size(); c++)
    	{
    		Values v;
    		for (Int b = 0; b < Blocks; b++)
    		{
    			v[b] = tree->value(b, c);
    		}

    		AssertEQ(MA_SRC, vals[c], v);
    	}
    }

    void assertEqual(const Tree* tree1, const Tree* tree2)
    {
    	AssertEQ(MA_SRC, tree1->size(), tree2->size());

    	for (Int c = 0; c < tree1->size(); c++)
    	{
    		Values v1, v2;
    		for (Int b = 0; b < Blocks; b++)
    		{
    			v1[b] = tree1->value(b, c);
    			v2[b] = tree2->value(b, c);
    		}

    		AssertEQ(MA_SRC, v1, v2);
    	}
    }

    void assertIndexCorrect(const char* src, const Tree* tree)
    {
    	try {
    		tree->check();
    	}
    	catch (Exception& e) {
    		out()<<"Tree structure check failed"<<std::endl;
    		tree->dump(out());
    		throw e;
    	}
    }

    void assertEmpty(const Tree* tree)
    {
    	Int empty_size = Tree::empty_size();

    	AssertEQ(MA_SRC, tree->size(), 0);
    	AssertEQ(MA_SRC, tree->data_size(), 0);
    	AssertEQ(MA_SRC, tree->block_size(), empty_size);
    	AssertEQ(MA_SRC, tree->index_size(), 0);
    }
};

}


#endif

