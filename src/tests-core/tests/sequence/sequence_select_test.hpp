
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

#include "sequence_test_base.hpp"

namespace memoria {
namespace tests {

template <size_t AlphabetSize, bool Use64BitSize = true>
class SequenceSelectTest: public SequenceTestBase<AlphabetSize, Use64BitSize> {

    using MyType = SequenceSelectTest<AlphabetSize, Use64BitSize>;
    using Base   = SequenceTestBase<AlphabetSize, Use64BitSize>;

    using typename Base::SymbolsRunT;
    using typename Base::RunTraits;

    using typename Base::Seq;    
    using typename Base::SeqSO;
    using typename Base::SeqPtr;

    using typename Base::BlockSize;
    using typename Base::BlockRank;
    using typename Base::LocateResult;

    using typename Base::SymbolT;
    using typename Base::SeqSizeT;

    using Base::getRandom;
    using Base::getRandom1;
    using Base::make_empty_sequence;
    using Base::make_random_sequence;
    using Base::make_sequence;
    using Base::assert_spans_equal;
    using Base::count;
    using Base::get_so;


    using Base::build_size_index;
    using Base::get_symbol;

    using Base::build_rank_index;
    using Base::get_rank;
    using Base::get_ranks;
    using Base::select;

    using Base::push_back;
    using Base::split_runs;
    using Base::out;
    using Base::size_;
    using Base::branch;
    using Base::create_sequence_ctr;


public:

    SequenceSelectTest(){}

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TESTS(
                    suite,
                    testSelectEq,
                    testSelectNeq,
                    testSelectGt,
                    testSelectGe,
                    testSelectLt,
                    testSelectLe,

                    testSelectFwBwEq,
                    testSelectFwBwNeq,
                    testSelectFwBwGt,
                    testSelectFwBwGe,
                    testSelectFwBwLt,
                    testSelectFwBwLe
        );
    }

    struct SelectEq {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() {return SeqOpType::EQ;}
    };

    struct SelectNeq {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() {return SeqOpType::NEQ;}
    };

    struct SelectLt {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() {return SeqOpType::LT;}
    };

    struct SelectLe {
        size_t get_sym() const {
            return AlphabetSize / 2 - 1;
        }

        SeqOpType op_type() {return SeqOpType::LE;}
    };


    struct SelectGt {
        size_t get_sym() const {
            return AlphabetSize / 2 - 1;
        }

        SeqOpType op_type() {return SeqOpType::GT;}
    };

    struct SelectGe {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() {return SeqOpType::GE;}
    };



    template <typename Fn>
    void testSelectFn(Fn&& fn)
    {
        for (size_t data_size = 2; data_size <= 1024 * 1024; data_size *= 2)
        {
            println("DataSize: {}", data_size);

            auto snp = branch();

            std::vector<SymbolsRunT> syms1    = make_random_sequence(data_size);
            std::vector<BlockRank> rank_index = build_rank_index(syms1);
            uint64_t size = count(syms1);

            size_t sym = fn.get_sym();
            uint64_t rank0_max = get_rank(rank_index, syms1, size, sym, fn.op_type());

            auto ctr = create_sequence_ctr(syms1);

            size_t queries = data_size / 2;
            std::vector<size_t> ranks;
            for (size_t c = 0; c < queries; c++) {
                ranks.push_back(getBIRandomG(rank0_max));
            }

            for (size_t c = 0; c < queries; c++) {
                SeqSizeT rank = ranks[c];

                SeqSizeT pos1 = select(rank_index, syms1, rank, sym, fn.op_type()).global_pos();
                SeqSizeT pos2 = ctr->select(sym, rank, fn.op_type())->entry_offset();

                try {
                    assert_equals(pos1, pos2);
                }
                catch (...) {
                    throw;
                }
            }

            snp->commit();
        }
    }

    void testSelectEq() {
        testSelectFn(SelectEq());
    }

    void testSelectNeq() {
        testSelectFn(SelectNeq());
    }

    void testSelectGt() {
        testSelectFn(SelectGt());
    }

    void testSelectGe() {
        testSelectFn(SelectGe());
    }

    void testSelectLt() {
        testSelectFn(SelectLt());
    }

    void testSelectLe() {
        testSelectFn(SelectLe());
    }





    void testSelectFwBwEq() {
        testSelectFwBwFn(SelectEq());
    }

    void testSelectFwBwNeq() {
        testSelectFwBwFn(SelectNeq());
    }

    void testSelectFwBwGt() {
        testSelectFwBwFn(SelectGt());
    }


    void testSelectFwBwGe() {
        testSelectFwBwFn(SelectGe());
    }

    void testSelectFwBwLt() {
        testSelectFwBwFn(SelectLt());
    }

    void testSelectFwBwLe() {
        testSelectFwBwFn(SelectLe());
    }


    template <typename Fn>
    void testSelectFwBwFn(Fn&& fn)
    {
        for (size_t data_size = 2048; data_size <= 1024 * 1024; data_size *= 2)
        {
            println("DataSize: {}", data_size);

            auto snp = branch();

            std::vector<SymbolsRunT> syms1    = make_random_sequence(data_size);
            std::vector<BlockRank> rank_index = build_rank_index(syms1);
            uint64_t size = count(syms1);

            auto ctr = create_sequence_ctr(syms1);

            size_t queries = data_size / 2;
            size_t sym = fn.get_sym();

            SeqSizeT ONE{1};

            for (size_t c = 0; c < queries; c++)
            {
                SeqSizeT x0 = getRandom(div_2(size));
                SeqSizeT x1 = x0 + getRandom(size - x0);

                SeqSizeT rank = ctr->rank(x0, x1, sym, fn.op_type());

                if (rank)
                {
                    auto ii = ctr->seek_entry(x0);
                    auto jj = ii->select_fw(sym, rank - ONE, fn.op_type());

                    auto fw_tgt_pos = jj->entry_offset();
                    assert_lt(fw_tgt_pos, x1);

                    if (fw_tgt_pos < x1) {
                        auto rnk_last = ctr->rank(fw_tgt_pos + ONE, x1, sym, fn.op_type());
                        assert_equals(0, rnk_last);
                    }

                    if (x1 && x1 > x0)
                    {
                        auto kk = ctr->seek_entry(x1 - ONE);
                        auto ll = kk->select_bw(sym, rank - ONE, fn.op_type());

                        auto bw_tgt_pos = ll->entry_offset();
                        assert_ge(bw_tgt_pos, x0);

                        if (bw_tgt_pos > x0) {
                            auto rnk_last = ctr->rank(bw_tgt_pos, x0, sym, fn.op_type());
                            assert_equals(0, rnk_last);
                        }
                    }

                }
            }

            snp->commit();
        }
    }
};


}}
