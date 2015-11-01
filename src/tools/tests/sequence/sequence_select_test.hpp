
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_SELECT_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_SELECT_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "sequence_test_base.hpp"

#include <vector>

namespace memoria {


template <Int BitsPerSymbol, bool Dense = true>
class SequenceSelectTest: public SequenceTestBase<BitsPerSymbol, Dense> {

    typedef SequenceSelectTest<BitsPerSymbol, Dense>                            MyType;
    typedef SequenceTestBase<BitsPerSymbol, Dense>                              Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Ctr                                                  Ctr;

    static const Int Symbols                                                    = Base::Symbols;

    Int ctr_name_;
    Int iterations_ = 100000;

public:
    SequenceSelectTest(StringRef name):
        Base(name)
    {
        Base::size_ = 30000;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();

        MEMORIA_ADD_TEST(testCtrSelect);
        MEMORIA_ADD_TEST(testIterSelectFw);
        MEMORIA_ADD_TEST(testIterSelectBw);
    }


    void testCtrSelect()
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

            auto ranks = seq.ranks();

            for (Int c = 0; c < iterations_; c++)
            {
                this->out()<<c<<std::endl;

                Int symbol  = this->getRandom(Base::Symbols);
                Int rank    = this->getRandom(ranks[symbol]);

                if (rank == 0) rank = 1; //  rank of 0 is not defined for select()

                auto iter1 = ctr.select(symbol, rank);
                auto iter2 = seq.select(symbol, rank);

                AssertFalse(MA_SRC, iter1.isEof());
                AssertTrue(MA_SRC,  iter2.is_found());

                AssertEQ(MA_SRC, iter1.pos(), iter2.idx());
            }
        }
        catch (...) {
            Base::dump_name_ = Base::Store(allocator);
            throw;
        }
    }


    void testIterSelectFw()
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

            Int size = this->size_;

            for (Int c = 0; c < iterations_; c++)
            {
                this->out()<<c<<std::endl;

                Int pos     = this->getRandom(size);
                Int symbol  = this->getRandom(Base::Symbols);

                Int max_rank = seq.rank(pos, size, symbol);

                if (max_rank > 0)
                {
                    Int rank    = this->getRandom(max_rank);

                    if (rank == 0) rank = 1;

                    auto iter   = ctr.seek(pos);

                    auto pos0 = iter.pos();

                    AssertEQ(MA_SRC, pos0, pos);

                    BigInt pos_delta1 = iter.selectFw(rank, symbol);

                    auto tgt_pos2 = seq.selectFw(pos, symbol, rank);

                    AssertEQ(MA_SRC, iter.pos(), tgt_pos2.idx());

                    if (tgt_pos2.is_found()) {
                    	AssertEQ(MA_SRC, pos_delta1, rank);
                    }
                    else {
                    	AssertEQ(MA_SRC, pos_delta1, tgt_pos2.rank());
                    }
                }
            }
        }
        catch (...) {
            Base::dump_name_ = Base::Store(allocator);
            throw;
        }
    }



    void testIterSelectBw()
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

            Int size = this->size_;

            for (Int c = 0; c < iterations_; c++)
            {
                this->out()<<c<<std::endl;

                Int pos     = this->getRandom(size);
                Int symbol  = this->getRandom(Base::Symbols);

                Int max_rank = seq.rank(0, pos, symbol);

                if (max_rank > 0)
                {
                    Int rank    = this->getRandom(max_rank);

                    if (rank == 0) rank = 1;

                    auto iter   = ctr.seek(pos);

                    auto tgt_pos        = seq.selectBw(pos, symbol, rank);
                    BigInt pos_delta1   = iter.selectBw(rank, symbol);

                    AssertEQ(MA_SRC, iter.pos(), tgt_pos.idx());

                    if (tgt_pos.is_found())
                    {
                    	AssertEQ(MA_SRC, pos_delta1, rank);
                    }
                    else {
                    	AssertEQ(MA_SRC, pos_delta1, tgt_pos.rank());
                    }
                }
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
