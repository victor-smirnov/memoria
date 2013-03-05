
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_CREATE_HPP_
#define MEMORIA_TESTS_PVLE_CREATE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include "pvle_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

template <Int BF, Int VPB>
class PVLEMapCreateTest: public PVLETestBase<PackedVLETreeTypes<Int, Int, Int, EmptyResizeHandler, BF, VPB>> {

	typedef PVLEMapCreateTest 																MyType;
	typedef PVLETestBase<PackedVLETreeTypes<Int, Int, Int, EmptyResizeHandler, BF, VPB>> 	Base;

	typedef typename Base::Tree 			Tree;
	typedef typename Base::TreePtr 			TreePtr;
	typedef typename Base::Value			Value;
	typedef typename Base::Codec			Codec;

public:

    PVLEMapCreateTest(): Base((SBuf()<<"Create."<<BF<<"."<<VPB).str())
    {
    	MEMORIA_ADD_TEST(testValueAccess);
    }

    virtual ~PVLEMapCreateTest() throw() {}


    void testValueAccess()
    {
    	for (Int block_size = 16; block_size < 128*1024; )
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
};


}


#endif
