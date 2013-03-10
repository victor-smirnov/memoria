
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_CREATE_HPP_
#define MEMORIA_TESTS_PVLE_CREATE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_tools.hpp>

#include "pvle_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

template <Int BF, Int VPB>
class PVLEMapCreateTest: public PVLETestBase<PackedVLETreeTypes<Int, Int, Int, EmptyAllocator, 2, BF, VPB>> {

	typedef PVLEMapCreateTest 																MyType;
	typedef PVLETestBase<PackedVLETreeTypes<Int, Int, Int, EmptyAllocator, 2, BF, VPB>> 	Base;

	typedef typename Base::Tree 			Tree;
	typedef typename Base::TreePtr 			TreePtr;
	typedef typename Base::Value			Value;
	typedef typename Base::Codec			Codec;

public:

    PVLEMapCreateTest(): Base((SBuf()<<"Create."<<BF<<"."<<VPB).str())
    {
    	MEMORIA_ADD_TEST(testValueAccess);
    	MEMORIA_ADD_TEST(testValueSet);
    	MEMORIA_ADD_TEST(testMultipleValuesSet);
    }

    virtual ~PVLEMapCreateTest() throw() {}


    void testValueAccess()
    {
    	for (Int block_size = sizeof(Tree); block_size < 128*1024; )
    	{
    		testValueAccess(block_size);

    		if (block_size < 4096)
    		{
    			block_size += 1;
    		}
    		else {
    			block_size += getRandom(1000) + 1;
    		}
    	}
    }

    void testValueAccess(Int block_size)
    {
    	Base::out()<<"Block size: "<<block_size<<endl;
    	TreePtr tree = Base::createTree(block_size);

    	Value c = 0;
    	Base::fillTree(tree, [&]()->Value {
    		return c++;
    	});

    	for (int idx = 0; idx < tree->size(); idx++)
    	{
    		AssertEQ(MA_SRC, idx, tree->value(idx));
    	}
    }

    void testValueSet()
    {
    	Int block_size = 4096;

    	Base::out()<<"Block size: "<<block_size<<endl;
    	TreePtr tree = Base::createTree(block_size);

    	vector<Value> values(tree->max_size() / 3);

    	for (auto& v: values) v = getRandom(10000);

    	Int idx = 0;
    	Base::fillTreeByElements(tree, values.size(), [&](){
    		return values[idx++];
    	});

    	AssertEQ(MA_SRC, tree->size(), (Int)values.size());

    	for (Int c = 0; c < 1000; c++)
    	{
    		Int idx = getRandom(values.size());
    		Int value = getRandom(10000);

    		values[idx] = value;

    		Int delta = tree->setValue(idx, value);

    		AssertEQ(MA_SRC, delta, 0, SBuf()<<idx<<" "<<value<<" "<<c);

    		for (Int c = 0; c < tree->size(); c++)
    		{
    			Value value = tree->value(c);

    			AssertEQ(MA_SRC, value, values[c]);
    		}
    	}
    }

    void testMultipleValuesSet()
    {
    	Int block_size = 4096;

    	Base::out()<<"Block size: "<<block_size<<endl;
    	TreePtr tree = Base::createTree(block_size);

    	MultiValueSetter<Tree, 16> setter(tree.get());

    	for (Int c = 0; c < 16; c++)
    	{
    		setter.value(c) = c;
    	}

    	setter.putValues();

    	for (Int c = 0; c < 16; c++)
    	{
    		setter.value(c) += 0x10;
    	}

    	setter.putValues();

    	for (Int c = 0; c < 16; c++)
    	{
    		setter.value(c) += 0x10;
    	}

    	setter.putValues();

    	for (Int c = 0; c < 16; c++)
    	{
    		setter.value(c) += 0x10;
    	}

    	setter.putValues();

    	tree->reindex();

    	for (Int c = 0; c < 16; c++)
    	{
    		for (Int d = 0; d < 4; d++)
    		{
    			Value value = tree->value(c * 4 + d);

    			AssertEQ(MA_SRC, value, c + d * 0x10);
    		}
    	}
    }
};


}


#endif
