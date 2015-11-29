
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_ARRAY_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_ARRAY_TEST_SUITE_HPP_

#include "../../tests_inc.hpp"


#include "packed_array_misc_test.hpp"
#include "packed_array_vlemisc_test.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedArrayTestSuite: public TestSuite {

public:

    PackedArrayTestSuite(): TestSuite("Packed.Array")
    {
        registerTask(new PackedArrayMiscTest<PackedVLEArray, UByteExintCodec, PackedTreeExintVPB>("Misc.VLE.Exint"));
        registerTask(new PackedArrayMiscTest<PackedVLEArray, UBigIntEliasCodec, PackedTreeEliasVPB>("Misc.VLE.Elias"));

//        registerTask(new PackedArrayVLEMiscTest<UByteExintCodec, PackedTreeExintVPB>("VLEMisc.Exint"));
//        registerTask(new PackedArrayVLEMiscTest<UBigIntEliasCodec, PackedTreeEliasVPB>("VLEMisc.Elias"));

    }

};

}


#endif

