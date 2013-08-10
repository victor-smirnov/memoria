
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VTREE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_VTREE_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "vtree_test.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class VTreeTestSuite: public TestSuite {

public:

	VTreeTestSuite(): TestSuite("VTreeSuite")
    {
        registerTask(new VTreeTest("Int"));
    }
};

}


#endif

