
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


#pragma once

#include <memoria/v1/tools/tests_inc.hpp>

#include "../maxtree/packed_maxtree_buffer_test.hpp"
#include "../maxtree/packed_maxtree_find_test.hpp"
#include "../maxtree/packed_maxtree_misc_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class PackedMaxTreeTestSuite: public TestSuite {

public:

    PackedMaxTreeTestSuite(): TestSuite("Packed.MaxTree")
    {
        registerTask(new PackedMaxTreeMiscTest<PkdFMTreeT<BigInt, 1>>("Misc.1.FSM"));
        registerTask(new PackedMaxTreeFindTest<PkdFMTreeT<BigInt, 1>>("Find.1.FSM"));

#ifdef HAVE_BOOST
        registerTask(new PackedMaxTreeMiscTest<PkdVBMTreeT<BigInteger>>("Misc.1.VBM"));
        registerTask(new PackedMaxTreeFindTest<PkdVBMTreeT<BigInteger>>("Find.1.VBM"));
#endif
    }

};

}}
