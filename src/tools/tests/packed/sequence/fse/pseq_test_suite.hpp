
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

#include "../../../tests_inc.hpp"


#include <memoria/v1/core/tools/i64_codec.hpp>
#include <memoria/v1/core/tools/i7_codec.hpp>
#include <memoria/v1/core/tools/elias_codec.hpp>

#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>

#include "pseq_misc_test.hpp"
#include "pseq_rank_test.hpp"
#include "pseq_select_test.hpp"
#include "pseq_speed_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class PackedSequenceTestSuite: public TestSuite {

public:

    PackedSequenceTestSuite(): TestSuite("Packed.Seq")
    {
        registerTask(new PackedSearchableSequenceMiscTest<
                1,
                PkdFQTreeT<Int, 2>,
                v1::BitmapReindexFn,
                BitmapSelectFn,
                BitmapRankFn,
                BitmapToolsFn
        >("Misc.1"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        4,
                        PkdFQTreeT<Int, 16>,
                        v1::ReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >("Misc.4.FSE"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        4,
                        PkdVDTreeT<Int, 16, UBigIntEliasCodec>,
                        VLEReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >("Misc.4.Elias"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        4,
                        PkdVQTreeT<Int, 16, UByteExintCodec>,
                        VLEReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >("Misc.4.Exint"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        8,
                        PkdVDTreeT<BigInt, 256, UBigIntEliasCodec>,
                        VLEReindex8Fn,
                        Seq8SelectFn,
                        Seq8RankFn,
                        Seq8ToolsFn
        >("Misc.8.Elias"));

        registerTask(new PackedSearchableSequenceMiscTest<
                8,
                PkdVDTreeT<BigInt, 256, UBigIntI64Codec>,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >("Misc.8.I64"));



        registerTask(new PackedSearchableSequenceRankTest<
                1,
                PkdFQTreeT<Int, 2>,
                v1::BitmapReindexFn,
                 BitmapSelectFn,
                 BitmapRankFn,
                 BitmapToolsFn
        >("Rank.1"));

        registerTask(new PackedSearchableSequenceRankTest<
                4,
                PkdFQTreeT<Int, 16>,
                v1::ReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Rank.4.FSE"));

        registerTask(new PackedSearchableSequenceRankTest<
                4,
                PkdVDTreeT<BigInt, 16, UBigIntEliasCodec>,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Rank.4.VLE"));

        registerTask(new PackedSearchableSequenceRankTest<
                8,
                PkdVDTreeT<BigInt, 256, UBigIntI64Codec>,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >("Rank.8.VLE"));






        registerTask(new PackedSearchableSequenceSelectTest<
                1,
                PkdFQTreeT<Int, 2>,
                v1::BitmapReindexFn,
                 BitmapSelectFn,
                 BitmapRankFn,
                 BitmapToolsFn
        >("Select.1"));

        registerTask(new PackedSearchableSequenceSelectTest<
                4,
                PkdFQTreeT<Int, 16>,
                v1::ReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Select.4.FSE"));

        registerTask(new PackedSearchableSequenceSelectTest<
                4,
                PkdVDTreeT<BigInt, 16, UBigIntEliasCodec>,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Select.4.VLE"));

        registerTask(new PackedSearchableSequenceSelectTest<
                8,
                PkdVDTreeT<BigInt, 256, UBigIntI64Codec>,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >("Select.8.VLE"));



        registerTask(new PackedSearchableSequenceSpeedTest<
                4,
                PkdVQTreeT<BigInt, 16, UBigIntI64Codec>,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Speed.4.VLE"));


        registerTask(new PackedSearchableSequenceSpeedTest<
                        8,
                        PkdVDTreeT<BigInt, 256, UBigIntI64Codec>,
                        VLEReindex8BlkFn,
                        Seq8SelectFn,
                        Seq8RankFn,
                        Seq8ToolsFn
        >("Speed.8.VLE"));

    }



};

}}
