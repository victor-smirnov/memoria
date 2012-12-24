
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SUM_SET_BATCH_SUM_TREE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_SUM_SET_BATCH_SUM_TREE_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "sum_set_batch_test.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class SumTreeTestSuite: public TestSuite {

public:

    SumTreeTestSuite(): TestSuite("SumTreeSuite")
    {
        registerTask(new SumsetBatchTest());
    }

};

}


#endif

