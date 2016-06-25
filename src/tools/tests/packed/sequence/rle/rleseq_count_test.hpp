
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
class PackedRLESearchableSequenceCountTest: public PackedRLESequenceTestBase<Symbols> {

    using MyType = PackedRLESearchableSequenceCountTest<Symbols>;
    using Base = PackedRLESequenceTestBase<Symbols>;



    using typename Base::Seq;
    using typename Base::SeqPtr;
    using Value = typename Seq::Value;


    static const Int Blocks                 = Seq::Indexes;
    static const Int Bits                   = NumberOfBits(Symbols);

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

    PackedRLESearchableSequenceCountTest(StringRef name): Base(name)
    {
        this->size_ = 3000;

        MEMORIA_ADD_TEST(runCountFWTest);
        MEMORIA_ADD_TEST(runCountBWTest);
    }

    virtual ~PackedRLESearchableSequenceCountTest() noexcept {}

    rleseq::CountResult countFW(const SeqPtr& seq, Int start)
    {
        Int total = 0;

        Int last_symbol = -1;

        for (Int c = start; c < seq->size(); c++)
        {
            Int symbol = seq->symbol(c);

            if (last_symbol != symbol && last_symbol >= 0)
            {
                return rleseq::CountResult(total, last_symbol);
            }

            total++;
            last_symbol = symbol;
        }

        return rleseq::CountResult(total, last_symbol);
    }

    rleseq::CountResult countBW(const SeqPtr& seq, Int start)
    {
        Int total = 0;

        Int last_symbol = -1;

        for (Int c = start; c >= 0; c--)
        {
            Int symbol = seq->symbol(c);

            if (last_symbol != symbol && last_symbol >= 0)
            {
                return rleseq::CountResult(total, last_symbol);
            }

            total++;
            last_symbol = symbol;
        }

        return rleseq::CountResult(total, last_symbol);
    }



    void assertCountFW(const SeqPtr& seq, Int start, Int rank)
    {
        auto result1 = seq->countFW(start);

        AssertEQ(MA_SRC, result1.count(),  rank, SBuf() << start << " " << rank);
    }

    void assertCountBW(const SeqPtr& seq, Int start, Int rank)
    {
        auto result1 = seq->countBW(start);
        AssertEQ(MA_SRC, result1.count(),  rank, SBuf() << start << " " << rank);
    }


    struct Pair {
        Int rank;
        Int idx;

        Pair(Int r, Int i): rank(r), idx(i) {}
    };

    vector<Pair> createRanksFW(const SeqPtr& seq)
    {
        vector<Pair> ranks;

        auto iter = seq->begin();

        while (iter.has_data())
        {
            Int block_start = iter.idx();
            Int block_end   = iter.idx() + iter.run().length();

            ranks.push_back(Pair(countFW(seq, block_start).count(), block_start));
            ranks.push_back(Pair(countFW(seq, block_start + 1).count(), block_start + 1));
            ranks.push_back(Pair(countFW(seq, block_end - 1).count(), block_end - 1));

            iter.next_run();
        }

        return ranks;
    }


    vector<Pair> createRanksBW(const SeqPtr& seq)
    {
        vector<Pair> ranks;

        auto iter = seq->begin();

        while (iter.has_data())
        {
            Int block_start = iter.idx();
            Int block_end   = iter.idx() + iter.run().length();

            ranks.push_back(Pair(countBW(seq, block_start).count(), block_start));
            ranks.push_back(Pair(countBW(seq, block_start + 1).count(), block_start + 1));
            ranks.push_back(Pair(countBW(seq, block_end - 1).count(), block_end - 1));

            iter.next_run();
        }

        return ranks;
    }



    void runCountFWTest()
    {
        auto seq = createEmptySequence();

        populateRandom(seq, this->size_, false);

        auto ranks = createRanksFW(seq);

        out()<<"Random bitmap"<<endl;

        for (const auto& pair: ranks)
        {
            assertCountFW(seq, pair.idx, pair.rank);
        }
    }

    void runCountBWTest()
    {
        auto seq = createEmptySequence();

        populateRandom(seq, this->size_, false);

        auto ranks = createRanksBW(seq);

        out()<<"Random bitmap"<<endl;

        for (const auto& pair: ranks)
        {
            assertCountBW(seq, pair.idx, pair.rank);
        }
    }
};


}}
