
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "sequence_rank_test.hpp"
#include "sequence_select_test.hpp"

#include <vector>

namespace memoria {

using namespace std;

class SequenceTestSuite: public TestSuite {

public:

    SequenceTestSuite(): TestSuite("SequenceSuite")
    {
        registerTask(new SequenceRankTest<1>("1.Rank"));
        registerTask(new SequenceRankTest<4>("4.Rank"));
        registerTask(new SequenceRankTest<8>("8.Rank"));

        registerTask(new SequenceSelectTest<1>("1.Select"));
        registerTask(new SequenceSelectTest<4>("4.Select"));
        registerTask(new SequenceSelectTest<8>("8.Select"));
    }

};

}


#endif

