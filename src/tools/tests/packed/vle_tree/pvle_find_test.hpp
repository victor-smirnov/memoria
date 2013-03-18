
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_FIND_HPP_
#define MEMORIA_TESTS_PVLE_FIND_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_vle_tree.hpp>

#include <memoria/core/tools/exint_codec.hpp>

#include "pvle_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

template <Int BF, Int VPB, template <typename> class Codec_>
class PVLEMapFindTest: public PVLETestBase<PackedVLETreeTypes<Int, Int, Codec_, 2, BF, VPB>> {

	typedef PVLEMapFindTest<BF, VPB, Codec_> 											MyType;
	typedef PVLETestBase<PackedVLETreeTypes<Int, Int, Codec_, 2, BF, VPB>> 				Base;

	typedef typename Base::Types			Types;
	typedef typename Base::Tree 			Tree;
	typedef typename Base::TreePtr 			TreePtr;
	typedef typename Base::Value			Value;
	typedef typename Base::Codec			Codec;

	typedef typename Tree::IndexKey			IndexKey;

	typedef VLETreeValueDescr<Value>		ValueDescr;

	vector<Int> block_sizes_ = {16, 50, 125, 256, 1024, 4096}; //

public:

    PVLEMapFindTest(StringRef codec): Base((SBuf()<<"Find."<<BF<<"."<<VPB<<"."<<codec).str())
    {
    	MEMORIA_ADD_TEST(testSumValues);

    	MEMORIA_ADD_TEST(testFindFromStart);
    	MEMORIA_ADD_TEST(testFind);

    	MEMORIA_ADD_TEST(testFindLargeValue);
    }

    virtual ~PVLEMapFindTest() throw() {}

    void fillTree(Tree* tree)
    {
    	fillTree(tree, tree->max_size());
    }

    void fillTree(Tree* tree, Int size)
    {
    	Value c = 0;
    	Base::fillTree(tree, [&]()->Value {
    		return c++;
    	});
    }

    ValueDescr findLE1(const Tree* tree, Int start_idx, Value value)
    {
    	FindElementFn<Tree, BTreeCompareLT> fn(*tree, value);

    	Int pos;

    	if (start_idx > 0)
    	{
    		Int start = tree->getValueOffset(start_idx);
    		pos = Tree::TreeTools::find_fw(start, fn);
    	}
    	else {
    		pos = Tree::TreeTools::find_fw(fn);
    	}

    	Codec codec;
    	Value actual_value;

    	const auto* values_ = tree->getValues();
    	codec.decode(values_, actual_value, pos);

    	return ValueDescr(actual_value + fn.sum(), pos, start_idx + fn.position());
    }

    ValueDescr findLE(const Tree* tree, Value value)
    {
    	FindElementFn<Tree, BTreeCompareLT> fn(*tree, value);

    	Int pos = Tree::TreeTools::find_fw(fn);

    	Codec codec;
    	Value actual_value;

    	const auto* values_ = tree->values();
    	codec.decode(values_, actual_value, pos, tree->max_size());

    	return ValueDescr(actual_value + fn.sum(), pos, fn.position());
    }

    ValueDescr findLE(const Tree* tree, Int start_idx, Value value)
    {
    	ValueDescr prefix = sum(tree, start_idx);

    	ValueDescr descr = findLE(tree, value + prefix.value());

    	return ValueDescr(descr.value() - prefix.value(), descr.pos(), descr.idx());
    }

    ValueDescr sum(const Tree* tree, Int to)
    {
    	GetVLEValuesSumFn<Tree> fn(*tree, to);

    	Int pos = Tree::TreeTools::find_fw(fn);

    	return ValueDescr(fn.value(), pos, to);
    }

    ValueDescr sum(const Tree* tree, Int from, Int to)
    {
    	ValueDescr prefix = sum(tree, from);
    	ValueDescr total = sum(tree, to);

    	return ValueDescr(total.value() - prefix.value(), total.pos(), to);
    }


    void testSumValues()
    {
    	Base::runFunctionFor(block_sizes_, [this](Int size){
    		this->testSumValues(size);
    	});
    }


    void testSumValues(Int block_size)
    {
    	Base::out() <<"Block Size: "<<block_size<<endl;

    	TreePtr tree_ptr = Base::createTree(block_size);
    	Tree* tree 		 = tree_ptr.get();


    	fillTree(tree);

    	for (Int start = 0; start < tree->size(); start++)
    	{
    		Base::out()<<start<<endl;
    		for (Int c = start; c < tree->size(); c++)
    		{
    			auto sum1 = sum(tree, start, c);
    			auto sum2 = Base::sumValues(tree, start, c);

    			AssertEQ(MA_SRC, sum1.value(), sum2);
    		}
    	}
    }


    void testFindFromStart()
    {
    	Base::runFunctionFor(block_sizes_, [this](Int size){
    		this->testFindFromStart(size);
    	});
    }

    void testFindFromStart(Int block_size)
    {
    	Base::out() <<"Block Size: "<<block_size<<endl;

    	TreePtr tree_ptr = Base::createTree(block_size);
    	Tree* tree 		 = tree_ptr.get();

    	fillTree(tree);

    	vector<Value> values = Base::sumValues(tree, 0);

    	for (Int idx = 0; idx < (Int)values.size(); idx++)
    	{
    		Value v = values[idx];
    		auto result = findLE(tree, 0, v);

    		AssertLT(MA_SRC, result.pos(), tree->max_size());
    		AssertEQ(MA_SRC, result.value(), v);
    		AssertEQ(MA_SRC, result.idx(), 0 + idx);
    	}
    }

    void testFind()
    {
    	Base::runFunctionFor(block_sizes_, [this](Int size){
    		this->testFind(size);
    	});
    }

    void testFind(Int block_size)
    {
    	Base::out() <<"Block Size: "<<block_size<<endl;

    	TreePtr tree_ptr = Base::createTree(block_size);
    	Tree* tree 		 = tree_ptr.get();

    	fillTree(tree);


    	for (Int start = 0; start < tree->size(); start++)
    	{
    		Base::out()<<start<<endl;

    		vector<Value> values = Base::sumValues(tree, start);

    		for (Int idx = 0; idx < (Int)values.size(); idx++)
    		{
    			Value v = values[idx];

    			auto result = findLE(tree, start, v);

    			AssertLT(MA_SRC, result.pos(), tree->max_size());
    			AssertEQ(MA_SRC, result.value(), v);
    			AssertEQ(MA_SRC, result.idx(), start + idx);
    		}
    	}
    }

    void testFindLargeValue()
    {
    	TreePtr tree_ptr = Base::createTree(4096);
    	Tree* tree 		 = tree_ptr.get();

    	for (Int size = 0; size < tree->max_size(); size += 10)
    	{
    		fillTree(tree, size);

    		Int pos1 = tree->getValueOffset(tree->size() - 1);
    		Int pos2 = tree->getValueOffset(tree->size());

    		AssertLT(MA_SRC, pos1, tree->max_size());
    		AssertGE(MA_SRC, pos2, tree->max_size());
    	}
    }
};


}


#endif
