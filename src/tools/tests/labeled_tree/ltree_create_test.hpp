
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LABELEDTREE_CREATE_HPP_
#define MEMORIA_TESTS_LABELEDTREE_CREATE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "ltree_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class LabeledTreeCreateTest: public LabeledTreeTestBase {

	typedef LabeledTreeTestBase													Base;
    typedef LabeledTreeCreateTest                                               MyType;

    Int 	max_degree_	= 10;
    Int 	iterations_ = 1;

public:

    LabeledTreeCreateTest(): LabeledTreeTestBase("Create")
    {
    	size_ = 100000;

        MEMORIA_ADD_TEST_PARAM(max_degree_);
        MEMORIA_ADD_TEST_PARAM(iterations_);

    	MEMORIA_ADD_TEST(testFillTree);
    }

    virtual ~LabeledTreeCreateTest() throw () {}

    void testFillTree()
    {
    	DefaultLogHandlerImpl logHandler(Base::out());

    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr tree(&allocator);

    	try {
    		TreeNode root = this->fillRandom(tree, size_, max_degree_);

    		forceCheck(allocator, MA_SRC);

    		allocator.commit();

    		StoreResource(allocator, "ftree", 0);

    		checkTree(tree, root);
    	}
    	catch (...) {
    		this->dump_name_ =  Store(allocator);
    		throw;
    	}
    }
};

}

#endif
