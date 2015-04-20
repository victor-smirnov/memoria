
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
#include "sequence_batch_test.hpp"
#include "sequence_update_test.hpp"


#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class SequenceTestSuite: public TestSuite {

public:

    SequenceTestSuite(): TestSuite("SequenceSuite")
    {
//        registerTask(new SequenceCreateTest<1>("1.Create"));
//        registerTask(new SequenceCreateTest<8>("8.Create"));
//
        registerTask(new SequenceRankTest<1>("1.Rank"));
//        registerTask(new SequenceRankTest<8>("8.Rank"));
//
        registerTask(new SequenceSelectTest<1>("1.Select"));
//        registerTask(new SequenceSelectTest<8>("8.Select"));

        registerTask(new SequenceBatchTest<1>("1.Batch"));
//        registerTask(new SequenceUpdateTest<1>("1.Update"));
    }

};

}


#endif

