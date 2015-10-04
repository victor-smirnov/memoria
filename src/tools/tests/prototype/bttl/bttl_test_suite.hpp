
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
    	registerTask(new BTTLCoreTest<Table<BigInt, Byte>>("Create"));
    	registerTask(new BTTLIterTest<Table<BigInt, Byte>>("Iter"));
    	registerTask(new BTTLInsertionTest<Table<BigInt, Byte>>("Insert"));
    }
};

}


#endif

