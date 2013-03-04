
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

#include <memoria/core/tools/exint_codec.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PVLEMapCreateTest: public TestTask {

	typedef PVLEMapCreateTest 				MyType;

	typedef PackedVLETreeTypes<
			Int, Int, Int
	>										Types;


	typedef PackedVLETree<Types> 			Tree;

public:

    PVLEMapCreateTest(): TestTask("Create")
    {
    	MEMORIA_ADD_TEST(testCreate);
    }

    virtual ~PVLEMapCreateTest() throw() {}

    void testOffsetSetter(const Tree* tree)
    {
    	cout<<tree->offset(1);
    }

    void testCreate()
    {
    	UByte buffer[4096];

    	Tree* tree = T2T<Tree*>(buffer);

    	tree->initByBlock(sizeof(buffer) - sizeof(Tree));

    	cout<<tree->max_size()<<" "<<tree->index_size()<<endl;

    	tree->offset(10) = 0;

    	AssertThrows<Exception>(MA_SRC, [tree]() {
    		tree->offset(0) = 15;
    	});
    }

};


}


#endif
