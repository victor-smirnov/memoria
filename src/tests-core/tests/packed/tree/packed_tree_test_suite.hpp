
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/tools/tests_inc.hpp>


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
