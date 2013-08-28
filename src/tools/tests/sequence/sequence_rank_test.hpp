
// Copyright Victor Smirnov 2013.
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

    typedef SequenceRankTest<BitsPerSymbol, Dense>                              MyType;
    typedef SequenceTestBase<BitsPerSymbol, Dense>                              Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Ctr                                                  Ctr;

    Int ctr_name_;
    Int iterations_ = 100000;

public:
    SequenceRankTest(StringRef name):
        Base(name)
    {
        Base::size_ = 30000;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();

        MEMORIA_ADD_TEST(testCtrRank);
        MEMORIA_ADD_TEST(testIterRank);
    }


    void testCtrRank()
    {
        DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr ctr(&allocator);

        allocator.commit();

        try {
            auto seq = Base::fillRandom(ctr, this->size_);

            this->forceCheck(allocator, MA_SRC);

            allocator.commit();

            for (Int c = 0; c < iterations_; c++)
            {
                this->out()<<c<<std::endl;

                Int pos     = getRandom(this->size_);
                Int symbol  = getRandom(Base::Symbols);

                BigInt rank1 = ctr.rank(pos, symbol);
                BigInt rank2 = seq.rank(pos, symbol);

                AssertEQ(MA_SRC, rank1, rank2);
            }
        }
        catch (...) {
            Base::dump_name_ = Base::Store(allocator);
            throw;
        }
    }


    void testIterRank()
    {
        DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr ctr(&allocator);

        allocator.commit();

        try {
            auto seq = Base::fillRandom(ctr, this->size_);

            this->forceCheck(allocator, MA_SRC);

            allocator.commit();

            for (Int c = 0; c < iterations_; c++)
            {
                this->out()<<c<<std::endl;

                Int pos1    = getRandom(this->size_);
                Int pos2    = pos1 + getRandom(this->size_ - pos1);
                Int symbol  = getRandom(Base::Symbols);

                auto iter   = ctr.seek(pos1);

                BigInt rank1 = iter.rank(pos2 - pos1, symbol);
                BigInt rank2 = seq.rank(pos1, pos2, symbol);

                AssertEQ(MA_SRC, iter.pos(), pos2);
                AssertEQ(MA_SRC, rank1, rank2);

                BigInt rank3 = iter.rank(pos1 - pos2, symbol);

                AssertEQ(MA_SRC, iter.pos(), pos1);
                AssertEQ(MA_SRC, rank1, rank3);
            }
        }
        catch (...) {
            Base::dump_name_ = Base::Store(allocator);
            throw;
        }
    }
};

}

#endif
