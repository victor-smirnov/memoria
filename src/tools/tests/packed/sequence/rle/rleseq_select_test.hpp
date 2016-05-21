
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

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memory>
#include <vector>
#include <functional>
#include "rleseq_test_base.hpp"

namespace memoria {
namespace v1 {

using namespace std;

template <Int Symbols>
class PackedRLESearchableSequenceSelectTest: public PackedRLESequenceTestBase<Symbols> {

    using MyType = PackedRLESearchableSequenceSelectTest<Symbols>;
    using Base = PackedRLESequenceTestBase<Symbols>;



    using typename Base::Seq;
    using typename Base::SeqPtr;
    using Value = typename Seq::Value;


    static const Int Blocks                 = Seq::Indexes;
    static const Int Bits                 	= NumberOfBits(Symbols);

    using Base::getRandom;
    using Base::createEmptySequence;
    using Base::fillRandom;
    using Base::populate;
    using Base::populateRandom;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::assertEmpty;
    using Base::out;

public:

    PackedRLESearchableSequenceSelectTest(StringRef name): Base(name)
    {
        this->size_ = 300;

        MEMORIA_ADD_TEST(runSelectFromFWTest);
        MEMORIA_ADD_TEST(runSelectFWTest);

        MEMORIA_ADD_TEST(runSelectBWTest);
    }

    virtual ~PackedRLESearchableSequenceSelectTest() noexcept {}



    SelectResult selectFW(const SeqPtr& seq, Int start, Int rank, Value symbol)
    {
        MEMORIA_V1_ASSERT(rank, >, 0);

        Int total = 0;

        for (Int c = start + 1; c < seq->size(); c++)
        {
            total += seq->test(c, symbol);

            if (total == rank)
            {
                return SelectResult(c, rank, true);
            }
        }

        return SelectResult(seq->size(), total, total == rank);
    }


    SelectResult selectBW(const SeqPtr& seq, Int start, Int rank, Value symbol)
    {
        MEMORIA_V1_ASSERT(rank, >, 0);

        Int total = 0;

        for (Int c = start - 1; c >= 0; c--)
        {
            total += seq->test(c, symbol);

            if (total == rank)
            {
                return SelectResult(c, rank, true);
            }
        }

        return SelectResult(-1, total, total == rank);
    }

    void assertSelectFW(const SeqPtr& seq, Int start, Int rank, Value symbol)
    {
        auto result1 = seq->selectFw(start, rank, symbol);
        auto result2 = selectFW(seq, start, rank, symbol);

        AssertEQ(MA_SRC, result1.idx(),  result2.idx(), SBuf() << start << " " << rank);
    }

    void assertSelectBW(const SeqPtr& seq, Int start, Int rank, Value symbol)
    {
        auto result1 = seq->selectBw(start, rank, symbol);
        auto result2 = selectBW(seq, start, rank, symbol);

        if (result1.is_set())
        {
        	AssertEQ(MA_SRC, result1.value().idx(),  result2.idx(), SBuf()<<start<<" "<<rank);
        }
        else {
        	// FIXME: check out-of-range cases
        }
    }

    void appendRank(vector<Int>& v, Int rank)
    {
        if (rank > 0)
        {
            v.push_back(rank);
        }
    }

    struct Pair {
        Int rank;
        Int idx;

        Pair(Int r, Int i): rank(r), idx(i) {}
    };

    vector<Pair> createRanksFW(const SeqPtr& seq, Int symbol)
    {
        vector<Pair> ranks;

        auto iter = seq->begin();

        while (iter.has_data())
        {
        	Int block_start = iter.idx();
        	Int block_end 	= iter.idx() + iter.run().length();

        	ranks.push_back(createRankFw(seq, block_start, symbol));
        	ranks.push_back(createRankFw(seq, block_start + 1, symbol));
        	ranks.push_back(createRankFw(seq, block_end - 1, symbol));

        	iter.next_run();
        }

        return ranks;
    }




    Pair createRankFw(const SeqPtr& seq, Int start, Int symbol)
    {
    	Int size = seq->size();
    	Int end = start + getRandom(size - start - 1) + 1;

        Int rank = 0;

        for (Int c = start + 1; c < end; c++)
        {
            if (seq->test(c, symbol))
            {
                rank++;
            }
        }

        return Pair(rank > 0 ? rank : 1, start);
    }





    Pair createRankBw(const SeqPtr& seq, Int start, Int symbol)
    {
    	Int end = getRandom(start);

        Int rank = 0;

        for (Int c = start - 1; c >= end; c--)
        {
            if (seq->test(c, symbol))
            {
                rank++;
            }
        }

        return Pair(rank > 0 ? rank : 1, start);
    }


    vector<Pair> createRanksBW(const SeqPtr& seq, Int symbol)
    {
        vector<Pair> ranks;

        auto iter = seq->begin();

        while (iter.has_data())
        {
        	Int block_start = iter.idx();
        	Int block_end 	= iter.idx() + iter.run().length();

        	ranks.push_back(createRankBw(seq, block_start, symbol));
        	ranks.push_back(createRankBw(seq, block_start + 1, symbol));
        	ranks.push_back(createRankBw(seq, block_end - 1, symbol));

        	iter.next_run();
        }

        return ranks;
    }




    void runSelectFromFWTest()
    {
        runSelectFromFWTest(0);
        runSelectFromFWTest(Symbols - 1);
    }

    void runSelectFromFWTest(Value symbol)
    {
        out()<<"Parameters: Symbols="<<Symbols<<" symbol="<<(Int)symbol<<endl;

        auto seq = createEmptySequence();

        populate(seq, this->size_, symbol);

        auto ranks = createRanksFW(seq, symbol);

        out()<<"Solid bitmap"<<endl;

        for (const auto& pair: ranks)
        {
        	assertSelectFW(seq, pair.idx, pair.rank, symbol);
        }

        out()<<endl;
        out()<<"Random bitmap, random positions"<<endl;

        populateRandom(seq, this->size_);

        ranks = createRanksFW(seq, symbol);
        for (const auto& pair: ranks)
        {
        	assertSelectFW(seq, pair.idx, pair.rank, symbol);
        }

        out()<<endl;
    }


    void runSelectFWTest()
    {
        auto seq = createEmptySequence();

        populateRandom(seq, this->size_);

        Int maxrank_ = seq->rank(1) + 1;

        for (Int rank = 1; rank < maxrank_; rank += 10)
        {
            auto result1 = seq->select(rank, 1);
            auto result3 = selectFW(seq, -1, rank, 1);

            AssertEQ(MA_SRC, result1.is_found(), result3.is_found(), SBuf()<<rank);

            if (result1.is_found())
            {
                AssertEQ(MA_SRC, result1.idx(), result3.idx(), SBuf()<<rank);
            }
        }
    }


    void runSelectBWTest()
    {
        runSelectBWTest(0);
        runSelectBWTest(Symbols - 1);
    }

    void runSelectBWTest(Value symbol)
    {
        out()<<"Parameters: Symbols="<<Symbols<<" symbol="<<(Int)symbol<<endl;

        auto seq = createEmptySequence();

        populate(seq, this->size_, symbol);

        auto ranks = this->createRanksBW(seq, symbol);

        out()<<"Solid bitmap"<<endl;

        for (const auto& pair: ranks)
        {
        	assertSelectBW(seq, pair.idx, pair.rank, symbol);
        }

        out()<<endl;
        out()<<"Random bitmap, random positions"<<endl;

        populateRandom(seq, this->size_);

        ranks = this->createRanksBW(seq, symbol);

        for (const auto& pair: ranks)
        {
        	assertSelectBW(seq, pair.idx, pair.rank, symbol);
        }

        out()<<endl;
    }
};


}}
