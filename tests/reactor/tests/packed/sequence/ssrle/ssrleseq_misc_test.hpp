
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
class PackedSSRLESearchableSequenceMiscTest: public PackedSSRLESequenceTestBase<AlphabetSize, Use64BitSize> {

    using MyType = PackedSSRLESearchableSequenceMiscTest<AlphabetSize, Use64BitSize>;
    using Base = PackedSSRLESequenceTestBase<AlphabetSize, Use64BitSize>;

    using typename Base::SymbolsRunT;
    using typename Base::RunTraits;

    using typename Base::Seq;
    using typename Base::SeqSO;
    using typename Base::SeqPtr;

    using typename Base::BlockSize;
    using typename Base::BlockRank;
    using typename Base::LocateResult;
    using typename Base::SplitBufResult;
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
    using Base::get_rank_eq;
    using Base::get_ranks;
    using Base::split_buffer;
    using Base::insert_to_buffer;
    using Base::remove_from_buffer;
    using Base::append_all;

    using Base::select_fw_eq;

    using Base::push_back;
    using Base::split_runs;
    using Base::out;
    using Base::size_;

public:

    PackedSSRLESearchableSequenceMiscTest()
    {}

    static void init_suite(TestSuite& suite){
        MMA_CLASS_TESTS(suite,
            testRunSequence,
            testCreate,
            testAccess,
            testSplitBuffer,
            testSplit,
            testMerge,
            testInsert,
            testRemove
        );
    }

    void testRunSequence()
    {
        for (size_t data_size = 2; data_size <= 32768; data_size *= 2)
        {
            println("DataSize: {}", data_size);
            auto seq1 = make_random_sequence(data_size);
            std::vector<SymbolsRunT> seq2 = split_runs(seq1);
            assert_spans_equal(seq1, seq2);
        }
    }

    template <typename T>
    void doQueries(
            SeqSO seq,
            const std::vector<SymbolsRunT>& syms,
            std::vector<T> size_index,
            size_t queries
    ) const {
        SeqSizeT size = count(syms);
        std::vector<size_t> poss;

        for (size_t c = 0; c < queries; c++) {
            poss.push_back(getBIRandomG(static_cast<size_t>(size)));
        }

        for (size_t c = 0; c < queries; c++)
        {
            uint64_t pos = poss[c];

            SymbolT sym1 = get_symbol(size_index, syms, pos);
            SymbolT sym2 = seq.access(pos);

            try {
                assert_equals(sym1, sym2);
            }
            catch (...) {
                throw;
            }
        }
    }

    void testAccess()
    {
        for (size_t data_size = 64; data_size <= 32768; data_size *= 2)
        {
            println("DataSize: {}", data_size);
            std::vector<SymbolsRunT> syms1    = make_random_sequence(data_size);
            std::vector<BlockSize> size_index = build_size_index(syms1);

            SeqPtr seq_ss = make_sequence(syms1);
            SeqSO seq = get_so(seq_ss);

            size_t queries = data_size / 2;
            doQueries(seq, syms1, size_index, queries);
        }
    }




    void testCreate()
    {
        for (size_t data_size = 2; data_size <= 32768; data_size *= 2)
        {
            println("DataSize: {}", data_size);
            std::vector<SymbolsRunT> syms1 = make_random_sequence(data_size);
            SeqSizeT size = count(syms1);

            SeqPtr seq_ss = make_sequence(syms1);
            SeqSO seq = get_so(seq_ss);

            seq.check();

            assert_equals(size, SeqSizeT{seq.size()});

            std::vector<SymbolsRunT> syms2 = seq.iterator().as_vector();
            assert_spans_equal(syms1, syms2);
        }
    }

    void testSplitBuffer()
    {
        std::vector<SymbolsRunT> syms1 = make_random_sequence(51);
        uint64_t pos = count(syms1);
        SplitBufResult res = split_buffer(syms1, pos / 2);

        auto buf = res.left;
        append_all(buf, to_const_span(res.right));
        assert_spans_equal(syms1, buf);
    }


    void testSplit()
    {
        for (size_t data_size = 8; data_size <= 32768; data_size *= 2)
        {
            println("DataSize: {}", data_size);

            std::vector<SymbolsRunT> syms1 = make_random_sequence(data_size);

            SeqSizeT size = count(syms1);
            SeqSizeT split_at = static_cast<uint64_t>(size) / 2;

            SplitBufResult res = split_buffer(syms1, split_at);

            SeqPtr seq1_ss = make_sequence(syms1);
            SeqSO seq1 = get_so(seq1_ss);

            seq1.check();

            assert_equals(size, SeqSizeT{seq1.size()});

            SeqPtr seq2_ss = make_sequence(syms1);
            SeqSO seq2 = get_so(seq2_ss);


            seq2.clear();
            assert_equals(SeqSizeT{0}, SeqSizeT{seq2.size()});
            assert_equals(0, seq2.data()->code_units());

            seq1.split_to(seq2, split_at);

            std::vector<SymbolsRunT> vec1 = seq1.iterator().as_vector();
            assert_equals(split_at, count(vec1));
            assert_spans_equal(res.left, vec1);

            assert_equals(split_at, seq1.size());
            assert_equals(size - split_at, seq2.size());

            std::vector<SymbolsRunT> vec2 = seq2.iterator().as_vector();
            assert_equals(size - split_at, count(vec2));

            assert_spans_equal(res.right, vec2);

            std::vector<BlockSize> size_index = build_size_index(res.right);
            size_t queries = data_size / 2;
            doQueries(seq2, res.right, size_index, queries);
        }
    }



    void testMerge()
    {
        for (size_t data_size = 8; data_size <= 32768; data_size *= 2)
        {
            println("DataSize: {}", data_size);

            std::vector<SymbolsRunT> syms1 = make_random_sequence(data_size);
            std::vector<SymbolsRunT> syms2 = make_random_sequence(data_size);
            std::vector<SymbolsRunT> syms3 = syms2;
            append_all(syms3, to_span(syms1));

            SeqSizeT size3 = count(syms3);

            SeqPtr seq1_ss = make_sequence(syms1);
            SeqSO seq1 = get_so(seq1_ss);

            seq1.check();


            SeqPtr seq2_ss = make_sequence(syms2, 10);
            SeqSO seq2 = get_so(seq2_ss);

            seq2.check();

            auto state = seq2.make_update_state();
            assert_success(seq1.prepare_merge_with(seq2, state.first));
            seq1.commit_merge_with(seq2, state.first);
            assert_equals(size3, seq2.size());

            std::vector<SymbolsRunT> vec3 = seq2.iterator().as_vector();
            assert_equals(size3, count(vec3));

            assert_spans_equal(syms3, vec3);

            std::vector<BlockSize> size_index = build_size_index(syms3);
            size_t queries = data_size / 2;
            doQueries(seq2, syms3, size_index, queries);
        }
    }


    void testInsert()
    {
        for (size_t data_size = 8; data_size <= 32768; data_size *= 2)
        {
            println("DataSize: {}", data_size);

            std::vector<SymbolsRunT> syms = make_random_sequence(data_size);
            SeqSizeT size = count(syms);

            size_t times = 16;
            SeqPtr seq_ss = make_sequence(syms, times);
            SeqSO seq = get_so(seq_ss);

            for (size_t cc = 0; cc < times; cc++)
            {
                std::vector<SymbolsRunT> src = make_random_sequence(data_size / 8);
                SeqSizeT pos = getBIRandomG(static_cast<uint64_t>(size) + 1);

                syms = insert_to_buffer(syms, src, pos);

                auto update_state = seq.make_update_state();
                assert_success(seq.prepare_insert(pos, update_state.first, src));
                seq.commit_insert(pos, update_state.first, src);
                seq.check();

                std::vector<SymbolsRunT> vv = seq.iterator().as_vector();

                assert_spans_equal(syms, vv);

                std::vector<BlockSize> size_index = build_size_index(syms);
                size_t queries = data_size / 2;
                doQueries(seq, syms, size_index, queries);
            }
        }
    }

    void testRemove()
    {
        for (size_t data_size = 8; data_size <= 32768; data_size *= 2)
        {
            println("DataSize: {}", data_size);

            std::vector<SymbolsRunT> syms = make_random_sequence(data_size);

            SeqPtr seq_ss = make_sequence(syms, 2);
            SeqSO seq = get_so(seq_ss);

            for (size_t cc = 0; cc < 8; cc++)
            {
                uint64_t size = count(syms);

                std::vector<SymbolsRunT> src = make_random_sequence(data_size / 8);
                uint64_t start = getBIRandomG(size);
                uint64_t end   = start + getBIRandomG(size - start + 1);

                syms = remove_from_buffer(syms, start, end);

                auto state = seq.make_update_state();
                assert_success(seq.prepare_remove(start, end, state.first));
                seq.commit_remove(start, end, state.first);
                seq.check();

                std::vector<SymbolsRunT> vv = seq.iterator().as_vector();

                assert_spans_equal(syms, vv);

                std::vector<BlockSize> size_index = build_size_index(syms);
                size_t queries = data_size / 2;
                doQueries(seq, syms, size_index, queries);
            }
        }
    }

};


}}
