
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_BASE_HPP_
#define MEMORIA_TESTS_PVLE_BASE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_vle_tree.hpp>
#include <memoria/core/packed/packed_sum_tree.hpp>

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

	typedef shared_ptr<Tree> 				TreePtr;

	typedef typename Tree::Value			Value;
	typedef typename Tree::Codec			Codec;

public:

    PVLETestBase(StringRef name): TestTask(name){}

    virtual ~PVLETestBase() throw() {}

    TreePtr createTree(Int size)
    {
    	void* buffer = malloc(size);

    	Tree* tree = T2T<Tree*>(buffer);

    	tree->initByBlock(size - sizeof(Tree));

    	return TreePtr(tree);
    }

    void fillTree(TreePtr& tree, function<Value()> value_provider)
    {
    	fillTree(tree, tree->max_size(), value_provider);
    }

    void fillTree(TreePtr& tree, Int size, function<Value()> value_provider)
    {
    	auto* values = tree->getValues();

    	Int pos = 0;

    	Codec codec;

    	Int tree_size = 0;

    	for (int c = 0; ; c++)
    	{
    		Int value = value_provider();

    		if (pos + (Int)codec.length(value) <= size)
    		{
    			pos += codec.encode(values, value, pos);
    			tree_size++;
    		}
    		else {
    			break;
    		}
    	}

    	tree->size() = tree_size;

    	tree->reindex();
    }

    vector<Value> sumValues(TreePtr tree, Int start)
    {
    	Int pos = tree->getValueOffset(start);
    	const UByte* values_array = tree->getValues();
    	Codec codec;

    	vector<Value> values;

    	Value sum = 0;

    	for (Int c = start; c < tree->size(); c++)
    	{
    		Value value;

    		pos += codec.decode(values_array, value, pos);

    		sum += value;
    		values.push_back(sum);
    	}

    	return values;
    }

    Value sumValues(TreePtr tree, Int start, Int stop) const
    {
    	Int pos = tree->getValueOffset(start);

    	const UByte* values = tree->getValues();

    	Value sum = 0;

    	Codec codec;

    	for (Int c = start; c < stop; c++)
    	{
    		Value value;

    		pos += codec.decode(values, value, pos);

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
};


}


#endif
