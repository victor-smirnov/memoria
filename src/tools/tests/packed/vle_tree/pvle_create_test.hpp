
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

template <Int BF, Int VPB, template <typename> class Codec_>
class PVLEMapCreateTest: public PVLETestBase<PackedVLETreeTypes<Int, Int, Codec_, 2, BF, VPB>> {

	typedef PVLEMapCreateTest 															MyType;
	typedef PVLETestBase<PackedVLETreeTypes<Int, Int, Codec_, 2, BF, VPB>> 			Base;

	typedef typename Base::Tree 			Tree;
	typedef typename Base::TreePtr 			TreePtr;
	typedef typename Base::Value			Value;
	typedef typename Base::Codec			Codec;

	typedef PackedAllocator 				Allocator;

public:

    PVLEMapCreateTest(String codec): Base((SBuf()<<"Create."<<BF<<"."<<VPB<<"."<<codec).str())
    {
    	MEMORIA_ADD_TEST(testValueAccess);
    	MEMORIA_ADD_TEST(testValueSet);
    	MEMORIA_ADD_TEST(testMultipleValuesSet);
    }

    virtual ~PVLEMapCreateTest() throw() {}


    void testValueAccess()
    {
    	for (Int block_size = 256; block_size < 128*1024; )
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

    	TreePtr tree_ptr = Base::createTree(block_size);
    	Tree* tree 		 = tree_ptr.get();

    	Value c = 0;
    	Base::fillTree(tree, [&]()->Value {
    		return c++;
    	});

    	for (int idx = 0; idx < tree->size(); idx++)
    	{
    		AssertEQ(MA_SRC, idx, tree->value(idx), SBuf()<<idx<<" "<<tree->size()<<" "<<tree->max_data_size());
    	}
    }



    void testValueSet()
    {
    	Int block_size = 4096;

    	Base::out()<<"Block size: "<<block_size<<endl;

    	TreePtr tree_ptr = Base::createTree(block_size);
    	Tree* tree 		 = tree_ptr.get();

    	Int tree_max = tree->max_data_size();
    	vector<Value> values = Base::getRandomVector(tree_max - tree_max / 10, 100);

    	Int idx = 0;
    	Base::fillTreeByElements(tree, values.size(), [&](){
    		return values[idx++];
    	});

    	AssertEQ(MA_SRC, tree->size(), (Int)values.size());

    	for (Int c = 0; c < 1000; c++)
    	{
    		Int idx = getRandom(values.size());
    		Int value = getRandom(100);

    		values[idx] = value;

    		Int delta = tree->setValue(idx, value);

    		AssertEQ(MA_SRC, delta, 0, SBuf()<<idx<<" "<<value<<" "<<c);

    		for (Int x = 0; x < tree->size(); x++)
    		{
    			Value value = tree->value(x);

    			AssertEQ(MA_SRC, value, values[x], SBuf()<<idx<<" "<<value<<" "<<x);
    		}
    	}
    }

    void testMultipleValuesSet()
    {
    	Int block_size = 4096;

    	Base::out()<<"Block size: "<<block_size<<endl;

    	TreePtr tree_ptr = Base::createTree(block_size);
    	Tree* tree 		 = tree_ptr.get();


    	MultiValueSetter<Tree, 16> setter(tree);

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
