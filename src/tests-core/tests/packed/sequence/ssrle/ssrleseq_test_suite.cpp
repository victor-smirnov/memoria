
// Copyright 2022 Victor Smirnov
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

//#include "rleseq_rank_test.hpp"
//#include "rleseq_select_test.hpp"
//#include "rleseq_count_test.hpp"
#include "ssrleseq_rle_test.hpp"
#include "ssrleseq_misc_test.hpp"

namespace memoria {
namespace tests {

namespace {

//auto Suite1 = register_class_suite<PackedRLESearchableSequenceRankTest<2>>("RleSeq.Rank.2");
//auto Suite2 = register_class_suite<PackedRLESearchableSequenceSelectTest<2>>("RleSeq.Select.2");
//auto Suite3 = register_class_suite<PackedRLESearchableSequenceCountTest<2>>("RleSeq.Count.2");

auto Suite0 = register_class_suite<PackedSSRLESearchableSequenceRLETest<1>>("SSRleSeq.RLE.1");
auto Suite1 = register_class_suite<PackedSSRLESearchableSequenceMiscTest<1>>("SSRleSeq.Misc.1");


}

}}
