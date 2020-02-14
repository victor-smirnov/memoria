
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
namespace tests {


template <int32_t Symbols>
class PackedRLESearchableSequenceSelectTest: public PackedRLESequenceTestBase<Symbols> {

    using MyType = PackedRLESearchableSequenceSelectTest<Symbols>;
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

    PackedRLESearchableSequenceSelectTest()
    {
        this->size_ = 300;
    }

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, runSelectFWTest, runSelectBWTest, runSelectFromFWTest, runSelectFromBWTest);
    }



    SelectResult selectFW(const SeqPtr& seq, int32_t start, int32_t rank, Value symbol)
    {
        MEMORIA_ASSERT(rank, >, 0);

        int32_t total = 0;

        for (int32_t c = start + 1; c < seq->size(); c++)
        {
            total += seq->test(c, symbol);

            if (total == rank)
            {
                return SelectResult(c, rank, true);
            }
        }

        return SelectResult(seq->size(), total, total == rank);
    }


    SelectResult selectBW(const SeqPtr& seq, int32_t start, int32_t rank, Value symbol)
    {
        MEMORIA_ASSERT(rank, >, 0);

        int32_t total = 0;

        for (int32_t c = start - 1; c >= 0; c--)
        {
            total += seq->test(c, symbol);

            if (total == rank)
            {
                return SelectResult(c, rank, true);
            }
        }

        return SelectResult(seq->size(), total, total == rank);
    }

    void assertSelectFW(const SeqPtr& seq, int32_t start, int32_t rank, Value symbol)
    {
        auto result1 = seq->selectFW(start, rank, symbol);
        auto result2 = selectFW(seq, start, rank, symbol);

        assert_equals(result1.is_found(),  result2.is_found(), "{} {}", start, rank);
        assert_equals(result1.rank(),  result2.rank(), "{} {}", start, rank);

        if (result1.is_found())
        {
            assert_equals(result1.local_pos(),  result2.local_pos(), "{} {}", start, rank);
        }
    }



    void assertSelectBW(const SeqPtr& seq, int32_t start, int32_t rank, Value symbol)
    {
        auto result1 = seq->selectBW(start, rank, symbol);
        auto result2 = selectBW(seq, start, rank, symbol);

        assert_equals(result1.is_found(),  result2.is_found(), "{} {}", start, rank);
        assert_equals(result1.rank(),  result2.rank(), "{} {}", start, rank);

        if (result1.is_found())
        {
            assert_equals(result1.local_pos(),  result2.local_pos(), "{} {}", start, rank);
        }
    }

    void appendRank(std::vector<int32_t>& v, int32_t rank)
    {
        if (rank > 0)
        {
            v.push_back(rank);
        }
    }

    struct Pair {
        int32_t rank;
        int32_t idx;

        Pair(int32_t r, int32_t i): rank(r), idx(i) {}
    };

    std::vector<Pair> createRanksFW(const SeqPtr& seq, int32_t symbol)
    {
        std::vector<Pair> ranks;

        auto iter = seq->begin();

        while (iter.has_data())
        {
            int32_t block_start = iter.local_pos();
            int32_t block_end   = iter.local_pos() + iter.run().length();

            ranks.push_back(createRankFw(seq, block_start, symbol));
            ranks.push_back(createRankFw(seq, block_start + 1, symbol));
            ranks.push_back(createRankFw(seq, block_end - 1, symbol));

            iter.next_run();
        }

        return ranks;
    }




    Pair createRankFw(const SeqPtr& seq, int32_t start, int32_t symbol)
    {
        int32_t size = seq->size();
        int32_t end = start + getRandom(size - start - 1) + 1;

        int32_t rank = 0;

        for (int32_t c = start + 1; c < end; c++)
        {
            if (seq->test(c, symbol))
            {
                rank++;
            }
        }

        return Pair(rank > 0 ? rank * 2 : 1, start);
    }





    Pair createRankBw(const SeqPtr& seq, int32_t start, int32_t symbol)
    {
        int32_t end = getRandom(start);

        int32_t rank = 0;

        for (int32_t c = start - 1; c >= end; c--)
        {
            if (seq->test(c, symbol))
            {
                rank++;
            }
        }

        return Pair(rank > 0 ? rank * 2 : 1, start);
    }


    std::vector<Pair> createRanksBW(const SeqPtr& seq, int32_t symbol)
    {
        std::vector<Pair> ranks;

        auto iter = seq->begin();

        while (iter.has_data())
        {
            int32_t block_start = iter.local_pos();
            int32_t block_end   = iter.local_pos() + iter.run().length();

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
        out() << "Parameters: Symbols=" << Symbols << " symbol=" << (int32_t)symbol << std::endl;

        out() << "Random bitmap, random positions" << std::endl;

        auto seq = createEmptySequence();
        populateRandom(seq, this->size_);

        auto ranks = createRanksFW(seq, symbol);
        for (const auto& pair: ranks)
        {
            assertSelectFW(seq, pair.idx, pair.rank, symbol);
        }

        out() << std::endl;
    }


    void runSelectFWTest()
    {
        auto seq = createEmptySequence();

        populateRandom(seq, this->size_);

        int32_t maxrank_ = seq->rank(1) + 1;

        for (int32_t rank = 1; rank < maxrank_; rank += 100)
        {
            auto result1 = seq->selectFW(rank, 1);
            auto result3 = selectFW(seq, -1, rank, 1);

            assert_equals(result1.is_found(), result3.is_found(), "{}", rank);
            assert_equals(result1.local_pos(), result3.local_pos(), "{}", rank);
            assert_equals(result1.rank(), result3.rank(), "{}", rank);
        }

        auto result1 = seq->selectFW(maxrank_ * 2, 1);
        auto result3 = selectFW(seq, -1, maxrank_ * 2, 1);

        assert_equals(false, result1.is_found());
        assert_equals(result1.rank(), maxrank_ - 1);

        assert_equals(result1.is_found(), result3.is_found());
        assert_equals(result1.rank(), result3.rank());
    }



    void runSelectBWTest()
    {
        auto seq = createEmptySequence();

        populateRandom(seq, this->size_);

        int32_t maxrank_ = seq->rank(1) + 1;

        for (int32_t rank = 1; rank < maxrank_; rank += 100)
        {
            auto result1 = seq->selectBW(rank, 1);
            auto result3 = selectBW(seq, seq->size(), rank, 1);

            assert_equals(result1.is_found(), result3.is_found(), "{}", rank);
            assert_equals(result1.local_pos(), result3.local_pos(), "{}", rank);

            assert_equals(result1.rank(), result3.rank(), "{}", rank);
        }

        auto result1 = seq->selectBW(maxrank_ * 2, 1);
        auto result3 = selectBW(seq, seq->size(), maxrank_ * 2, 1);

        assert_equals(false, result1.is_found());
        assert_equals(result1.rank(), maxrank_ - 1);

        assert_equals(result1.is_found(), result3.is_found());
        assert_equals(result1.rank(), result3.rank());
    }


    void runSelectFromBWTest()
    {
        runSelectFromBWTest(0);
        runSelectFromBWTest(Symbols - 1);
    }

    void runSelectFromBWTest(Value symbol)
    {
        out() << "Parameters: Symbols=" << Symbols << " symbol=" << (int32_t)symbol << std::endl;


        out() << "Random bitmap, random positions" << std::endl;

        auto seq = createEmptySequence();
        populateRandom(seq, this->size_);

        auto ranks = this->createRanksBW(seq, symbol);

        for (const auto& pair: ranks)
        {
            assertSelectBW(seq, pair.idx, pair.rank, symbol);
        }

        out() << std::endl;
    }
};


}}
