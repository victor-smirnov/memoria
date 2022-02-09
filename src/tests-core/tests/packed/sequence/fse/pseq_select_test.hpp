
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

#include "pseq_test_base.hpp"

#include <memory>
#include <vector>
#include <functional>


namespace memoria {
namespace tests {

template <
    size_t Bits,
    typename IndexType,
    template <typename> class ReindexFnType = BitmapReindexFn,
    template <typename> class SelectFnType  = BitmapSelectFn,
    template <typename> class RankFnType    = BitmapRankFn,
    template <typename> class ToolsFnType   = BitmapToolsFn
>
class PackedSearchableSequenceSelectTest: public PackedSearchableSequenceTestBase<
    Bits,
    IndexType,
    ReindexFnType,
    SelectFnType,
    RankFnType,
    ToolsFnType
> {

    typedef PackedSearchableSequenceSelectTest<
            Bits,
            IndexType,
            ReindexFnType,
            SelectFnType,
            RankFnType,
            ToolsFnType
    >                                                                           MyType;

    typedef PackedSearchableSequenceTestBase<
            Bits,
            IndexType,
            ReindexFnType,
            SelectFnType,
            RankFnType,
            ToolsFnType
    >                                                                           Base;

    using typename Base::Seq;
    using typename Base::SeqPtr;
    using Value = typename Seq::Value;


    static const size_t Blocks                 = Seq::Indexes;
    static const size_t Symbols                = 1 << Bits;
    static const size_t VPB                    = Seq::ValuesPerBranch;

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

    PackedSearchableSequenceSelectTest()
    {
        this->size_ = 8192;
    }

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, runSelectFromFWTest, runSelectFWTest, runSelectBWTest);
    }



    SelectResult selectFW(const SeqPtr& seq, size_t start, size_t rank, Value symbol)
    {
        MEMORIA_ASSERT(rank, >, 0);

        size_t total = 0;

        for (size_t c = start; c < seq->size(); c++)
        {
            total += seq->test(c, symbol);

            if (total == rank)
            {
                return SelectResult(c, rank, true);
            }
        }

        return SelectResult(seq->size(), total, total == rank);
    }


    SelectResult selectBW(const SeqPtr& seq, size_t start, size_t rank, Value symbol)
    {
        MEMORIA_ASSERT(rank, >, 0);

        size_t total = 0;

        for (size_t cc = 0; cc < start; cc++)
        {
            size_t c = start - 1 - cc;

            total += seq->test(c, symbol);

            if (total == rank)
            {
                return SelectResult(c, rank, true);
            }
        }

        return SelectResult(-1, total, total == rank);
    }

    void assertSelectFW(const SeqPtr& seq, size_t start, size_t rank, Value symbol)
    {
        auto result1 = seq->selectFw(start, symbol, rank);
        auto result2 = selectFW(seq, start, rank, symbol);

        assert_equals(result1.is_found(),  result2.is_found(), "{} {}", start, rank);

        if (!result1.is_found())
        {
            try {
                assert_equals(result1.rank(), result2.rank(), "{} {}", start, rank);
            }
            catch (...) {
                seq->dump(this->out());
                throw;
            }
        }
        else {
            assert_equals(result1.local_pos(),  result2.local_pos(), "{} {}", start, rank);
        }
    }

    void assertSelectBW(const SeqPtr& seq, size_t start, size_t rank, Value symbol)
    {
        auto result1 = seq->selectBw(start, symbol, rank);
        auto result2 = selectBW(seq, start, rank, symbol);

        assert_equals(result1.is_found(),  result2.is_found(), "{} {}", start, rank);

        if (!result1.is_found())
        {
            assert_equals(result1.rank(), result2.rank(), "{} {}", start, rank);
        }
        else {
            assert_equals(result1.local_pos(),  result2.local_pos(), "{} {}", start, rank);
        }
    }

    std::vector<size_t> createStarts(const SeqPtr& seq)
    {
        size_t max_block  = seq->size() / VPB + (seq->size() % VPB == 0 ? 0 : 1);

        std::vector<size_t> starts;

        for (size_t block = 0; block < max_block; block++)
        {
            size_t block_start = block * VPB;
            size_t block_end = block_start + VPB <= (size_t)seq->size() ? block_start + VPB : seq->size();

            starts.push_back(block_start);
            starts.push_back(block_start + 1);

            for (size_t d = 2; d < (size_t)VPB && block_start + d < block_end; d += 128)
            {
                starts.push_back(block_start + d);
            }

            starts.push_back(block_end - 1);
            starts.push_back(block_end);
        }

        return starts;
    }


    std::vector<size_t> createRanks(const SeqPtr& seq, size_t start)
    {
        size_t max_block  = seq->size() / VPB + (seq->size() % VPB == 0 ? 0 : 1);

        std::vector<size_t> ranks;

        for (size_t block = start / VPB; block < max_block; block++)
        {
            size_t block_start = block * VPB;
            size_t block_end = block_start + VPB <= (size_t)seq->size() ? block_start + VPB : seq->size();

            appendRank(ranks, block_start);
            appendRank(ranks, block_start + 1);

            size_t rank_delta = 128 / Bits;

            for (size_t d = rank_delta; d < VPB; d += rank_delta)
            {
                appendRank(ranks, block_start + d);
            }

            appendRank(ranks, block_end - 1);
            appendRank(ranks, block_end);
        }

        return ranks;
    }

    void appendRank(std::vector<size_t>& v, size_t rank)
    {
        if (rank > 0)
        {
            v.push_back(rank);
        }
    }

    struct Pair {
        size_t rank;
        size_t idx;

        Pair(size_t r, size_t i): rank(r), idx(i) {}
    };

    std::vector<Pair> createRanksFW(const SeqPtr& seq, size_t start, Value symbol)
    {
        std::vector<Pair> ranks;

        size_t rank = 0;

        for (size_t c = start; c < (size_t)seq->size(); c++)
        {
            if (seq->test(c, symbol))
            {
                rank++;
                ranks.push_back(Pair(rank, c));
            }
        }

        return ranks;
    }

    std::vector<Pair> createRanksBW(const SeqPtr& seq, size_t start, Value symbol)
    {
        std::vector<Pair> ranks;

        size_t rank = 0;

        for (size_t cc = start + 1; cc > 0; cc--)
        {
            size_t c = cc - 1;

            if (seq->test(c, symbol))
            {
                rank++;
                ranks.push_back(Pair(rank, c));
            }
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
        out() << "Parameters: Bits=" << Bits << " symbol=" << (size_t)symbol << std::endl;

        auto seq = createEmptySequence();

        populate(seq, this->size_, symbol);

        std::vector<size_t> starts = createStarts(seq);

        out() << "Solid bitmap" << std::endl;

        for (size_t start: starts)
        {
            out() << start << std::endl;

            std::vector<size_t> ranks = createRanks(seq, start);

            for (size_t rank: ranks)
            {
                assertSelectFW(seq, start, rank, symbol);
            }
        }

        out() << std::endl;
        out() << "Random bitmap, random positions" << std::endl;

        populateRandom(seq, this->size_);
        starts = createStarts(seq);

        for (size_t start: starts)
        {
            this->out() << start << std::endl;

            std::vector<size_t> ranks = createRanks(seq, start);

            for (size_t rank: ranks)
            {
                assertSelectFW(seq, start, rank, symbol);
            }
        }

        out() << std::endl;
        out() << "Random bitmap, " << (size_t)symbol << "-set positions" << std::endl;

        starts = createStarts(seq);

        for (size_t start : starts)
        {
            auto pairs = createRanksFW(seq, start, symbol);

            out() << start << std::endl;

            for (auto pair: pairs)
            {
                auto result = seq->selectFw(start, symbol, pair.rank);

                assert_equals(true, result.is_found());

                if (!result.is_found())
                {
                    assert_equals(result.rank(), pair.rank);
                }
                else {
                    assert_equals(result.local_pos(),  pair.idx);
                }
            }
        }

        out() << std::endl;
    }


    void runSelectFWTest()
    {
        auto seq = createEmptySequence();

        populateRandom(seq, this->size_);

        size_t maxrank_ = seq->rank(1) + 1;

        for (size_t rank = 1; rank < maxrank_; rank++)
        {
            auto result1 = seq->selectFw(0, 1, rank);
            auto result2 = seq->selectFw(1, rank);

            auto result3 = selectFW(seq, 0, rank, 1);

            assert_equals(result1.is_found(), result2.is_found(), "{}", rank);
            assert_equals(result1.is_found(), result3.is_found(), "{}", rank);

            if (result1.is_found())
            {
                assert_equals(result1.local_pos(), result2.local_pos(), "{}", rank);
                assert_equals(result1.local_pos(), result3.local_pos(), "{}", rank);
            }
            else {
                assert_equals(result1.rank(), result2.rank(), "{}", rank);
                assert_equals(result1.rank(), result3.rank(), "{}", rank);
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
        out() << "Parameters: " << Bits << " " << (size_t)symbol << std::endl;

        auto seq = createEmptySequence();

        populate(seq, this->size_, symbol);

        std::vector<size_t> starts = createStarts(seq);

        starts.push_back(seq->size());

        out() << "Solid bitmap" << std::endl;

        for (size_t start: starts)
        {
            out() << start << std::endl;

            std::vector<size_t> ranks = createRanks(seq, start);

            for (size_t rank: ranks)
            {
                assertSelectBW(seq, start, rank, symbol);
            }
        }

        out() << std::endl;
        out() << "Random bitmap, random positions" << std::endl;

        populateRandom(seq, this->size_);

        for (size_t start: starts)
        {
            this->out() << start << std::endl;

            std::vector<size_t> ranks = createRanks(seq, start);

            for (size_t rank: ranks)
            {
                assertSelectBW(seq, start, rank, symbol);
            }
        }

        out() << std::endl;
        out() << "Random bitmap, " << (size_t)symbol << "-set positions" << std::endl;

        for (size_t start : starts)
        {
            if (start > 0 && start < seq->size())
            {
                out() << start << std::endl;

                auto pairs = createRanksBW(seq, start-1, symbol);

                for (auto pair: pairs)
                {
                    SelectResult result = seq->selectBw(start, symbol, pair.rank);

                    assert_equals(true, result.is_found(), "{} {}", start, pair.rank);

                    if (!result.is_found())
                    {
                        assert_equals(result.rank(), pair.rank, "{} {}", start, pair.rank);
                    }
                    else {
                        assert_equals(result.local_pos(),  pair.idx, "{} {}", start, pair.rank);
                    }
                }
            }
        }

        out() << std::endl;

        size_t seqrank_ = this->rank(seq, 0, seq->size(), symbol);
        assertSelectBW(seq, seq->size(), seqrank_/2, symbol);
    }
};


}}
