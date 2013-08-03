
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LOUDS_CREATE_HPP_
#define MEMORIA_TESTS_LOUDS_CREATE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>



#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class LoudsCreateTest: public LoudsTestBase {

    typedef LoudsCreateTest                                                     MyType;

    BigInt 	tree_size_ 	= 100000;
    Int 	nodes_		= 10;
    Int 	iterations_ = 1;

public:

    LoudsCreateTest(): LoudsTestBase("Create")
    {
        MEMORIA_ADD_TEST_PARAM(tree_size_);
        MEMORIA_ADD_TEST_PARAM(nodes_);
        MEMORIA_ADD_TEST_PARAM(iterations_);

    	MEMORIA_ADD_TEST(runCreateRandomTest);
    }

    virtual ~LoudsCreateTest() throw () {}



    void runCreateRandomTest()
    {
    	for (Int c = 1; c <= iterations_; c++)
    	{
    		Allocator allocator;

    		Ctr ctr(&allocator);

    		BigInt t0 = getTimeInMillis();

    		BigInt count0 = createRandomLouds(ctr, tree_size_ * c, nodes_);

    		allocator.commit();

    		DebugCounter = 1;

    		StoreResource(allocator, "louds", c);

    		BigInt t1 = getTimeInMillis();

    		checkTreeStructure(ctr);

    		BigInt t2 = getTimeInMillis();

    		BigInt count1 = ctr.getSubtreeSize(ctr.rootNode());

    		BigInt nodes = ctr.nodes();

    		AssertEQ(MA_SRC, count0 + 1, nodes);
    		AssertEQ(MA_SRC, count1, nodes);

    		cout<<"TreeSize: "<<nodes<<" Tree Build Time: "<<FormatTime(t1-t0)<<", Traverse Time: "<<FormatTime(t2-t1)<<endl;

    		DebugCounter = 0;
    	}
    }

};

}

#endif
