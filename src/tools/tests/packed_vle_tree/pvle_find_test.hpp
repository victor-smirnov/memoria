
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_FIND_HPP_
#define MEMORIA_TESTS_PVLE_FIND_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_vle_tree.hpp>
#include <memoria/core/packed/packed_sum_tree.hpp>

#include <memoria/core/tools/exint_codec.hpp>

#include "pvle_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

template <Int BF, Int VPB>
class PVLEMapFindTest: public PVLETestBase<PackedVLETreeTypes<Int, Int, Int, EmptyResizeHandler, BF, VPB>> {

	typedef PVLEMapFindTest<BF, VPB> 														MyType;
	typedef PVLETestBase<PackedVLETreeTypes<Int, Int, Int, EmptyResizeHandler, BF, VPB>> 	Base;

	typedef typename Base::Types			Types;
	typedef typename Base::Tree 			Tree;
	typedef typename Base::TreePtr 			TreePtr;
	typedef typename Base::Value			Value;
	typedef typename Base::Codec			Codec;

	typedef typename Tree::IndexKey			IndexKey;
	typedef typename Tree::Key				Key;

	vector<Int> block_sizes_ = {16, 50, 125, 256, 1024, 4096};

public:

    PVLEMapFindTest(): Base((SBuf()<<"Find."<<BF<<"."<<VPB).str())
    {
    	MEMORIA_ADD_TEST(testSumValues);
//
    	MEMORIA_ADD_TEST(testFindFromStart);
    	MEMORIA_ADD_TEST(testFind);

    	MEMORIA_ADD_TEST(testFindLargeValue);
    }

    virtual ~PVLEMapFindTest() throw() {}

    void fillTree(TreePtr& tree)
    {
    	fillTree(tree, tree->max_size());
    }

    void fillTree(TreePtr& tree, Int size)
    {
    	Value c = 0;
    	Base::fillTree(tree, [&]()->Value {
    		return c++;
    	});
    }


    class ValueDescr {
    	Value value_;
    	Int pos_;
    	Int idx_;
    public:
    	ValueDescr(BigInt value, Int pos, Int idx): value_(value), pos_(pos), idx_(idx) {}

    	Value value() const 	{return value_;}
    	Int pos() const 		{return pos_;}
    	Int idx() const 		{return idx_;}
    };



    class GetValuesSumFn: public GetValueOffsetFnBase<Tree, GetValuesSumFn> {
    	typedef GetValueOffsetFnBase<Tree, GetValuesSumFn> Base;

    	const IndexKey* indexes_;

    	Value value_ = 0;

    public:
    	GetValuesSumFn(const Tree& me, Int limit): Base(me, limit)
    	{
    		indexes_ = me.indexes(0);
    	}

    	void processIndexes(Int start, Int end)
    	{
    		for (Int c = start; c < end; c++)
    		{
    			value_ += indexes_[c];
    		}
    	}

    	void processValue(Value value)
    	{
    		value_ += value;
    	}

    	Value value() const {
    		return value_;
    	}
    };




    template <template <typename, typename> class Comparator>
    class FindElementFn: public FindForwardFnBase<Tree, FindElementFn<Comparator>, IndexKey, Comparator> {

    	typedef FindForwardFnBase<Tree, FindElementFn<Comparator>, IndexKey, Comparator> 	Base;

    public:
    	static const Int BranchingFactor        = Types::BranchingFactor;
    	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
    private:

    	const Tree& 		me_;

    	const UByte* 		values_;
    	const IndexKey* 	sizes_;

    	Int position_;

    public:
    	FindElementFn(const Tree& me, BigInt limit): Base(me.indexes(0), limit), me_(me), position_(0)
    	{
    		values_  = me.getValues();
    		sizes_   = me.sizes();
    	}

    	Int max_size() const {
    		return me_.max_size();
    	}

    	Int index_size() const {
    		return me_.index_size();
    	}

    	Int size() const {
    		return me_.max_size();
    	}

    	Int position() const {
    		return position_;
    	}

    	Int walkFirstValuesBlock(Int start, Int end)
    	{
    		return walkValues(start, end);
    	}

    	Int walkLastValuesBlock(Int value_block_num)
    	{
    		Int offset = value_block_num ? me_.offset(value_block_num) : 0;

    		Int pos = value_block_num * ValuesPerBranch + offset;
    		Int end = me_.max_size();

    		return walkValues(pos, end);
    	}

    	Int walkValues(Int pos, Int end)
    	{
    		Comparator<Int, Key> compare;
    		Codec codec;

    		while (pos < end)
    		{
    			Value value;

    			Int len = codec.decode(values_, value, pos);

    			if (compare(value, Base::limit_))
    			{
    				Base::sum_ 	 += value;
    				Base::limit_ -= value;

    				pos += len;

    				position_ ++;
    			}
    			else {
    				return pos;
    			}
    		}

    		return end;
    	}

    	void processIndexes(Int start, Int end)
    	{
    		for (Int c = start; c < end; c++)
    		{
    			position_ += sizes_[c];
    		}
    	}
    };


    ValueDescr findLE1(const TreePtr& tree, Int start_idx, Value value)
    {
    	FindElementFn<BTreeCompareLT> fn(*tree.get(), value);

    	Int pos;

    	if (start_idx > 0)
    	{
    		Int start = tree->getValueOffset(start_idx);
    		pos = tree->find_fw(start, fn);
    	}
    	else {
    		pos = tree->find_fw(fn);
    	}

    	Codec codec;
    	Value actual_value;

    	const UByte* values_ = tree->getValues();
    	codec.decode(values_, actual_value, pos);

    	return ValueDescr(actual_value + fn.sum(), pos, start_idx + fn.position());
    }

    ValueDescr findLE(const TreePtr& tree, Value value)
    {
    	FindElementFn<BTreeCompareLT> fn(*tree.get(), value);

    	Int pos = tree->find_fw(fn);

    	Codec codec;
    	Value actual_value;

    	const UByte* values_ = tree->getValues();
    	codec.decode(values_, actual_value, pos);

    	return ValueDescr(actual_value + fn.sum(), pos, fn.position());
    }

    ValueDescr findLE(const TreePtr& tree, Int start_idx, Value value)
    {
    	ValueDescr prefix = sum(tree, start_idx);

    	ValueDescr descr = findLE(tree, value + prefix.value());

    	return ValueDescr(descr.value() - prefix.value(), descr.pos(), descr.idx());
    }

    ValueDescr sum(const TreePtr& tree, Int to)
    {
    	GetValuesSumFn fn(*tree.get(), to);

    	Int pos = tree->find_fw(fn);

    	return ValueDescr(fn.value(), pos, to);
    }

    ValueDescr sum(const TreePtr& tree, Int from, Int to)
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

    	TreePtr tree = Base::createTree(block_size);

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

    	TreePtr tree = Base::createTree(block_size);

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

    	TreePtr tree = Base::createTree(block_size);

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
    	TreePtr tree = Base::createTree(4096);

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
