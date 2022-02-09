
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
class PackedSearchableSequenceRankTest: public PackedSearchableSequenceTestBase<
    Bits,
    IndexType,
    ReindexFnType,
    SelectFnType,
    RankFnType,
    ToolsFnType
> {

    typedef PackedSearchableSequenceRankTest<
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
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::assertEmpty;
    using Base::out;

public:

    PackedSearchableSequenceRankTest()
    {
        this->size_ = 8192;
    }


    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, runTest1, runTest3, runTest4);
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


    std::vector<size_t> createEnds(const SeqPtr& seq, size_t start)
    {
        size_t max_block  = seq->size() / VPB + (seq->size() % VPB == 0 ? 0 : 1);

        std::vector<size_t> ranks;

        for (size_t block = start / VPB; block < max_block; block++)
        {
            size_t block_start = block * VPB;
            size_t block_end = block_start + VPB <= (size_t)seq->size() ? block_start + VPB : seq->size();

            appendPos(ranks, start, block_end, block_start);
            appendPos(ranks, start, block_end, block_start + 1);


            for (size_t d = 128; d < (size_t)VPB; d += 128)
            {
                if (block_start + d >= start  && block_start + d < block_end) {
                    appendPos(ranks, start, block_end, block_start + d);
                }
            }

            appendPos(ranks, start, block_end, block_end - 1);
            appendPos(ranks, start, block_end, block_end);
        }

        return ranks;
    }

    void appendPos(std::vector<size_t>& v, size_t start, size_t end, size_t pos)
    {
        if (pos >= start && pos < end)
        {
            v.push_back(pos);
        }
    }

    void assertRank(const SeqPtr& seq, size_t start, size_t end, Value symbol)
    {
        size_t localrank_  = seq->rank(start, end, symbol);
        size_t popc        = this->rank(seq, start, end, symbol);

        assert_equals(localrank_, popc);
    }

    void assertRank(const SeqPtr& seq, size_t end, Value symbol)
    {
        size_t rank = seq->rank(end, symbol);
        size_t popc = seq->rank(0, end, symbol);

        assert_equals(rank, popc, "{}", end);
    }

    void runTest1()
    {
        out() << "Parameters: Bits=" << Bits << std::endl;

        auto seq = createEmptySequence();

        fillRandom(seq, this->size_);

        assertRank(seq, 10, seq->size() - 10, 0);

        auto starts = createStarts(seq);

        for (size_t start: starts)
        {
            out() << start << std::endl;

            auto ends = createEnds(seq, start);

            for (size_t end: ends)
            {
                assertRank(seq, start, end, 0);
                assertRank(seq, start, end, Symbols - 1);
            }
        }
    }

    void runTest3()
    {
        out() << "Parameters: Bits=" << Bits << std::endl;

        auto seq = createEmptySequence();

        fillRandom(seq, this->size_);

        size_t stop = seq->size() - 1;

        for (size_t start = 0; start < seq->size(); start++)
        {
            assertRank(seq, start, stop, 0);
            assertRank(seq, start, stop, Symbols - 1);
        }
    }

    void runTest4()
    {
        out() << "Parameters: Bits=" << Bits << std::endl;

        auto seq = createEmptySequence();

        fillRandom(seq, this->size_);

        for (size_t c = 0; c <= seq->size(); c++)
        {
            assertRank(seq, c, 0);
        }
    }
};


}}
