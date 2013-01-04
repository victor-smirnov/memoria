
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQ_PSEQ_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_SEQ_PSEQ_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "pseq_init_test.hpp"
#include "pseq_rank_test.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedSeqTestSuite: public TestSuite {

public:

    PackedSeqTestSuite(): TestSuite("PackedSeqSuite")
    {
    	registerTask(new PSeqInitTest());

    	registerTask(new PSeqRankTest<1>());
    	registerTask(new PSeqRankTest<2>());
    	registerTask(new PSeqRankTest<4>());
    	registerTask(new PSeqRankTest<8>());
    }

};

}


#endif

