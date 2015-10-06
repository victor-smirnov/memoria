
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_TEST_SUITE_HPP_
#define MEMORIA_TESTS_BTTL_TEST_SUITE_HPP_


#include "../../tests_inc.hpp"

#include "bttl_core_test.hpp"
#include "bttl_iter_test.hpp"
#include "bttl_insertion_test.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class BTTLTestSuite: public TestSuite {

public:

    BTTLTestSuite(): TestSuite("BT.TL")
    {
    	registerTask(new BTTLCoreTest<BTTLTestCtr<3, PackedSizeType::VARIABLE>>("Create"));
    	registerTask(new BTTLIterTest<BTTLTestCtr<3, PackedSizeType::VARIABLE>>("Iter"));
    	registerTask(new BTTLInsertionTest<BTTLTestCtr<3, PackedSizeType::VARIABLE>>("Insert"));
    }
};

}


#endif

