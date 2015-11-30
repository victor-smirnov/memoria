
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQUENCE_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_SEQUENCE_SUITE_HPP_

#include "../../tests_inc.hpp"


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

using namespace memoria::vapi;
using namespace std;

class PackedSequenceTestSuite: public TestSuite {

public:

    PackedSequenceTestSuite(): TestSuite("Packed.Seq")
    {
        registerTask(new PackedSearchableSequenceMiscTest<
        		1,
				PkdFQTree<Int, 2>,
				::memoria::BitmapReindexFn,
				BitmapSelectFn,
				BitmapRankFn,
				BitmapToolsFn
		>("Misc.1"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        4,
                        PkdFQTree<Int, 16>,
                        ::memoria::ReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >("Misc.4.FSE"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        4,
                        PkdVDTree<Int, 16, UBigIntEliasCodec>,
                        VLEReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >("Misc.4.Elias"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        4,
                        PkdVQTree<Int, 16, UByteExintCodec>,
                        VLEReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
        >("Misc.4.Exint"));

        registerTask(new PackedSearchableSequenceMiscTest<
                        8,
                        PkdVDTree<BigInt, 256, UBigIntEliasCodec>,
                        VLEReindex8Fn,
                        Seq8SelectFn,
                        Seq8RankFn,
                        Seq8ToolsFn
        >("Misc.8.Elias"));

        registerTask(new PackedSearchableSequenceMiscTest<
                8,
                PkdVDTree<BigInt, 256, UBigIntI64Codec>,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >("Misc.8.I64"));



        registerTask(new PackedSearchableSequenceRankTest<
        		1,
				PkdFQTree<Int, 2>,
				::memoria::BitmapReindexFn,
				 BitmapSelectFn,
				 BitmapRankFn,
				 BitmapToolsFn
		>("Rank.1"));

        registerTask(new PackedSearchableSequenceRankTest<
                4,
                PkdFQTree<Int, 16>,
                ::memoria::ReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Rank.4.FSE"));

        registerTask(new PackedSearchableSequenceRankTest<
                4,
                PkdVDTree<BigInt, 16, UBigIntEliasCodec>,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Rank.4.VLE"));

        registerTask(new PackedSearchableSequenceRankTest<
                8,
                PkdVDTree<BigInt, 256, UBigIntI64Codec>,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >("Rank.8.VLE"));






        registerTask(new PackedSearchableSequenceSelectTest<
        		1,
				PkdFQTree<Int, 2>,
				::memoria::BitmapReindexFn,
				 BitmapSelectFn,
				 BitmapRankFn,
				 BitmapToolsFn
		>("Select.1"));

        registerTask(new PackedSearchableSequenceSelectTest<
                4,
                PkdFQTree<Int, 16>,
                ::memoria::ReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Select.4.FSE"));

        registerTask(new PackedSearchableSequenceSelectTest<
                4,
                PkdVDTree<BigInt, 16, UBigIntEliasCodec>,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Select.4.VLE"));

        registerTask(new PackedSearchableSequenceSelectTest<
                8,
                PkdVDTree<BigInt, 256, UBigIntI64Codec>,
                VLEReindex8BlkFn,
                Seq8SelectFn,
                Seq8RankFn,
                Seq8ToolsFn
        >("Select.8.VLE"));



        registerTask(new PackedSearchableSequenceSpeedTest<
                4,
                PkdVQTree<BigInt, 16, UBigIntI64Codec>,
                VLEReindexFn,
                SeqSelectFn,
                SeqRankFn,
                SeqToolsFn
        >("Speed.4.VLE"));


        registerTask(new PackedSearchableSequenceSpeedTest<
                        8,
                        PkdVDTree<BigInt, 256, UBigIntI64Codec>,
                        VLEReindex8BlkFn,
                        Seq8SelectFn,
                        Seq8RankFn,
                        Seq8ToolsFn
        >("Speed.8.VLE"));

    }



};

}


#endif

