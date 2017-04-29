
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


#include <memoria/v1/tools/tests_inc.hpp>

#include "rleseq_rank_test.hpp"
#include "rleseq_select_test.hpp"
#include "rleseq_count_test.hpp"
#include "rleseq_misc_test.hpp"
#include "rleseq_buffer_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class PackedRLESequenceTestSuite: public TestSuite {

public:

    PackedRLESequenceTestSuite(): TestSuite("Packed.RLESeq")
    {
        registerTask(new PackedRLESearchableSequenceRankTest<2>("Rank.2"));
        registerTask(new PackedRLESearchableSequenceSelectTest<2>("Select.2"));
        registerTask(new PackedRLESearchableSequenceCountTest<2>("Count.2"));
        registerTask(new PackedRLESearchableSequenceMiscTest<2>("Misc.2"));
        registerTask(new PackedRLESearchableSequenceBufferTest<2>("Buffer.2"));


        registerTask(new PackedRLESearchableSequenceMiscTest<4>("Misc.4"));
        registerTask(new PackedRLESearchableSequenceRankTest<4>("Rank.4"));
        registerTask(new PackedRLESearchableSequenceSelectTest<4>("Select.4"));
        registerTask(new PackedRLESearchableSequenceCountTest<4>("Count.4"));
        registerTask(new PackedRLESearchableSequenceBufferTest<4>("Buffer.4"));
    }
};


MMA1_REGISTER_TEST_SUITE(PackedRLESequenceTestSuite)

}}
