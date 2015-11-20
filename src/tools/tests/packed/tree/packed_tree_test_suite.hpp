
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_TREE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_TREE_TEST_SUITE_HPP_

#include "../../tests_inc.hpp"


#include "packed_tree_misc_test.hpp"
#include "packed_tree_sum_test.hpp"
#include "packed_tree_find_test.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedTreeTestSuite: public TestSuite {

public:

    PackedTreeTestSuite(): TestSuite("Packed.Tree")
    {
        registerTask(new PackedTreeMiscTest<PkdFQTree<BigInt, 4>>("Misc.4.FSE"));
        registerTask(new PackedTreeFindTest<PkdFQTree<BigInt, 4>>("Find.4.FSE"));
        registerTask(new PackedTreeSumTest<PkdFQTree<BigInt, 4>>("Sum.4.FSE"));

        registerTask(new PackedTreeMiscTest<PkdVQTree<BigInt, 4, UByteI7Codec>>("Misc.4.VLE.I7"));
        registerTask(new PackedTreeMiscTest<PkdVQTree<BigInt, 4, UBigIntEliasCodec>>("Misc.4.VLE.Elias"));

        registerTask(new PackedTreeSumTest<PkdVQTree<BigInt, 4,  UByteI7Codec>>("Sum.4.VLE.I7"));
        registerTask(new PackedTreeSumTest<PkdVQTree<BigInt, 4,  UBigIntEliasCodec>>("Sum.4.VLE.Elias"));

        registerTask(new PackedTreeFindTest<PkdVQTree<BigInt, 4, UBigIntEliasCodec>>("Find.4.VLE.Elias"));
        registerTask(new PackedTreeFindTest<PkdVQTree<BigInt, 4, UByteI7Codec>>("Find.4.VLE.I7"));
    }

};

}


#endif

