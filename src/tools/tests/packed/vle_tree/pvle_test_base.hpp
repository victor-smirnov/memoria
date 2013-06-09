
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_BASE_HPP_
#define MEMORIA_TESTS_PVLE_BASE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_vle_tree.hpp>
#include <memoria/core/packed2/packed_allocator.hpp>

#include <memoria/core/tools/exint_codec.hpp>

#include <memory>

namespace memoria {

using namespace std;

template <typename Types_>
class PVLETestBase: public TestTask {
protected:
	typedef Types_							Types;
	typedef PVLETestBase 					MyType;


	typedef PackedVLETree<Types> 			Tree;

	typedef typename Tree::Value			Value;
	typedef typename Tree::Codec			Codec;

	typedef typename Types::Allocator		Allocator;

	typedef shared_ptr<Tree> 				TreePtr;

public:

    PVLETestBase(StringRef name): TestTask(name){}

    virtual ~PVLETestBase() throw() {}

    TreePtr createTree(Int block_size, Int free_space = 0)
    {
    	free_space = PackedAllocator::roundDownBytesToAlignmentBlocks(free_space);

    	Int max_tree_size = Tree::max_tree_size(block_size);

    	Int tree_block_size = Tree::block_size(max_tree_size);

    	Tree* tree = T2T<Tree*>(malloc(tree_block_size + free_space));
    	memset(tree, 0, tree_block_size + free_space);

    	tree->init(tree_block_size);

    	tree->forceResize(free_space);

    	return TreePtr(tree, free);
    }

    void fillTree(Tree* tree, function<Value()> value_provider)
    {
    	fillTree(tree, tree->max_data_size(), value_provider);
    }

    void fillTree(Tree* tree, Int size, function<Value()> value_provider)
    {
    	auto* values = tree->values();

    	Int pos = 0;

    	Codec codec;

    	Int tree_size = 0;

    	for (int c = 0; ; c++)
    	{
    		Int value = value_provider();

    		if (pos + (Int)codec.length(value) <= size)
    		{
    			pos += codec.encode(values, value, pos, tree->max_data_size());
    			tree_size++;
    		}
    		else {
    			break;
    		}
    	}

    	tree->size() = tree_size;

    	tree->reindex();
    }

    void fillTreeByElements(Tree* tree, Int size, function<Value()> value_provider)
    {
    	auto* values = tree->values();

    	Int pos = 0;

    	Codec codec;

    	Int tree_size = 0;

    	for (int c = 0; c < size; c++)
    	{
    		Int value = value_provider();

    		pos += codec.encode(values, value, pos, tree->max_data_size());
    		tree_size++;
    	}

    	tree->size() = tree_size;

    	tree->reindex();
    }


    vector<Value> sumValues(Tree* tree, Int start)
    {
    	Int pos = tree->getValueOffset(start);
    	const auto* values_array = tree->values();
    	Codec codec;

    	vector<Value> values;

    	Value sum = 0;

    	for (Int c = start; c < tree->size(); c++)
    	{
    		Value value;

    		pos += codec.decode(values_array, value, pos, tree->max_data_size());

    		sum += value;
    		values.push_back(sum);
    	}

    	return values;
    }

    Value sumValues(Tree* tree, Int start, Int stop) const
    {
    	Int pos = tree->getValueOffset(start);

    	const auto* values = tree->values();

    	Value sum = 0;

    	Codec codec;

    	for (Int c = start; c < stop; c++)
    	{
    		Value value;

    		pos += codec.decode(values, value, pos, tree->max_data_size());

    		sum += value;
    	}

    	return sum;
    }

    void runFunctionFor(const vector<Int>& values, function<void (Int)> fn)
    {
    	for (auto value: values)
    	{
    		fn(value);
    	}
    }

    vector<Value> getRandomVector(Int size_limit, Int value_max)
    {
    	vector<Value> values;

    	Codec codec;

    	Int len = 0;
    	while (true)
    	{
    		Int value = getRandom(value_max);

    		len += codec.length(value);

    		if (len > size_limit)
    		{
    			break;
    		}
    		else {
    			values.push_back(value);
    		}
    	}

    	return values;
    }


    Int getDataSize(const vector<Value>& data)
    {
    	Int len = 0;
    	Codec codec;

    	for (auto& v: data)
    	{
    		len += codec.length(v);
    	}

    	return len;
    }
};


}


#endif
