
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_RANK_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_RANK_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "sequence_test_base.hpp"

#include <vector>

namespace memoria {


template <Int BitsPerSymbol, bool Dense = true>
class SequenceRankTest: public SequenceTestBase<BitsPerSymbol, Dense> {

    using MyType = SequenceRankTest<BitsPerSymbol, Dense>;
    using Base   = SequenceTestBase<BitsPerSymbol, Dense>;


    using typename Base::Iterator;
    using typename Base::Ctr;
    using typename Base::CtrName;


    Int iterations_ = 100000;

    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::fillRandomSeq;
    using Base::size_;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;
    using Base::ctr_name_;
    using Base::getRandom;



public:
    SequenceRankTest(StringRef name):
        Base(name)
    {
        Base::size_ = 30000;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testCtrRank);
        MEMORIA_ADD_TEST(testIterRank);
    }


    void testCtrRank()
    {
    	auto snp = branch();

    	auto ctr = create<CtrName>(snp);

    	auto seq = fillRandomSeq(*ctr.get(), size_);

    	check(MA_SRC);

        for (Int c = 0; c < iterations_; c++)
        {
        	out()<<c<<std::endl;

        	Int pos     = getRandom(size_);
        	Int symbol  = getRandom(Base::Symbols);

        	BigInt rank1 = ctr->rank(pos, symbol);
        	BigInt rank2 = seq->rank(pos, symbol);

        	AssertEQ(MA_SRC, rank1, rank2);
        }

        commit();
    }


    void testIterRank()
    {
    	auto snp = branch();

    	auto ctr = create<CtrName>(snp);

    	auto seq = fillRandomSeq(*ctr.get(), size_);

    	check(MA_SRC);

    	for (Int c = 0; c < iterations_; c++)
    	{
    		out()<<c<<std::endl;

    		Int pos1    = getRandom(size_);
    		Int pos2    = pos1 + getRandom(size_ - pos1);
    		Int symbol  = getRandom(Base::Symbols);

    		auto iter   = ctr->seek(pos1);

    		BigInt rank1 = iter->rank(pos2 - pos1, symbol);
    		BigInt rank2 = seq->rank(pos1, pos2, symbol);

    		AssertEQ(MA_SRC, iter->pos(), pos2);
    		AssertEQ(MA_SRC, rank1, rank2);

    		BigInt rank3 = iter->rank(pos1 - pos2, symbol);

    		AssertEQ(MA_SRC, iter->pos(), pos1);
    		AssertEQ(MA_SRC, rank1, rank3);
    	}

    	commit();
    }
};

}

#endif
