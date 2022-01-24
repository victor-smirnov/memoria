
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/tools/time.hpp>

#include <memory>

#include "ssrleseq_test_base.hpp"

namespace memoria {
namespace tests {

template <int32_t Bps>
class PackedSSRLESearchableSequenceRankTest: public PackedSSRLESequenceTestBase<Bps> {

    using MyType = PackedSSRLESearchableSequenceRankTest<Bps>;
    using Base = PackedSSRLESequenceTestBase<Bps>;

    using typename Base::SymbolsRunT;
    using typename Base::RunTraits;

    using typename Base::Seq;    
    using typename Base::SeqPtr;

    using typename Base::BlockSize;
    using typename Base::BlockRank;
    using typename Base::LocateResult;

    using Value = typename Seq::Value;

    using Base::Symbols;

    using Base::getRandom;
    using Base::getRandom1;
    using Base::make_empty_sequence;
    using Base::make_random_sequence;
    using Base::make_sequence;
    using Base::assert_spans_equal;
    using Base::count;


    using Base::build_size_index;
    using Base::get_symbol;

    using Base::build_rank_index;
    using Base::get_rank_eq;
    using Base::get_ranks;

    using Base::push_back;
    using Base::split_runs;
    using Base::out;
    using Base::size_;
    using Base::iterations_;

public:
    PackedSSRLESearchableSequenceRankTest(){}

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TESTS(suite, testRankEq, testRankNeq, testRankGt, testRankGe, testRankLt, testRankLe, testRanks);
    }

    struct RankEq {
        uint64_t get_rank(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const {
            return test->get_rank_eq(index, runs, idx, symbol);
        }

        uint64_t rank(SeqPtr seq, size_t idx, size_t symbol) const {
            return seq->rank_eq(idx, symbol);
        }
    };

    struct RankLt {
        uint64_t get_rank(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const {
            return test->get_rank_lt(index, runs, idx, symbol);
        }

        uint64_t rank(SeqPtr seq, size_t idx, size_t symbol) const {
            return seq->rank_lt(idx, symbol);
        }
    };

    struct RankLe {
        uint64_t get_rank(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const {
            return test->get_rank_le(index, runs, idx, symbol);
        }

        uint64_t rank(SeqPtr seq, size_t idx, size_t symbol) const {
            return seq->rank_le(idx, symbol);
        }
    };

    struct RankGt {
        uint64_t get_rank(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const {
            return test->get_rank_gt(index, runs, idx, symbol);
        }

        uint64_t rank(SeqPtr seq, size_t idx, size_t symbol) const {
            return seq->rank_gt(idx, symbol);
        }
    };

    struct RankGe {
        uint64_t get_rank(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const {
            return test->get_rank_ge(index, runs, idx, symbol);
        }

        uint64_t rank(SeqPtr seq, size_t idx, size_t symbol) const {
            return seq->rank_ge(idx, symbol);
        }
    };

    struct RankNeq {
        uint64_t get_rank(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const {
            return test->get_rank_neq(index, runs, idx, symbol);
        }

        uint64_t rank(SeqPtr seq, size_t idx, size_t symbol) const {
            return seq->rank_neq(idx, symbol);
        }
    };

    template <typename Fn>
    void testRankFn(Fn&& fn)
    {
        std::vector<SymbolsRunT> syms1    = make_random_sequence(10240);
        std::vector<BlockRank> rank_index = build_rank_index(syms1);
        uint64_t size = count(syms1);

        SeqPtr seq = make_sequence(syms1);

        size_t queries = 10000;

        std::vector<size_t> poss;

        for (size_t c = 0; c < queries; c++) {
            poss.push_back(getBIRandomG(size));
        }

        int64_t t0 = getTimeInMillis();

        for (size_t c = 0; c < queries; c++)
        {
            uint64_t pos = poss[c];

            uint64_t rank1 = fn.get_rank(this, rank_index, syms1, pos, 0);
            uint64_t rank2 = fn.rank(seq, pos, 0);

            try {
                assert_equals(rank1, rank2);
            }
            catch (...) {
                throw;
            }
        }

        int64_t t1 = getTimeInMillis();

        println(out(), "Query time: {}", FormatTime(t1 - t0));
    }

    void testRankEq()
    {
        testRankFn(RankEq());
    }

    void testRankNeq()
    {
        testRankFn(RankNeq());
    }

    void testRankGt()
    {
        testRankFn(RankGt());
    }

    void testRankGe()
    {
        testRankFn(RankGe());
    }

    void testRankLt()
    {
        testRankFn(RankLt());
    }

    void testRankLe()
    {
        testRankFn(RankLe());
    }

    void testRanks()
    {
        std::vector<SymbolsRunT> syms1    = make_random_sequence(10240);
        std::vector<BlockRank> rank_index = build_rank_index(syms1);
        uint64_t size = count(syms1);

        SeqPtr seq = make_sequence(syms1);

        size_t queries = 10000;

        std::vector<size_t> poss;

        for (size_t c = 0; c < queries; c++) {
            poss.push_back(getBIRandomG(size));
        }

        int64_t t0 = getTimeInMillis();

        for (size_t c = 0; c < queries; c++)
        {
            uint64_t pos = poss[c];

            uint64_t ranks1[Symbols]{0,};
            get_ranks(rank_index, syms1, pos, Span<uint64_t>(ranks1, Symbols));

            uint64_t ranks2[Symbols]{0,};
            seq->ranks(pos, Span<uint64_t>(ranks2, Symbols));

            for (size_t c = 0; c < Symbols; c++) {
                assert_equals(ranks1[c], ranks2[c]);
            }
        }

        int64_t t1 = getTimeInMillis();
        println(out(), "Query time: {}", FormatTime(t1 - t0));
    }
};


}}
