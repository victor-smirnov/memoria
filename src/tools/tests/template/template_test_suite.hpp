
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SUM_SET_BATCH_TEMPLATE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_SUM_SET_BATCH_TEMPLATE_TEST_SUITE_HPP_


#include "../tests_inc.hpp"

#include "task.hpp"



namespace memoria {

using namespace memoria::vapi;
using namespace std;

class TemplateTestSuite: public TestSuite {

public:

    TemplateTestSuite(): TestSuite("TemplateTestSuite")
    {
        enabled = false;

        registerTask(new TemplateTestTask());
    }

};

}


#endif

