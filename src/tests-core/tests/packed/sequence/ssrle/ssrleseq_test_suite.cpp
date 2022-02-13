
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


#include "ssrleseq_select_test.hpp"
#include "ssrleseq_count_test.hpp"
#include "ssrleseq_rle_test.hpp"
#include "ssrleseq_misc_test.hpp"
#include "ssrleseq_rank_test.hpp"

namespace memoria {
namespace tests {

namespace {

auto Suite0 = register_class_suite<PackedSSRLESearchableSequenceRLETest<2>> ("SSRleSeq.RLE.2");
auto Suite1 = register_class_suite<PackedSSRLESearchableSequenceMiscTest<2>>("SSRleSeq.Misc.2");
auto Suite2 = register_class_suite<PackedSSRLESearchableSequenceRankTest<2>>("SSRleSeq.Rank.2");
auto Suite3 = register_class_suite<PackedSSRLESearchableSequenceSelectTest<2>>("SSRleSeq.Select.2");
auto Suite4 = register_class_suite<PackedSSRLESearchableSequenceCountTest<2>>("SSRleSeq.Count.2");

auto Suite5 = register_class_suite<PackedSSRLESearchableSequenceRLETest<4>> ("SSRleSeq.RLE.4");
auto Suite6 = register_class_suite<PackedSSRLESearchableSequenceMiscTest<4>>("SSRleSeq.Misc.4");
auto Suite7 = register_class_suite<PackedSSRLESearchableSequenceRankTest<4>>("SSRleSeq.Rank.4");
auto Suite8 = register_class_suite<PackedSSRLESearchableSequenceSelectTest<4>>("SSRleSeq.Select.4");
auto Suite9 = register_class_suite<PackedSSRLESearchableSequenceCountTest<4>>("SSRleSeq.Count.4");

auto Suite10 = register_class_suite<PackedSSRLESearchableSequenceRLETest<3>> ("SSRleSeq.RLE.3");
auto Suite11 = register_class_suite<PackedSSRLESearchableSequenceMiscTest<3>>("SSRleSeq.Misc.3");
auto Suite12 = register_class_suite<PackedSSRLESearchableSequenceRankTest<3>>("SSRleSeq.Rank.3");
auto Suite13 = register_class_suite<PackedSSRLESearchableSequenceSelectTest<3>>("SSRleSeq.Select.3");
auto Suite14 = register_class_suite<PackedSSRLESearchableSequenceCountTest<3>>("SSRleSeq.Count.3");

auto Suite15 = register_class_suite<PackedSSRLESearchableSequenceRLETest<3, true>> ("SSRleSeq.RLE.3.64b");
auto Suite16 = register_class_suite<PackedSSRLESearchableSequenceMiscTest<3, true>>("SSRleSeq.Misc.3.64b");
auto Suite17 = register_class_suite<PackedSSRLESearchableSequenceRankTest<3, true>>("SSRleSeq.Rank.3.64b");
auto Suite18 = register_class_suite<PackedSSRLESearchableSequenceSelectTest<3, true>>("SSRleSeq.Select.3.64b");
auto Suite19 = register_class_suite<PackedSSRLESearchableSequenceCountTest<3, true>>("SSRleSeq.Count.3.64b");

auto Suite20 = register_class_suite<PackedSSRLESearchableSequenceRLETest<2, true>> ("SSRleSeq.RLE.2.64b");
auto Suite21 = register_class_suite<PackedSSRLESearchableSequenceMiscTest<2, true>>("SSRleSeq.Misc.2.64b");
auto Suite22 = register_class_suite<PackedSSRLESearchableSequenceRankTest<2, true>>("SSRleSeq.Rank.2.64b");
auto Suite23 = register_class_suite<PackedSSRLESearchableSequenceSelectTest<2,true>>("SSRleSeq.Select.2.64b");
auto Suite24 = register_class_suite<PackedSSRLESearchableSequenceCountTest<2, true>>("SSRleSeq.Count.2.64b");


auto Suite25 = register_class_suite<PackedSSRLESearchableSequenceRLETest<256>> ("SSRleSeq.RLE.256");
auto Suite26 = register_class_suite<PackedSSRLESearchableSequenceMiscTest<256>>("SSRleSeq.Misc.256");
auto Suite27 = register_class_suite<PackedSSRLESearchableSequenceRankTest<256>>("SSRleSeq.Rank.256");
auto Suite28 = register_class_suite<PackedSSRLESearchableSequenceSelectTest<256>>("SSRleSeq.Select.256");
auto Suite29 = register_class_suite<PackedSSRLESearchableSequenceCountTest<256>>("SSRleSeq.Count.256");

}

}}
