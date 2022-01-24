
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
class PackedSSRLESearchableSequenceCountTest: public PackedSSRLESequenceTestBase<Bps> {

    using MyType = PackedSSRLESearchableSequenceCountTest<Bps>;
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
    using Base::count_fw;
    using Base::count_bw;

    using Base::push_back;
    using Base::split_runs;
    using Base::out;
    using Base::size_;
    using Base::iterations_;

public:

    PackedSSRLESearchableSequenceCountTest(){}

    static void init_suite(TestSuite& suite){
        MMA_CLASS_TESTS(suite, testCountFw, testCountBw);
    }

    void testCountFw()
    {
        std::vector<SymbolsRunT> syms1    = make_random_sequence(10240);
        std::vector<BlockSize> size_index = build_size_index(syms1);
        std::vector<BlockRank> rank_index = build_rank_index(syms1);
        uint64_t size = count(syms1);

        SeqPtr seq = make_sequence(syms1);

        size_t queries = 10000;

        std::vector<size_t> poss;

        for (size_t c = 0; c < queries; c++) {
            poss.push_back(getBIRandomG(size));
        }

        int64_t t0 = getTimeInMillis();

        for (size_t c = 30; c < queries; c++)
        {
            uint64_t pos = poss[c];

            size_t sym = get_symbol(size_index, syms1, pos);

            uint64_t pos1 = count_fw(rank_index, syms1, pos, sym);
            uint64_t pos2 = seq->count_fw(pos, sym);

            try {
                assert_equals(pos1, pos2);
            }
            catch (...) {
                throw;
            }
        }

        int64_t t1 = getTimeInMillis();

        println(out(), "Query time: {}", FormatTime(t1 - t0));
    }

    void testCountBw()
    {
        std::vector<SymbolsRunT> syms1    = make_random_sequence(10240);
        std::vector<BlockSize> size_index = build_size_index(syms1);
        std::vector<BlockRank> rank_index = build_rank_index(syms1);
        uint64_t size = count(syms1);

        SeqPtr seq = make_sequence(syms1);

        size_t queries = 10000;

        std::vector<size_t> poss;

        for (size_t c = 0; c < queries; c++) {
            poss.push_back(getBIRandomG(size));
        }

        int64_t t0 = getTimeInMillis();

        for (size_t c = 30; c < queries; c++)
        {
            uint64_t pos = poss[c];

            size_t sym = get_symbol(size_index, syms1, pos);

            uint64_t pos1 = count_bw(rank_index, syms1, pos, sym);
            uint64_t pos2 = seq->count_bw(pos, sym);

            try {
                assert_equals(pos1, pos2);
            }
            catch (...) {
                throw;
            }
        }

        int64_t t1 = getTimeInMillis();

        println(out(), "Query time: {}", FormatTime(t1 - t0));
    }
};


}}
