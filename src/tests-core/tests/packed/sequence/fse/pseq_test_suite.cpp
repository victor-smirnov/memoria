
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


#include <memoria/core/tools/i64_codec.hpp>
#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>

#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_dense_tree.hpp>

#include "pseq_misc_test.hpp"
#include "pseq_rank_test.hpp"
#include "pseq_select_test.hpp"
#include "pseq_speed_test.hpp"

namespace memoria {
namespace tests {


namespace {

auto Suite1 = register_class_suite<PackedSearchableSequenceMiscTest<
                1,
                PkdFQTreeT<int32_t, 2>,
                BitmapReindexFn,
                BitmapSelectFn,
                BitmapRankFn,
                BitmapToolsFn
        >>("PSeq.Misc.1");



auto Suite2 = register_class_suite<PackedSearchableSequenceMiscTest<
                        4,
                        PkdFQTreeT<int32_t, 16>,
                        ReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >>("PSeq.Misc.4.FSE");

auto Suite3 = register_class_suite<PackedSearchableSequenceMiscTest<
                        4,
                        PkdVDTreeT<int32_t, 16, UInt64EliasCodec>,
                        VLEReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >>("PSeq.Misc.4.Elias");

auto Suite4 = register_class_suite<PackedSearchableSequenceMiscTest<
                        4,
                        PkdVQTreeT<int32_t, 16, UByteExintCodec>,
                        VLEReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >>("PSeq.Misc.4.Exint");

auto Suite5 = register_class_suite<PackedSearchableSequenceMiscTest<
                        8,
                        PkdVDTreeT<int64_t, 256, UInt64EliasCodec>,
                        VLEReindex8Fn,
                        Seq8SelectFn,
                        Seq8RankFn,
                        Seq8ToolsFn
        >>("PSeq.Misc.8.Elias");

auto Suite6 = register_class_suite<PackedSearchableSequenceMiscTest<
                8,
                PkdVDTreeT<int64_t, 256, UInt64I64Codec>,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >>("PSeq.Misc.8.I64");



auto Suite7 = register_class_suite<PackedSearchableSequenceRankTest<
                1,
                PkdFQTreeT<int32_t, 2>,
                BitmapReindexFn,
                BitmapSelectFn,
                BitmapRankFn,
                BitmapToolsFn
        >>("PSeq.Rank.1");

auto Suite8 = register_class_suite<PackedSearchableSequenceRankTest<
                4,
                PkdFQTreeT<int32_t, 16>,
                ReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >>("PSeq.Rank.4.FSE");

auto Suite9 = register_class_suite<PackedSearchableSequenceRankTest<
                4,
                PkdVDTreeT<int64_t, 16, UInt64EliasCodec>,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >>("PSeq.Rank.4.VLE");

auto Suite10 = register_class_suite<PackedSearchableSequenceRankTest<
                8,
                PkdVDTreeT<int64_t, 256, UInt64I64Codec>,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >>("PSeq.Rank.8.VLE");






auto Suite11 = register_class_suite<PackedSearchableSequenceSelectTest<
                1,
                PkdFQTreeT<int32_t, 2>,
                BitmapReindexFn,
                BitmapSelectFn,
                BitmapRankFn,
                BitmapToolsFn
        >>("PSeq.Select.1");

auto Suite12 = register_class_suite<PackedSearchableSequenceSelectTest<
                4,
                PkdFQTreeT<int32_t, 16>,
                ReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >>("PSeq.Select.4.FSE");

auto Suite13 = register_class_suite<PackedSearchableSequenceSelectTest<
                4,
                PkdVDTreeT<int64_t, 16, UInt64EliasCodec>,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >>("PSeq.Select.4.VLE");

auto Suite14 = register_class_suite<PackedSearchableSequenceSelectTest<
                8,
                PkdVDTreeT<int64_t, 256, UInt64I64Codec>,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >>("PSeq.Select.8.VLE");



auto Suite15 = register_class_suite<PackedSearchableSequenceSpeedTest<
                4,
                PkdVQTreeT<int64_t, 16, UInt64I64Codec>,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >>("PSeq.Speed.4.VLE");


auto Suite16 = register_class_suite<PackedSearchableSequenceSpeedTest<
                        8,
                        PkdVDTreeT<int64_t, 256, UInt64I64Codec>,
                        VLEReindex8BlkFn,
                        Seq8SelectFn,
                        Seq8RankFn,
                        Seq8ToolsFn
        >>("PSeq.Speed.8.VLE");

}


}}
