
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/assertions.hpp>

#include <memoria/v1/reactor/reactor.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include "packed_tree_buffer_test.hpp"
#include "packed_tree_find_test.hpp"
#include "packed_tree_sum_test.hpp"
#include "packed_tree_misc_test.hpp"

namespace memoria {
namespace v1 {
namespace tests {

namespace {

using Suite1 = PackedTreeMiscTest<PkdFQTreeT<int64_t, 4>>;
MMA1_CLASS_SUITE(Suite1, "Tree.Misc.4.FSQ");

using Suite2 = PackedTreeFindTest<PkdFQTreeT<int64_t, 4>>;
MMA1_CLASS_SUITE(Suite2, "Tree.Find.4.FSQ");

using Suite3 = PackedTreeSumTest<PkdFQTreeT<int64_t, 4>>;
MMA1_CLASS_SUITE(Suite3, "Tree.Sum.4.FSQ");


using Suite4 = PackedTreeMiscTest<PkdVQTreeT<int64_t, 4, UByteI7Codec>>;
MMA1_CLASS_SUITE(Suite4, "Tree.Misc.4.VLQ.I7");

using Suite5 = PackedTreeMiscTest<PkdVQTreeT<int64_t, 4, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite5, "Tree.Misc.4.VLQ.Elias");

using Suite6 = PackedTreeMiscTest<PkdVDTreeT<int64_t, 4, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite6, "Tree.Misc.4.VLD.Elias");

using Suite7 = PackedTreeSumTest<PkdVQTreeT<int64_t, 4, UByteI7Codec>>;
MMA1_CLASS_SUITE(Suite7, "Tree.Sum.4.VLQ.I7");

using Suite8 = PackedTreeSumTest<PkdVQTreeT<int64_t, 4, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite8, "Tree.Sum.4.VLQ.Elias");

using Suite9 = PackedTreeSumTest<PkdVDTreeT<int64_t, 4, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite9, "Tree.Sum.4.VLD.Elias");


using Suite10 = PackedTreeFindTest<PkdVQTreeT<int64_t, 4, UByteI7Codec>>;
MMA1_CLASS_SUITE(Suite10, "Tree.Find.4.VLQ.I7");

using Suite11 = PackedTreeFindTest<PkdVQTreeT<int64_t, 4, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite11, "Tree.Find.4.VLQ.Elias");

using Suite12 = PackedTreeFindTest<PkdVDTreeT<int64_t, 4, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite12, "Tree.Find.4.VLD.Elias");


using Suite13 = PackedTreeInputBufferTest<PkdVQTreeT<int64_t, 4, UByteI7Codec>>;
MMA1_CLASS_SUITE(Suite13, "Tree.Buffer.4.VLQ.I7");

using Suite14 = PackedTreeInputBufferTest<PkdVQTreeT<int64_t, 4, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite14, "Tree.Buffer.4.VLQ.Elias");


using Suite15 = PackedTreeInputBufferTest<PkdVDTreeT<int64_t, 4, UByteI7Codec>>;
MMA1_CLASS_SUITE(Suite15, "Tree.Buffer.4.VLD.I7");

using Suite16 = PackedTreeInputBufferTest<PkdVDTreeT<int64_t, 4, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite16, "Tree.Buffer.4.VLD.Elias");

using Suite17 = PackedTreeInputBufferTest<PkdFQTreeT<int64_t, 4>>;
MMA1_CLASS_SUITE(Suite17, "Tree.Buffer.4.FSQ");

}


}}}
