
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKEDLOUDS_CREATE_HPP_
#define MEMORIA_TESTS_PACKEDLOUDS_CREATE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_louds_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

class PackedLoudsCreateTest: public PackedLoudsTestBase {

	typedef PackedLoudsTestBase													Base;
    typedef PackedLoudsCreateTest 												MyType;

public:

    PackedLoudsCreateTest(): PackedLoudsTestBase("Create")
    {
    	MEMORIA_ADD_TEST(testCreateRandom);
    }

    virtual ~PackedLoudsCreateTest() throw() {}


    void testCreateRandom()
    {
    	LoudsTreePtr tree = createRandomTree(100000);

    	Int nodes = tree->rank1();

    	for (Int c = 1; c <= nodes; c++)
    	{
    		PackedLoudsNode node(tree->select1(c), c);

    		checkTreeStructure(tree.get(), node);
    	}
    }
};


}


#endif
