
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../../tests_inc.hpp"


#include "packed_tree_misc_test.hpp"
#include "packed_tree_sum_test.hpp"
#include "packed_tree_find_test.hpp"
#include "packed_tree_buffer_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class PackedTreeTestSuite: public TestSuite {

public:

    PackedTreeTestSuite(): TestSuite("Packed.Tree")
    {

        registerTask(new PackedTreeMiscTest<PkdFQTreeT<BigInt, 4>>("Misc.4.FSQ"));
        registerTask(new PackedTreeFindTest<PkdFQTreeT<BigInt, 4>>("Find.4.FSQ"));
        registerTask(new PackedTreeSumTest<PkdFQTreeT<BigInt, 4>>("Sum.4.FSQ"));

        registerTask(new PackedTreeMiscTest<PkdVQTreeT<BigInt, 4, UByteI7Codec>>("Misc.4.VLQ.I7"));
        registerTask(new PackedTreeMiscTest<PkdVQTreeT<BigInt, 4, UBigIntEliasCodec>>("Misc.4.VLQ.Elias"));
        registerTask(new PackedTreeMiscTest<PkdVDTreeT<BigInt, 4, UBigIntEliasCodec>>("Misc.4.VLD.Elias"));

        registerTask(new PackedTreeSumTest<PkdVQTreeT<BigInt, 4,  UByteI7Codec>>("Sum.4.VLQ.I7"));
        registerTask(new PackedTreeSumTest<PkdVQTreeT<BigInt, 4,  UBigIntEliasCodec>>("Sum.4.VLQ.Elias"));
        registerTask(new PackedTreeSumTest<PkdVDTreeT<BigInt, 4,  UBigIntEliasCodec>>("Sum.4.VLD.Elias"));

        registerTask(new PackedTreeFindTest<PkdVQTreeT<BigInt, 4, UBigIntEliasCodec>>("Find.4.VLQ.Elias"));
        registerTask(new PackedTreeFindTest<PkdVQTreeT<BigInt, 4, UByteI7Codec>>("Find.4.VLQ.I7"));
        registerTask(new PackedTreeFindTest<PkdVDTreeT<BigInt, 4, UByteI7Codec>>("Find.4.VLD.I7"));

        registerTask(new PackedTreeInputBufferTest<PkdVQTreeT<BigInt, 4, UByteI7Codec>>("Buffer.4.VLQ.I7"));
        registerTask(new PackedTreeInputBufferTest<PkdVQTreeT<BigInt, 4, UBigIntEliasCodec>>("Buffer.4.VLQ.Elias"));

        registerTask(new PackedTreeInputBufferTest<PkdVDTreeT<BigInt, 4, UByteI7Codec>>("Buffer.4.VLD.I7"));
        registerTask(new PackedTreeInputBufferTest<PkdVDTreeT<BigInt, 4, UBigIntEliasCodec>>("Buffer.4.VLD.Elias"));

        registerTask(new PackedTreeInputBufferTest<PkdFQTreeT<BigInt, 4>>("Buffer.4.FSQ"));
    }

};

}}