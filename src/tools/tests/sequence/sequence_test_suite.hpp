
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_SUITE_HPP_

#include "../tests_inc.hpp"


#include "sequence_create_test.hpp"
#include "sequence_rank_test.hpp"
#include "sequence_select_test.hpp"


#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class SequenceTestSuite: public TestSuite {

public:

    SequenceTestSuite(): TestSuite("SequenceSuite")
    {
//    	registerTask(new SequenceCreateTest<1>("Create.1"));
    	registerTask(new SequenceCreateTest<8>("Create.8"));

//    	registerTask(new SequenceRankTest<1>("Rank.1"));
//    	registerTask(new SequenceRankTest<3>("Rank.3"));

//    	registerTask(new SequenceSelectTest<1>("Select.1"));
//    	registerTask(new SequenceSelectTest<3>("Select.3"));
    }

};

}


#endif

