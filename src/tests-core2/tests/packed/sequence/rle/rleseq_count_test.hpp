
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

#include <memory>
#include <vector>
#include <functional>

#include "rleseq_test_base.hpp"

namespace memoria {
namespace v1 {
namespace tests {

template <int32_t Symbols>
class PackedRLESearchableSequenceCountTest: public PackedRLESequenceTestBase<Symbols> {

    using MyType = PackedRLESearchableSequenceCountTest<Symbols>;
    using Base = PackedRLESequenceTestBase<Symbols>;



    using typename Base::Seq;
    using typename Base::SeqPtr;
    using Value = typename Seq::Value;


    static const int32_t Blocks                 = Seq::Indexes;
    static const int32_t Bits                   = NumberOfBits(Symbols);

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

    PackedRLESearchableSequenceCountTest()
    {
        this->size_ = 3000;
    }

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TESTS(suite, runCountFWTest, runCountBWTest);
    }


    rleseq::CountResult countFW(const SeqPtr& seq, int32_t start)
    {
        int32_t total = 0;

        int32_t last_symbol = -1;

        for (int32_t c = start; c < seq->size(); c++)
        {
            int32_t symbol = seq->symbol(c);

            if (last_symbol != symbol && last_symbol >= 0)
            {
                return rleseq::CountResult(total, last_symbol);
            }

            total++;
            last_symbol = symbol;
        }

        return rleseq::CountResult(total, last_symbol);
    }

    rleseq::CountResult countBW(const SeqPtr& seq, int32_t start)
    {
        int32_t total = 0;

        int32_t last_symbol = -1;

        for (int32_t c = start; c >= 0; c--)
        {
            int32_t symbol = seq->symbol(c);

            if (last_symbol != symbol && last_symbol >= 0)
            {
                return rleseq::CountResult(total, last_symbol);
            }

            total++;
            last_symbol = symbol;
        }

        return rleseq::CountResult(total, last_symbol);
    }



    void assertCountFW(const SeqPtr& seq, int32_t start, int32_t rank)
    {
        auto result1 = seq->countFW(start);

        assert_equals(result1.count(),  rank, u"{} {}", start, rank);
    }

    void assertCountBW(const SeqPtr& seq, int32_t start, int32_t rank)
    {
        auto result1 = seq->countBW(start);
        assert_equals(result1.count(),  rank, u"{} {}", start, rank);
    }


    struct Pair {
        int32_t rank;
        int32_t idx;

        Pair(int32_t r, int32_t i): rank(r), idx(i) {}
    };

    std::vector<Pair> createRanksFW(const SeqPtr& seq)
    {
        std::vector<Pair> ranks;

        auto iter = seq->begin();

        while (iter.has_data())
        {
            int32_t block_start = iter.idx();
            int32_t block_end   = iter.idx() + iter.run().length();

            ranks.push_back(Pair(countFW(seq, block_start).count(), block_start));
            ranks.push_back(Pair(countFW(seq, block_start + 1).count(), block_start + 1));
            ranks.push_back(Pair(countFW(seq, block_end - 1).count(), block_end - 1));

            iter.next_run();
        }

        return ranks;
    }


    std::vector<Pair> createRanksBW(const SeqPtr& seq)
    {
        std::vector<Pair> ranks;

        auto iter = seq->begin();

        while (iter.has_data())
        {
            int32_t block_start = iter.idx();
            int32_t block_end   = iter.idx() + iter.run().length();

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

        out() << "Random bitmap" << std::endl;

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

        out() << "Random bitmap" << std::endl;

        for (const auto& pair: ranks)
        {
            assertCountBW(seq, pair.idx, pair.rank);
        }
    }
};


}}}
