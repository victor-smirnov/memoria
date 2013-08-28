
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQUENCE_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_SEQUENCE_SUITE_HPP_

#include "../../tests_inc.hpp"


#include "palloc_rank_test.hpp"
#include "palloc_select_test.hpp"
#include "palloc_misc_test.hpp"
#include "palloc_speed_test.hpp"

#include <memoria/core/tools/i64_codec.hpp>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedSequenceTestSuite: public TestSuite {

public:

    PackedSequenceTestSuite(): TestSuite("Packed.SequenceSuite")
    {
        registerTask(new PackedSearchableSequenceMiscTest<1>("Misc.1"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        4,
                        PkdFTree,
                        ValueFSECodec,
                        ::memoria::ReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >("Misc.4.FSE"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        4,
                        PkdVTree,
                        UBigIntEliasCodec,
                        VLEReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >("Misc.4.Elias"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        4,
                        PkdVTree,
                        UByteExintCodec,
                        VLEReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >("Misc.4.Exint"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        8,
                        PkdVTree,
                        UBigIntEliasCodec,
                        VLEReindex8Fn,
                        Seq8SelectFn,
                        Seq8RankFn,
                        Seq8ToolsFn
        >("Misc.8.Elias"));

        registerTask(new PackedSearchableSequenceMiscTest<
                8,
                PkdVTree,
                UBigIntI64Codec,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >("Misc.8.I64"));



        registerTask(new PackedSearchableSequenceRankTest<1>("Rank.1"));

        registerTask(new PackedSearchableSequenceRankTest<
                4,
                PkdFTree,
                ValueFSECodec,
                ::memoria::ReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Rank.4.FSE"));

        registerTask(new PackedSearchableSequenceRankTest<
                4,
                PkdVTree,
                UBigIntEliasCodec,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Rank.4.VLE"));

        registerTask(new PackedSearchableSequenceRankTest<
                8,
                PkdVTree,
                UBigIntI64Codec,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >("Rank.8.VLE"));






        registerTask(new PackedSearchableSequenceSelectTest<1>("Select.1"));

        registerTask(new PackedSearchableSequenceSelectTest<
                4,
                PkdFTree,
                ValueFSECodec,
                ::memoria::ReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Select.4.FSE"));

        registerTask(new PackedSearchableSequenceSelectTest<
                4,
                PkdVTree,
                UBigIntEliasCodec,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Select.4.VLE"));

        registerTask(new PackedSearchableSequenceSelectTest<
                8,
                PkdVTree,
                UBigIntI64Codec,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >("Select.8.VLE"));

        registerTask(new PackedSearchableSequenceSpeedTest<
                4,
                PkdVTree,
                //UBigIntEliasCodec,
                //UByteExintCodec,
                UBigIntI64Codec,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Speed.4.VLE"));


        registerTask(new PackedSearchableSequenceSpeedTest<
                        8,
                        PkdVTree,
//                      UBigIntEliasCodec,
//                      UByteExintCodec,
                        UBigIntI64Codec,
                        VLEReindex8BlkFn,
                        Seq8SelectFn,
                        Seq8RankFn,
                        Seq8ToolsFn
        >("Speed.8.VLE"));

    }



};

}


#endif

