
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PVLE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PVLE_TEST_SUITE_HPP_

#include "../../tests_inc.hpp"

#include "pvle_misc_test.hpp"
#include "pvle_init_test.hpp"
#include "pvle_create_test.hpp"
#include "pvle_find_test.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedVLETreeTestSuite: public TestSuite {

public:

    PackedVLETreeTestSuite(): TestSuite("Packed.VLETreeSuite")
    {
        registerTask(new PVLEMapMiscTest());
//    	registerTask(new PVLEMapInitTest());
//
//    	registerTask(new PVLEMapCreateTest<32, 256>());
//    	registerTask(new PVLEMapCreateTest<15, 25>());
//    	registerTask(new PVLEMapCreateTest<44, 125>());
//    	registerTask(new PVLEMapCreateTest<11, 18>());
//
//    	registerTask(new PVLEMapFindTest<32, 256>());
//    	registerTask(new PVLEMapFindTest<11, 33>());
//    	registerTask(new PVLEMapFindTest<44, 125>());
    }

};

}


#endif

