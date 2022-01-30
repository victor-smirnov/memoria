
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
class PackedSSRLESearchableSequenceSelectTest: public PackedSSRLESequenceTestBase<Bps> {

    using MyType = PackedSSRLESearchableSequenceSelectTest<Bps>;
    using Base = PackedSSRLESequenceTestBase<Bps>;

    using typename Base::SymbolsRunT;
    using typename Base::RunTraits;

    using typename Base::Seq;    
    using typename Base::SeqPtr;

    using typename Base::BlockSize;
    using typename Base::BlockRank;
    using typename Base::LocateResult;

    using typename Base::SymbolT;
    using typename Base::SeqSizeT;

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


public:

    PackedSSRLESearchableSequenceSelectTest(){}

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TESTS(
                    suite,
                    testSelectFwEq,
                    testSelectFwNeq,
                    testSelectFwGt,
                    testSelectFwGe,
                    testSelectFwLt,
                    testSelectFwLe
        );
    }

    struct SelectFwEq {
        size_t get_sym() const {
            return Symbols / 2;
        }

        uint64_t get_rank_fn(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->get_rank_eq(index, runs, rank, symbol);
        }

        LocateResult test_select_fw(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->select_fw_eq(index, runs, rank, symbol);
        }

        auto seq_select_fw(SeqPtr seq, SeqSizeT rank, SymbolT symbol) const {
            return seq->select_fw_eq(rank, symbol);
        }
    };

    struct SelectFwNeq {
        size_t get_sym() const {
            return Symbols/2;
        }

        uint64_t get_rank_fn(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->get_rank_neq(index, runs, rank, symbol);
        }

        LocateResult test_select_fw(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->select_fw_neq(index, runs, rank, symbol);
        }

        auto seq_select_fw(SeqPtr seq, SeqSizeT rank, SymbolT symbol) const {
            return seq->select_fw_neq(rank, symbol);
        }
    };

    struct SelectFwLt {
        size_t get_sym() const {
            return Symbols / 2;
        }

        uint64_t get_rank_fn(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->get_rank_lt(index, runs, rank, symbol);
        }


        LocateResult test_select_fw(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->select_fw_lt(index, runs, rank, symbol);
        }

        auto seq_select_fw(SeqPtr seq, SeqSizeT rank, SymbolT symbol) const {
            return seq->select_fw_lt(rank, symbol);
        }
    };

    struct SelectFwLe {
        size_t get_sym() const {
            return Symbols / 2 - 1;
        }

        uint64_t get_rank_fn(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->get_rank_le(index, runs, rank, symbol);
        }

        LocateResult test_select_fw(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->select_fw_le(index, runs, rank, symbol);
        }

        auto seq_select_fw(SeqPtr seq, SeqSizeT rank, SymbolT symbol) const {
            return seq->select_fw_le(rank, symbol);
        }
    };


    struct SelectFwGt {
        size_t get_sym() const {
            return Symbols / 2 - 1;
        }

        uint64_t get_rank_fn(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->get_rank_gt(index, runs, rank, symbol);
        }

        LocateResult test_select_fw(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->select_fw_gt(index, runs, rank, symbol);
        }

        auto seq_select_fw(SeqPtr seq, SeqSizeT rank, SymbolT symbol) const {
            return seq->select_fw_gt(rank, symbol);
        }
    };

    struct SelectFwGe {
        size_t get_sym() const {
            return Symbols / 2;
        }

        uint64_t get_rank_fn(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->get_rank_ge(index, runs, rank, symbol);
        }

        LocateResult test_select_fw(const MyType* test, Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol) const {
            return test->select_fw_ge(index, runs, rank, symbol);
        }

        auto seq_select_fw(SeqPtr seq, SeqSizeT rank, SymbolT symbol) const {
            return seq->select_fw_ge(rank, symbol);
        }
    };



    template <typename Fn>
    void testSelectFwFn(Fn&& fn)
    {
        for (size_t data_size = 2; data_size <= 32768; data_size *= 2)
        {
            println("DataSize: {}", data_size);
            std::vector<SymbolsRunT> syms1    = make_random_sequence(data_size);
            std::vector<BlockRank> rank_index = build_rank_index(syms1);
            uint64_t size = count(syms1);

            size_t sym = fn.get_sym();
            uint64_t rank0_max = fn.get_rank_fn(this, rank_index, syms1, size, sym);

            SeqPtr seq = make_sequence(syms1);

            size_t queries = data_size / 2;
            std::vector<size_t> ranks;
            for (size_t c = 0; c < queries; c++) {
                ranks.push_back(getBIRandomG(rank0_max));
            }

            for (size_t c = 0; c < queries; c++) {
                SeqSizeT rank = ranks[c];

                SeqSizeT pos1 = fn.test_select_fw(this, rank_index, syms1, rank, sym).global_pos();
                SeqSizeT pos2 = fn.seq_select_fw(seq, rank, sym).idx;

                try {
                    assert_equals(pos1, pos2);
                }
                catch (...) {
                    throw;
                }
            }
        }
    }

    void testSelectFwEq() {
        testSelectFwFn(SelectFwEq());
    }

    void testSelectFwNeq() {
        testSelectFwFn(SelectFwNeq());
    }

    void testSelectFwGt() {
        testSelectFwFn(SelectFwGt());
    }

    void testSelectFwGe() {
        testSelectFwFn(SelectFwGe());
    }

    void testSelectFwLt() {
        testSelectFwFn(SelectFwLt());
    }

    void testSelectFwLe() {
        testSelectFwFn(SelectFwLe());
    }
};


}}
