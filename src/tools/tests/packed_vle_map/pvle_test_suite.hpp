
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PVLE_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "pvle_misc_test.hpp"
#include "pvle_find_test.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedVLEMapTestSuite: public TestSuite {

public:

    PackedVLEMapTestSuite(): TestSuite("PackedVLEMapSuite")
    {
        registerTask(new PVLEMapMiscTest());
        registerTask(new PVLEMapFindTest());
    }

};

}


#endif

