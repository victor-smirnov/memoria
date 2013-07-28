
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQUENCE_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_SEQUENCE_SUITE_HPP_

#include "../../tests_inc.hpp"


//#include "palloc_cxmultiseq_test.hpp"

#include "palloc_rank_test.hpp"
#include "palloc_select_test.hpp"
#include "palloc_misc_test.hpp"


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
						PackedFSETree,
						ValueFSECodec,
						::memoria::ReindexFn,
						SequenceSelectFn,
						SequenceRankFn
		>("Misc.4.FSE"));
		registerTask(new PackedSearchableSequenceMiscTest<
						4,
						PackedVLETree,
						UBigIntEliasCodec,
						VLEReindexFn,
						SequenceSelectFn,
						SequenceRankFn
		>("Misc.4.Elias"));
		registerTask(new PackedSearchableSequenceMiscTest<
						4,
						PackedVLETree,
						UByteExintCodec,
						VLEReindexFn,
						SequenceSelectFn,
						SequenceRankFn
		>("Misc.4.Exint"));
		registerTask(new PackedSearchableSequenceMiscTest<
						8,
						PackedVLETree,
						UBigIntEliasCodec,
						VLEReindexFn,
						Sequence8SelectFn,
						Sequence8RankFn
		>("Misc.8.Elias"));

/*


		registerTask(new PackedSearchableSequenceRankTest<1>("Rank.1"));

		registerTask(new PackedSearchableSequenceRankTest<
				4,
				PackedFSETree,
				ValueFSECodec,
				::memoria::ReindexFn,
				SequenceSelectFn,
				SequenceRankFn
		>("Rank.4.FSE"));

		registerTask(new PackedSearchableSequenceRankTest<
				4,
				PackedVLETree,
				UBigIntEliasCodec,
				VLEReindexFn,
				SequenceSelectFn,
				SequenceRankFn
		>("Rank.4.VLE"));

		registerTask(new PackedSearchableSequenceRankTest<
				8,
				PackedVLETree,
				UBigIntEliasCodec,
				VLEReindexFn,
				Sequence8SelectFn,
				Sequence8RankFn
		>("Rank.8.VLE"));



		registerTask(new PackedSearchableSequenceSelectTest<1>("Select.1"));

		registerTask(new PackedSearchableSequenceSelectTest<
				4,
				PackedFSETree,
				ValueFSECodec,
				::memoria::ReindexFn,
				SequenceSelectFn,
				SequenceRankFn
		>("Select.4.FSE"));

		registerTask(new PackedSearchableSequenceSelectTest<
				4,
				PackedVLETree,
				UBigIntEliasCodec,
				VLEReindexFn,
				SequenceSelectFn,
				SequenceRankFn
		>("Select.4.VLE"));

		registerTask(new PackedSearchableSequenceSelectTest<
				8,
				PackedVLETree,
				UBigIntEliasCodec,
				VLEReindexFn,
				Sequence8SelectFn,
				Sequence8RankFn
		>("Select.8.VLE"));
		*/
    }



};

}


#endif

