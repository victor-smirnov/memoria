
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

template <size_t AlphabetSize, bool Use64BitSize = false>
class PackedSSRLESearchableSequenceSelectTest: public PackedSSRLESequenceTestBase<AlphabetSize, Use64BitSize> {

    using MyType = PackedSSRLESearchableSequenceSelectTest<AlphabetSize, Use64BitSize>;
    using Base = PackedSSRLESequenceTestBase<AlphabetSize, Use64BitSize>;

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


public:

    PackedSSRLESearchableSequenceSelectTest(){}

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TESTS(
                    suite,
                    testSelectEq,
                    testSelectNeq,
                    testSelectGt,
                    testSelectGe,
                    testSelectLt,
                    testSelectLe
        );
    }

    struct SelectEq {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() const {return SeqOpType::EQ;}
    };

    struct SelectNeq {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() const {return SeqOpType::NEQ;}
    };

    struct SelectLt {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() const {return SeqOpType::LT;}
    };

    struct SelectLe {
        size_t get_sym() const {
            return AlphabetSize / 2 - 1;
        }

        SeqOpType op_type() const {return SeqOpType::LE;}
    };


    struct SelectGt {
        size_t get_sym() const {
            return AlphabetSize / 2 - 1;
        }

        SeqOpType op_type() const {return SeqOpType::GT;}
    };

    struct SelectGe {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() const {return SeqOpType::GE;}
    };



    template <typename Fn>
    void testSelectFn(Fn&& fn)
    {
        for (size_t data_size = 2; data_size <= 32768; data_size *= 2)
        {
            println("DataSize: {}", data_size);
            std::vector<SymbolsRunT> syms1    = make_random_sequence(data_size);
            std::vector<BlockRank> rank_index = build_rank_index(syms1);
            uint64_t size = count(syms1);

            size_t sym = fn.get_sym();
            uint64_t rank0_max = get_rank(rank_index, syms1, size, sym, fn.op_type());

            SeqPtr seq_ss = make_sequence(syms1);
            SeqSO seq = get_so(seq_ss);

            size_t queries = data_size / 2;
            std::vector<size_t> ranks;
            for (size_t c = 0; c < queries; c++) {
                ranks.push_back(getBIRandomG(rank0_max));
            }

            for (size_t c = 0; c < queries; c++) {
                SeqSizeT rank = ranks[c];

                SeqSizeT pos1 = select(rank_index, syms1, rank, sym, fn.op_type()).global_pos();
                SeqSizeT pos2 = seq.select_fw(rank, sym, fn.op_type()).idx;

                try {
                    assert_equals(pos1, pos2);
                }
                catch (...) {
                    throw;
                }
            }
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
};


}}
