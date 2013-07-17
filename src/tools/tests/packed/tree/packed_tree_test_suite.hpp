
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_TREE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_TREE_TEST_SUITE_HPP_

#include "../../tests_inc.hpp"


#include "packed_tree_misc_test.hpp"
#include "packed_tree_vlemisc_test.hpp"
#include "packed_tree_sum_test.hpp"
#include "packed_tree_find_test.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedTreeTestSuite: public TestSuite {

public:

    PackedTreeTestSuite(): TestSuite("Packed.TreeSuite")
    {
        registerTask(new PackedTreeMiscTest<PackedFSETree, ValueFSECodec, 4>("Misc.4.FSE"));

    	registerTask(new PackedTreeMiscTest<PackedVLETree, UByteExintCodec, 4, PackedTreeExintVPB>("Misc.4.VLE.Exint"));
        registerTask(new PackedTreeMiscTest<PackedVLETree, UBigIntEliasCodec, 4, PackedTreeExintVPB>("Misc.4.VLE.Elias"));

        registerTask(new PackedTreeVLEMiscTest<UByteExintCodec, 4>("VLEMisc.4.Exint"));
        registerTask(new PackedTreeVLEMiscTest<UBigIntEliasCodec, 4>("VLEMisc.4.Elias"));

        registerTask(new PackedTreeSumTest<PackedVLETree,  UByteExintCodec, 4, PackedTreeExintVPB>("Sum.4.VLE.Exint"));
        registerTask(new PackedTreeSumTest<PackedVLETree,  UBigIntEliasCodec, 4, PackedTreeEliasVPB>("Sum.4.VLE.Elias"));

    	registerTask(new PackedTreeFindTest<PackedVLETree,  UBigIntEliasCodec, 4, PackedTreeEliasVPB>("Find.4.VLE.Elias"));
    	registerTask(new PackedTreeFindTest<PackedVLETree,  UByteExintCodec, 4, PackedTreeExintVPB>("Find.4.VLE.Exint"));

    	registerTask(new PackedTreeFindTest<PackedFSETree,  ValueFSECodec, 4>("Find.4.FSE"));

    	registerTask(new PackedTreeSumTest<PackedFSETree,  ValueFSECodec, 4>("Sum.4.FSE"));
    }

};

}


#endif

