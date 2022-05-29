
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

template <size_t AlphabetSize, bool Use64BitSize = false>
class SequenceRankTest: public SequenceTestBase<AlphabetSize, Use64BitSize> {

    using MyType = SequenceRankTest<AlphabetSize, Use64BitSize>;
    using Base = SequenceTestBase<AlphabetSize, Use64BitSize>;

    using typename Base::SymbolsRunT;
    using typename Base::RunTraits;

    using typename Base::Seq;
    using typename Base::SeqSO;
    using typename Base::SeqPtr;

    using typename Base::BlockSize;
    using typename Base::BlockRank;
    using typename Base::LocateResult;
    using typename Base::SeqSizeT;
    using typename Base::RunSizeT;
    using typename Base::SymbolT;

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

    using Base::push_back;
    using Base::split_runs;
    using Base::out;
    using Base::size_;
    using Base::branch;
    using Base::create_sequence_ctr;

public:
    SequenceRankTest(){}

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TESTS(suite, testRankEq, testRankNeq, testRankGt, testRankGe, testRankLt, testRankLe);
    }

    struct RankEq {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() {return SeqOpType::EQ;}
    };

    struct RankLt {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() {return SeqOpType::LT;}
    };

    struct RankLe {
        size_t get_sym() const {
            return AlphabetSize / 2 - 1;
        }

        SeqOpType op_type() {return SeqOpType::LE;}
    };

    struct RankGt {
        size_t get_sym() const {
            return AlphabetSize / 2 - 1;
        }

        SeqOpType op_type() {return SeqOpType::GT;}
    };

    struct RankGe {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() {return SeqOpType::GE;}
    };

    struct RankNeq {
        size_t get_sym() const {
            return AlphabetSize / 2;
        }

        SeqOpType op_type() {return SeqOpType::NEQ;}
    };

    template <typename Fn>
    void testRankFn(Fn&& fn)
    {
        for (size_t data_size = 2; data_size <= 1024*1024; data_size *= 2)
        {
            println("DataSize: {}", data_size);
            auto snp = branch();

            std::vector<SymbolsRunT> syms1    = make_random_sequence(data_size);
            std::vector<BlockRank> rank_index = build_rank_index(syms1);
            SeqSizeT size = count(syms1);

            auto ctr = create_sequence_ctr(to_const_span(syms1));

            size_t queries = data_size / 2;

            std::vector<size_t> poss;

            for (size_t c = 0; c < queries; c++) {
                poss.push_back(getBIRandomG(static_cast<size_t>(size)));
            }

            int64_t t0 = getTimeInMillis();

            SymbolT sym = fn.get_sym();

            for (size_t c = 0; c < queries; c++)
            {
                uint64_t pos = poss[c];

                SeqSizeT rank1 = get_rank(rank_index, syms1, pos, sym, fn.op_type());
                SeqSizeT rank2 = ctr->rank(pos, sym, fn.op_type());

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
};


}}
