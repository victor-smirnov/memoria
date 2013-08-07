
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LABELEDTREE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_LABELEDTREE_TEST_SUITE_HPP_


#include "../tests_inc.hpp"

#include "ltree_api_test.hpp"
//#include "ltree_create_test.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class LabeledTreeTestSuite: public TestSuite {

public:

    LabeledTreeTestSuite(): TestSuite("LabeledTreeSuite")
    {
    	registerTask(new LabeledTreeApiTest());
//    	registerTask(new LabeledTreeCreateTest());
    }

};

}


#endif

