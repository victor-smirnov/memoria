
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

    	MEMORIA_ADD_TEST(testAccessSpeed);
    	MEMORIA_ADD_TEST(testSearchSpeed);
    	MEMORIA_ADD_TEST(testSearchLargeBlockSpeed);
    }

    virtual ~PVLEMapFindTest() throw() {}

    void fillTree(Tree* tree)
    {
    	fillTree(tree, tree->max_data_size());
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
    	FindElementFn<Tree, btree::BTreeCompareLT> fn(*tree, value);

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
    	const typename Tree::Metadata* meta = tree->metadata();

    	FindElementFn<Tree, btree::BTreeCompareLT> fn(*tree, value, meta->index_size(), meta->size(), meta->data_size());

    	Int pos = Tree::TreeTools::find_fw(fn);

    	Codec codec;
    	Value actual_value;

    	const auto* values_ = tree->values();
    	codec.decode(values_, actual_value, pos, tree->max_data_size());

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

    		AssertLT(MA_SRC, result.pos(), tree->max_data_size());
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

    			AssertLT(MA_SRC, result.pos(), tree->max_data_size());
    			AssertEQ(MA_SRC, result.value(), v);
    			AssertEQ(MA_SRC, result.idx(), start + idx);
    		}
    	}
    }

    void testFindLargeValue()
    {
    	TreePtr tree_ptr = Base::createTree(4096);
    	Tree* tree 		 = tree_ptr.get();

    	for (Int size = 0; size < tree->max_data_size(); size += 10)
    	{
    		fillTree(tree, size);

    		Int pos1 = tree->getValueOffset(tree->size() - 1);
    		Int pos2 = tree->getValueOffset(tree->size());

    		AssertLT(MA_SRC, pos1, tree->max_data_size());
    		AssertGE(MA_SRC, pos2, tree->max_data_size());
    	}
    }

    void testAccessSpeed()
    {
    	Base::out()<<"Test Access Speed"<<endl;

    	for (Int size = 4; size <= 64; size *= 2)
    	{
    		testAccessSpeed(size*1024);
    	}

    	Base::out()<<endl;
    }


    void testAccessSpeed(Int size_max)
    {
    	vector<Value> data(size_max);
    	for (auto& v: data)
    	{
    		v = getRandom(10) + 1;
    	}

    	vector<Int> indexes(1000000);

    	for (auto& v: indexes)
    	{
    		v = getRandom(size_max);
    	}

    	Int tree_size = Base::getDataSize(data);

    	TreePtr tree_ptr = Base::createTree(Tree::block_size(tree_size));
    	Tree* tree = tree_ptr.get();

    	Int idx = 0;
    	Base::fillTreeByElements(tree, data.size(), [&](){
    		return data[idx++];
    	});

    	BigInt t0 = getTimeInMillis();

    	Value v = 0;

    	for (auto idx: indexes)
    	{
    		v += tree->getValue(idx);
    	}

    	BigInt t1 = getTimeInMillis();

    	Base::out()<<tree->size()<<" "<<tree->block_size()<<" "<<FormatTime(t1-t0)<<endl;
    }

    void testSearchSpeed()
    {
    	Base::out()<<"Test Search Speed"<<endl;

    	for (Int size = 4; size <= 1024; size *= 2)
    	{
    		testSearchSpeed(size*1024);
    	}

    	Base::out()<<endl;
    }


    void testSearchSpeed(Int size_max)
    {
    	vector<Value> data(size_max);
    	for (auto& v: data)
    	{
    		v = getRandom(10) + 1;
    	}

    	vector<Int> indexes(1000000);

    	for (auto& v: indexes)
    	{
    		v = getRandom(size_max);
    	}

    	Int tree_size = Base::getDataSize(data);

    	TreePtr tree_ptr = Base::createTree(Tree::block_size(tree_size));
    	Tree* tree = tree_ptr.get();

    	Int idx = 0;
    	Base::fillTreeByElements(tree, data.size(), [&](){
    		return data[idx++];
    	});

    	Value last = 0;

    	for (auto& v: data)
    	{
    		v += last;
    		last = v;
    	}

    	BigInt t0 = getTimeInMillis();

    	Value v = 0;

    	for (auto idx: indexes)
    	{
    		v += findLE(tree, data[idx]).value();
    	}

    	BigInt t1 = getTimeInMillis();

    	Base::out()<<tree->size()<<" "<<tree->block_size()<<" "<<FormatTime(t1-t0)<<endl;
    }

    void testSearchLargeBlockSpeed()
    {
    	Base::out()<<"Search Large Block Speed"<<endl;

    	for (Int c = 1; c <= 1024; c *= 2)
    	{
    		testSearchLargeBlockSpeed(c * 4096, 16*1024/c);
    	}

    	Base::out()<<endl;
    }


    void testSearchLargeBlockSpeed(Int size_max, Int blocks)
    {
    	typedef std::pair<Int, Int> IntPair;
    	typedef std::pair<TreePtr, vector<Value>> TreePair;

    	vector<TreePair> trees;

    	for (Int c = 0; c < blocks; c++)
    	{
    		vector<Value> data(size_max);
    		for (auto& v: data)
    		{
    			v = getRandom(10) + 1;
    		}

    		Int tree_size = Base::getDataSize(data);

    		TreePtr tree_ptr = Base::createTree(Tree::block_size(tree_size));
    		Tree* tree = tree_ptr.get();

    		Int idx = 0;
    		Base::fillTreeByElements(tree, data.size(), [&](){
    			return data[idx++];
    		});

    		Value last = 0;

    		for (auto& v: data)
    		{
    			v += last;
    			last = v;
    		}

    		trees.push_back(TreePair(tree_ptr, data));
    	}

    	vector<IntPair> indexes(1000000);

    	for (auto& v: indexes)
    	{
    		v.first 	= getRandom(blocks);
    		v.second 	= getRandom(size_max);
    	}


    	BigInt t0 = getTimeInMillis();

    	Value v = 0;

    	for (auto idx: indexes)
    	{
    		Tree* tree = trees[idx.first].first.get();
    		vector<Value>& data =  trees[idx.first].second;

    		v += findLE(tree, data[idx.second]).value();
    	}

    	BigInt t1 = getTimeInMillis();

    	Base::out()<<size_max<<" "<<blocks<<" "<<FormatTime(t1-t0)<<endl;
    }
};


}


#endif
