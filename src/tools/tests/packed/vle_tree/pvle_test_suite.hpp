
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
    	registerTask(new PVLEMapInitTest());

    	registerTask(new PVLEMapCreateTest<32, 256, UByteExintCodec>("Exint"));
    	registerTask(new PVLEMapCreateTest<15, 25,  UByteExintCodec>("Exint"));
    	registerTask(new PVLEMapCreateTest<44, 125, UByteExintCodec>("Exint"));
    	registerTask(new PVLEMapCreateTest<11, 18,  UByteExintCodec>("Exint"));

    	registerTask(new PVLEMapFindTest<64, 128, UByteExintCodec>("Exint"));
    	registerTask(new PVLEMapFindTest<11, 33,  UByteExintCodec>("Exint"));
    	registerTask(new PVLEMapFindTest<44, 125, UByteExintCodec>("Exint"));

    	registerTask(new PVLEMapCreateTest<64, 128, UBigIntEliasCodec>("Elias"));
    	registerTask(new PVLEMapFindTest<64, 128, UBigIntEliasCodec>("Elias"));
    }

};

}


#endif

