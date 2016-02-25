
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_WT_TEST_SUITE_HPP_
#define MEMORIA_TESTS_WT_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "wt_test.hpp"

#include <vector>

namespace memoria {

using namespace std;

class WTTestSuite: public TestSuite {

public:

    WTTestSuite(): TestSuite("WT")
    {
        registerTask(new WTTest("Int"));
    }
};

}


#endif

