
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PFSE_FIND_HPP_
#define MEMORIA_TESTS_PFSE_FIND_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_fse_tree.hpp>

#include <memoria/core/tools/exint_codec.hpp>

#include "pfse_test_base.hpp"

#include <memory>


namespace memoria {

using namespace std;


template <Int BF, Int VPB>
class PackedFSETreeFindTest: public PackedFSETestBase<Packed2TreeTypes<Int, Int, 1, ValueFSECodec, BF, VPB>> {

	typedef PackedFSETreeFindTest<BF, VPB> 													MyType;
	typedef PackedFSETestBase<Packed2TreeTypes<Int, Int, 1, ValueFSECodec, BF, VPB>> 		Base;

	typedef typename Base::Types			Types;
	typedef typename Base::Tree 			Tree;
	typedef typename Base::Value			Value;
	typedef typename Base::Values			Values;


	typedef typename Tree::IndexValue		IndexKey;


	vector<Int> block_sizes_ = {50, 125, 256, 1024, 4096}; //

public:

	PackedFSETreeFindTest(): Base((SBuf()<<"Find."<<BF<<"."<<VPB).str())
    {
    	MEMORIA_ADD_TEST(testSumValues);

    	MEMORIA_ADD_TEST(testFindFromStart);
    	MEMORIA_ADD_TEST(testFind);
    	MEMORIA_ADD_TEST(testFindExt);

		MEMORIA_ADD_TEST(testSearchSpeed);
		MEMORIA_ADD_TEST(testSearchLargeBlockSpeed);
    }

    virtual ~PackedFSETreeFindTest() throw() {}

    void fillTree(Tree* tree)
    {
    	fillTree(tree, tree->max_size());
    }

    void fillTree(Tree* tree, Int size)
    {
    	Value c = 0;
    	Base::fillTree(tree, size, [&](){
    		return Values(c++);
    	});
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

    	Tree* tree = Base::createTree(block_size);

    	fillTree(tree);

    	for (Int start = 0; start < tree->size(); start++)
    	{
    		Base::out()<<start<<endl;
    		for (Int c = start; c < tree->size(); c++)
    		{
    			auto sum1 = tree->sum(0, start, c);
    			auto sum2 = Base::sumValues(tree, start, c);

    			AssertEQ(MA_SRC, sum1, sum2);
    		}
    	}

    	Base::remove(tree);
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

    	Tree* tree = Base::createTree(block_size);

    	fillTree(tree);

    	vector<Value> values = Base::sumValues(tree, 0);

    	for (Int idx = 0; idx < (Int)values.size(); idx++)
    	{
    		Value v = values[idx];
    		auto result = tree->findLEForward(0, 0, v);

    		AssertEQ(MA_SRC, result.value(), v);
    		AssertEQ(MA_SRC, result.idx(), 0 + idx);
    	}

    	Base::remove(tree);
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

    	Tree* tree = Base::createTree(block_size);

    	fillTree(tree);


    	for (Int start = 0; start < tree->size(); start++)
    	{
    		Base::out()<<start<<endl;

    		vector<Value> values = Base::sumValues(tree, start);

    		for (Int idx = 0; idx < (Int)values.size(); idx++)
    		{
    			Value v = values[idx];

    			auto result = tree->findLEForward(0, start, v);

    			AssertEQ(MA_SRC, result.value(), v);
    			AssertEQ(MA_SRC, result.idx(), start + idx);
    		}
    	}

    	Base::remove(tree);
    }

    void testFindExt()
    {
    	Tree* tree = Base::createTree(4096);

    	fillTree(tree, 100);

    	auto result1 = tree->template find_le<FSEFindExtender>(10);
    	Base::out()<<result1.idx()<<" "<<result1.value()<<endl;

    	auto result2 = tree->template find_lt<FSEFindExtender>(10);
    	Base::out()<<result2.idx()<<" "<<result2.value()<<endl;

    	Base::remove(tree);
    }


    void testSearchSpeed()
    {
    	Base::out()<<"Test Search Speed"<<endl;

    	for (Int size = 4; size <= 4096; size *= 2)
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

    	Int tree_size = size_max;

    	Tree* tree = Base::createTree(Tree::block_size(tree_size));

    	Int idx = 0;
    	Base::fillTree(tree, data.size(), [&](){
    		return Values(data[idx++]);
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
    		v += tree->findLEForward(0, 0, data[idx]).value();
    	}

    	BigInt t1 = getTimeInMillis();

    	Base::out()<<tree->size()<<" "<<tree->block_size()<<" "<<FormatTime(t1-t0)<<endl;

    	Base::remove(tree);
    }

    void testSearchLargeBlockSpeed()
    {
    	Base::out()<<"Search Large Block Speed"<<endl;

    	for (Int c = 1; c <= 1024; c *= 2)
    	{
    		testSearchLargeBlockSpeed(c * 4096, 1024/c);
    	}

    	Base::out()<<endl;
    }


    void testSearchLargeBlockSpeed(Int size_max, Int blocks)
    {
    	typedef std::pair<Int, Int> IntPair;
    	typedef std::pair<Tree*, vector<Value>> TreePair;

    	vector<TreePair> trees;

    	for (Int c = 0; c < blocks; c++)
    	{
    		vector<Value> data(size_max);
    		for (auto& v: data)
    		{
    			v = getRandom(10) + 1;
    		}

    		Int tree_size = size_max;

    		Tree* tree = Base::createTree(Tree::block_size(tree_size));

    		Int idx = 0;
    		Base::fillTree(tree, data.size(), [&](){
    			return Values(data[idx++]);
    		});



    		Value last = 0;

    		for (auto& v: data)
    		{
    			v += last;
    			last = v;
    		}

    		trees.push_back(TreePair(tree, data));
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
    		const Tree* tree_ptr = trees[idx.first].first;
    		vector<Value>& data =  trees[idx.first].second;

    		v += tree_ptr->findLEForward(0, 0, data[idx.second]).value();
    	}

    	BigInt t1 = getTimeInMillis();

    	Base::out()<<size_max<<" "<<blocks<<" "<<FormatTime(t1-t0)<<endl;

    	for (auto pair: trees) {
    		Base::remove(pair.first);
    	}
    }
};


}



#endif
