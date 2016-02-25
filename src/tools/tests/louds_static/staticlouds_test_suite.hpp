
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_STATICLOUDS_TEST_SUITE_HPP_
#define MEMORIA_TESTS_STATICLOUDS_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "staticlouds_misc_test.hpp"
#include "staticlouds_subtree_test.hpp"

namespace memoria {

using namespace std;

class StaticLoudsTestSuite: public TestSuite {

public:

    StaticLoudsTestSuite(): TestSuite("StaticLoudsSuite")
    {
        registerTask(new StaticLoudsMiscTest());
        registerTask(new StaticLoudsSubtreeTest());
    }

};

}


#endif

