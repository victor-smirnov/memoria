
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
    using Base::rank;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;
    using Base::ctr_name_;
    using Base::getRandom;



public:
    SequenceRankTest(StringRef name):
        Base(name)
    {
        Base::size_ = 300000;

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

        for (Int c = 0; c < iterations_; c++)
        {
            out()<<c<<std::endl;

            Int pos1    = getRandom(size_);
            Int pos2    = pos1 + getRandom(size_ - pos1);
            Int symbol  = getRandom(Base::Symbols);

            auto iter   = ctr->seek(pos1);

            BigInt rank1 = iter->rankFw(pos2 - pos1, symbol);
            BigInt rank2 = seq->rank(pos1, pos2, symbol);

            AssertEQ(MA_SRC, rank1, rank2);
            AssertEQ(MA_SRC, iter->pos(), pos2);

            BigInt rank3 = iter->rank(pos1 - pos2, symbol);

            AssertEQ(MA_SRC, iter->pos(), pos1);
            AssertEQ(MA_SRC, rank1, rank3);
        }

        commit();
    }
};

}}