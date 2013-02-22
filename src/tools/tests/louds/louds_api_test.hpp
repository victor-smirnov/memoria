// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LOUDS_LOUDS_API_TEST_HPP_
#define MEMORIA_TESTS_LOUDS_LOUDS_API_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "louds_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class LoudsApiTest: public LoudsTestBase {

    typedef LoudsApiTest                                                       	MyType;

    vector<char> degrees_ = {1,4,2,2,2,2,0,0,2,1,0,0,2,0,0,0,1,1,0,0,0};

public:

    LoudsApiTest(): LoudsTestBase("Api")
    {
        MEMORIA_ADD_TEST(runParent);
    }

    virtual ~LoudsApiTest() throw () {}

    void runParent()
    {
    	Allocator allocator;
    	Ctr tree(&allocator);

    	createLouds(tree, degrees_);

    	BigInt count = 0;
    	traverseTree(tree, 0, 0, count);

    	AssertEQ(MA_SRC, count, (BigInt)degrees_.size() - 1);

    	cout<<tree.parentNode(0).node()<<endl;
    }
};

}

#endif
