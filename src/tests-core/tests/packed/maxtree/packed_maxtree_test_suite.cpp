
// Copyright 2016 Victor Smirnov
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

#include "packed_maxtree_find_test.hpp"
#include "packed_maxtree_misc_test.hpp"

namespace memoria {
namespace v1 {
namespace tests {

using FixedWidthArgMiscSuite = PackedMaxTreeMiscTest<PkdFMTreeT<int64_t, 1>>;
MMA1_CLASS_SUITE(FixedWidthArgMiscSuite, u"FixedWidthArgMiscSuite");

using FixedWidthArgFindSuite = PackedMaxTreeFindTest<PkdFMTreeT<int64_t, 1>>;
MMA1_CLASS_SUITE(FixedWidthArgFindSuite, u"FixedWidthArgFindSuite");


//using VariableWidthArgMiscSuite = PackedMaxTreeMiscTest<PkdVBMTreeT<BigInteger>>;
//MMA1_CLASS_SUITE(VariableWidthArgMiscSuite, u"VariableWidthArgMiscSuite");

//using VariableWidthArgFindSuite = PackedMaxTreeFindTest<PkdVBMTreeT<BigInteger>>;
//MMA1_CLASS_SUITE(VariableWidthArgFindSuite, u"VariableWidthArgFindSuite");

}}}
