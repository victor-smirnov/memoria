
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

#include <memoria/v1/memoria.hpp>
#include <memoria/v1/tools/tests.hpp>

#include "sequence_test_base.hpp"

#include <vector>

namespace memoria {
namespace v1 {


template <Int BitsPerSymbol, bool Dense = true>
class SequenceSelectTest: public SequenceTestBase<BitsPerSymbol, Dense> {

    using MyType = SequenceSelectTest<BitsPerSymbol, Dense>;
    using Base   = SequenceTestBase<BitsPerSymbol, Dense>;

    using typename Base::Iterator;
    using typename Base::Ctr;
    using typename Base::CtrName;


    static const Int Symbols  = Base::Symbols;

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



    Int iterations_ = 100000;

public:
    SequenceSelectTest(StringRef name):
        Base(name)
    {
        Base::size_ = 30000;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testCtrSelect);
        MEMORIA_ADD_TEST(testIterSelectFw);
        MEMORIA_ADD_TEST(testIterSelectBw);
    }


    void testCtrSelect()
    {
        auto snp = branch();

        auto ctr = create<CtrName>(snp);

        auto seq = fillRandomSeq(*ctr.get(), size_);

        check(MA_SRC);

        auto ranks = seq->ranks();

        for (Int c = 0; c < iterations_; c++)
        {
            out() << c <<std::endl;

            Int symbol  = getRandom(Base::Symbols);
            Int rank    = getRandom(ranks[symbol]);

            if (rank == 0) rank = 1; //  rank of 0 is not defined for select()

            auto iter1 = ctr->select(symbol, rank);
            auto iter2 = seq->selectFw(symbol, rank);

            AssertFalse(MA_SRC, iter1->isEof());
            AssertTrue(MA_SRC,  iter2.is_found());

            AssertEQ(MA_SRC, iter1->pos(), iter2.idx());
        }

        commit();
    }


    void testIterSelectFw()
    {
        auto snp = branch();

        auto ctr = create<CtrName>(snp);

        auto seq = fillRandomSeq(*ctr.get(), size_);

        check(MA_SRC);

        for (Int c = 0; c < iterations_; c++)
        {
            out() << c << std::endl;

            Int pos     = getRandom(size_);
            Int symbol  = getRandom(Base::Symbols);

            Int maxrank_ = seq->rank(pos, size_, symbol);

            if (maxrank_ > 0)
            {
                Int rank    = getRandom(maxrank_);

                if (rank == 0) rank = 1;

                auto iter = ctr->seek(pos);

                auto pos0 = iter->pos();

                AssertEQ(MA_SRC, pos0, pos);

                BigInt pos_delta1 = iter->selectFw(rank, symbol);

                auto tgt_pos2 = seq->selectFw(pos, symbol, rank);

                AssertEQ(MA_SRC, iter->pos(), tgt_pos2.idx());

                if (tgt_pos2.is_found()) {
                    AssertEQ(MA_SRC, pos_delta1, rank);
                }
                else {
                    AssertEQ(MA_SRC, pos_delta1, tgt_pos2.rank());
                }
            }
        }

        commit();
    }

    void testIterSelectBw()
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

            Int maxrank_ = seq->rank(0, pos, symbol);

            if (maxrank_ > 0)
            {
                Int rank    = getRandom(maxrank_);

                if (rank == 0) rank = 1;

                auto iter   = ctr->seek(pos);

                auto tgt_pos    = seq->selectBw(pos, symbol, rank);
                auto pos_delta1 = iter->selectBw(rank, symbol);

                AssertEQ(MA_SRC, iter->pos(), tgt_pos.idx());

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

};

}}