
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_CREATE_HPP_
#define MEMORIA_TESTS_PMAP_CREATE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_vle_tree.hpp>
#include <memoria/core/packed/packed_sum_tree.hpp>

#include <memoria/core/tools/exint_codec.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PVLEMapFindTest: public TestTask {

	typedef PVLEMapFindTest 				MyType;

	typedef PackedVLETreeTypes<
			Int, Int, Int
	>										Types;


	typedef PackedVLETree<Types> 			Tree;

	typedef shared_ptr<Tree> 				TreePtr;

	typedef typename Tree::Value			Value;
	typedef typename Tree::Codec			Codec;

public:

    PVLEMapFindTest(): TestTask("Find")
    {
    	MEMORIA_ADD_TEST(testValueAccess);
    	MEMORIA_ADD_TEST(testSumValues);

    	MEMORIA_ADD_TEST(testFindFromStart);
    	MEMORIA_ADD_TEST(testFind);
    }

    TreePtr createTree(Int size)
    {
    	void* buffer = malloc(size);

    	Tree* tree = T2T<Tree*>(buffer);

    	tree->initByBlock(size - sizeof(Tree));

    	return TreePtr(tree);
    }

    virtual ~PVLEMapFindTest() throw() {}


    void fillTree(TreePtr& tree, function<Value()> value_provider)
    {
    	auto* values = tree->getValues();

    	Int pos = 0;

    	Tree::Codec codec;

    	Int tree_size = 0;

    	for (int c = 0; ; c++)
    	{
    		Int value = value_provider();

    		if (pos + (Int)codec.length(value) <= tree->max_size())
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

    void testValueAccess()
    {
    	TreePtr tree = createTree(4096);

    	Value c = 0;
    	fillTree(tree, [&]()->Value {
    		return c++;
    	});

    	for (int idx = 0; idx < tree->size(); idx++)
    	{
    		AssertEQ(MA_SRC, idx, tree->value(idx));
    	}
    }





    void testSumValues()
    {
    	TreePtr tree = createTree(4096);

    	Value c = 0;
    	fillTree(tree, [&]()->Value {
    		return c++;
    	});

    	for (Int start = 0; start < tree->size(); start++)
    	{
    		out()<<start<<endl;
    		for (Int c = start; c < tree->size(); c++)
    		{
    			auto sum1 = tree->sum(start, c);
    			auto sum2 = sumValues(tree, start, c);

    			AssertEQ(MA_SRC, sum1.value(), sum2);
    		}
    	}
    }

    void testFindFromStart()
    {
    	TreePtr tree = createTree(4096);

    	Value c = 0;
    	fillTree(tree, [&]()->Value {
    		return c++;
    	});

    	vector<Value> values = sumValues(tree, 0);

    	for (Int idx = 0; idx < (Int)values.size(); idx++)
    	{
    		Value v = values[idx];
    		auto result = tree->findLE(0, v);

    		SBuf id = SBuf()<<"start = "<<0<<" value = "<<v;

    		AssertLT(MA_SRC, result.pos(), tree->max_size(), id);
    		AssertEQ(MA_SRC, result.value(), v, id);
    		AssertEQ(MA_SRC, result.idx(), 0 + idx, id);
    	}
    }

    void testFind()
    {
    	TreePtr tree = createTree(4096*2);

    	Value c = 0;
    	fillTree(tree, [&]()->Value {
    		return c++;
    	});

    	for (Int start = 0; start < tree->size(); start++)
    	{
    		out()<<start<<endl;

    		vector<Value> values = sumValues(tree, start);

    		for (Int idx = 0; idx < (Int)values.size(); idx++)
    		{
    			Value v = values[idx];

    			auto result = tree->findLE(start, v);

    			AssertLT(MA_SRC, result.pos(), tree->max_size());
    			AssertEQ(MA_SRC, result.value(), v);
    			AssertEQ(MA_SRC, result.idx(), start + idx);
    		}
    	}
    }
};


}


#endif
