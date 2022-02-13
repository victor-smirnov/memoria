
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

#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>

#include <memoria/reactor/reactor.hpp>

#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <memoria/core/integer/integer.hpp>

#include "packed_tree_find_test.hpp"
#include "packed_tree_sum_test.hpp"
#include "packed_tree_misc_test.hpp"

namespace memoria {
namespace tests {

namespace {

//template <typename DataType, bool Indexed, size_t Columns, DTOrdering Ordering>

using Tree4C = PackedDataTypeBufferT<BigInt, true, 4, DTOrdering::SUM>;

using Suite1 = PackedTreeMiscTest<Tree4C>;
MMA_CLASS_SUITE(Suite1, "Tree.Misc.4.FSQ");

using Suite2 = PackedTreeFindTest<Tree4C>;
MMA_CLASS_SUITE(Suite2, "Tree.Find.4.FSQ");

using Suite3 = PackedTreeSumTest<Tree4C>;
MMA_CLASS_SUITE(Suite3, "Tree.Sum.4.FSQ");


//using Suite18 = PackedTreeMiscTest<PkdFQTreeT<UnsignedAccumulator<256>, 4, UnsignedAccumulator<128>>>;
//MMA_CLASS_SUITE(Suite18, "Tree.Misc.UAcc128.FSQ");

//using Suite19 = PackedTreeFindTest<PkdFQTreeT<UnsignedAccumulator<256>, 4, UnsignedAccumulator<128>>>;
//MMA_CLASS_SUITE(Suite19, "Tree.Find.UAcc128.FSQ");

//using Suite20 = PackedTreeSumTest<PkdFQTreeT<UnsignedAccumulator<256>, 4, UnsignedAccumulator<128>>>;
//MMA_CLASS_SUITE(Suite20, "Tree.Sum.UAcc128.FSQ");



}


}}
