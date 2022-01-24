
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

#include <memory>

#include "ssrleseq_test_base.hpp"

namespace memoria {
namespace tests {

template <int32_t Bps>
class PackedSSRLESearchableSequenceRLETest: public PackedSSRLESequenceTestBase<Bps> {

    using MyType = PackedSSRLESearchableSequenceRLETest<Bps>;
    using Base = PackedSSRLESequenceTestBase<Bps>;

    using typename Base::SymbolsRunT;
    using typename Base::RunTraits;

    using typename Base::Seq;    
    using typename Base::SeqPtr;

    using Value = typename Seq::Value;

    using Base::getRandom;
    using Base::getRandom1;
    using Base::make_empty_sequence;
    using Base::make_random_sequence;
    using Base::make_sequence;
    using Base::assert_spans_equal;
    using Base::push_back;
    using Base::split_runs;
    using Base::make_span;
    using Base::out;
    using Base::size_;
    using Base::iterations_;


public:

    PackedSSRLESearchableSequenceRLETest(){}

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TESTS(suite, testCreate, testRank, testCount, testSelect);
    }

    void testCount()
    {
        std::initializer_list<size_t> syms1 = {1,1,1,0,0,1,1};

        size_t run_length = 10;
        SymbolsRunT run1 = SymbolsRunT::make_run(syms1, run_length);

        size_t cnt1 = run1.count_fw(0, 1);
        assert_equals(3, cnt1);

        size_t cnt2 = run1.count_fw(1, 1);
        assert_equals(2, cnt2);

        size_t cnt3 = run1.count_fw(2, 1);
        assert_equals(1, cnt3);

        size_t cnt4 = run1.count_fw(3, 1);
        assert_equals(0, cnt4);

        size_t cnt5 = run1.count_fw(5, 1);
        assert_equals(5, cnt5);

        std::initializer_list<size_t> syms2 = {0,0,0,1,0,0,0};

        SymbolsRunT run2 = SymbolsRunT::make_run(syms2, run_length);

        size_t cnt6 = run2.count_fw(4, 0);
        assert_equals(6, cnt6);

        size_t cnt7 = run1.count_bw(2, 1);
        assert_equals(3, cnt7);

        size_t cnt8 = run1.count_bw(2 + run1.pattern_length(), 1);
        assert_equals(5, cnt8);

        size_t cnt9 = run2.count_bw(2, 0);
        assert_equals(3, cnt9);

        size_t cnt10 = run2.count_bw(2 + run1.pattern_length(), 0);
        assert_equals(6, cnt10);
    }


    void testSelect()
    {
        std::initializer_list<size_t> syms = {1,0,1,1,0,1,0};

        size_t ranks[2]{0, 0};
        for (size_t sym: syms) {
            ranks[sym]++;
        }

        size_t run_length = 10;
        SymbolsRunT run = SymbolsRunT::make_run(syms, run_length);

        size_t pattern_length = run.pattern_length();

        size_t pos1 = run.select_fw_eq(0, 0);
        assert_equals(1, pos1);

        size_t patterns = 5;
        size_t pos2 = run.select_fw_eq(ranks[0] * patterns + 1, 0);
        assert_equals(pattern_length * patterns + 4, pos2);

        size_t pos3 = run.select_fw_lt(16, 1);
        assert_equals(39, pos3);

        size_t pos4 = run.select_fw_le(21, 1);
        assert_equals(21, pos4);

        size_t pos5 = run.select_bw_lt(21, 1);
        assert_equals(20, pos5);

        size_t pos6 = run.select_bw_le(21, 1);
        assert_equals(48, pos6);
    }

    void testRank()
    {
        std::initializer_list<size_t> syms = {1,0,1,1,0,1,0};

        size_t ranks[2]{0, 0};
        for (size_t sym: syms) {
            ranks[sym]++;
        }

        size_t run_length = 10;
        SymbolsRunT run = SymbolsRunT::make_run(syms, run_length);
        size_t pattern_length = run.pattern_length();

        std::vector<size_t> ranks1(1 << Bps);
        run.full_ranks(make_span(ranks1));

        assert_equals(ranks[0] * run_length, ranks1[0]);
        assert_equals(ranks[1] * run_length, ranks1[1]);

        size_t sym0_full_rank = run.full_rank_eq(0);
        assert_equals(ranks[0] * run_length, sym0_full_rank);

        std::vector<size_t> ranks2(1 << Bps);

        size_t patterns = 2;
        run.ranks(pattern_length * patterns + 4, make_span(ranks2));

        assert_equals(ranks[0] * patterns + 1, ranks2[0]);
        assert_equals(ranks[1] * patterns + 3, ranks2[1]);

        size_t sym0_rank = run.rank_eq(pattern_length * patterns + 4, 0);
        assert_equals(ranks[0] * patterns + 1, sym0_rank);
    }

    void testCreate()
    {
        size_t pattern_length = 3;
        size_t run_length = 1;

        SymbolsRunT run1(pattern_length, 0xFFFFFFFFFFFFFFFF, run_length);

        size_t pattern = (size_t(1) << (Bps * pattern_length)) - 1;
        assert_equals(pattern, run1.pattern());
        assert_equals(pattern_length, run1.pattern_length());
        assert_equals(run_length, run1.run_length());

        std::initializer_list<size_t> syms = {1,0,1,1,0,1,0};

        SymbolsRunT run2 = SymbolsRunT::make_run(syms, run_length);

        size_t cnt{};
        for (size_t sym: syms) {
            assert_equals(sym, run2.symbol(cnt));
            cnt++;
        }

        assert_equals(cnt, run2.pattern_length());
        assert_equals(run_length, run2.run_length());

        SymbolsRunT run3;

        assert_equals(0, run3.pattern());
        assert_equals(0, run3.pattern_length());
        assert_equals(0, run3.run_length());
    }
};


}}
