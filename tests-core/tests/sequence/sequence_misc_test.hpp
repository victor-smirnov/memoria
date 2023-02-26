
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
class SequenceMiscTest: public SequenceTestBase<AlphabetSize, Use64BitSize> {

    using MyType = SequenceMiscTest<AlphabetSize, Use64BitSize>;
    using Base = SequenceTestBase<AlphabetSize, Use64BitSize>;

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
    using typename Base::CtrID;
    using typename Base::CtrSizeT;


    using Base::create_sequence_ctr;
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
    using Base::branch;
    using Base::commit;
    using Base::println;


public:

    SequenceMiscTest()
    {}

    static void init_suite(TestSuite& suite){
        MMA_CLASS_TESTS(suite,
            test_create,
            test_access,
            test_insert,
            test_remove
        );
    }

    void test_create()
    {
        for (size_t c = 1; c <= 1024 * 1024 * 8; c *= 2) {
            test_create(c);
        }
    }


    void test_create(size_t symbol_runs)
    {
        println("Test Create: {} symbol runs", symbol_runs);

        auto snp = branch();

        std::vector<SymbolsRunT> seq_data = make_random_sequence(symbol_runs);
        println("\tSeq data size: {}", count(seq_data));

        auto ctr = create_sequence_ctr(seq_data);
        println("\tSeq size: {}", ctr->size());

        auto runs = ctr->read(CtrSizeT{});

        println("\tSeq data size: {}", count(runs));

        assert_spans_equal(seq_data, runs);

        snp->commit();
    }


    template <typename T, typename CtrT>
    void doQueries(
            CtrT seq,
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
            SymbolT sym2 = seq->seek_entry(pos)->current_symbol();

            yield();

            try {
                assert_equals(sym1, sym2);
            }
            catch (...) {
                throw;
            }
        }
    }

    void test_access()
    {
        for (size_t data_size = 1; data_size <= 1024*1024; data_size *= 2)
        {
            test_access(data_size);
        }
    }

    void test_access(size_t symbol_runs)
    {
        println("Test Access: {} symbol runs", symbol_runs);

        auto snp = branch();

        std::vector<SymbolsRunT> syms1    = make_random_sequence(symbol_runs);
        std::vector<BlockSize> size_index = build_size_index(syms1);

        auto seq = create_sequence_ctr(syms1);

        size_t queries = symbol_runs / 2;
        doQueries(seq, syms1, size_index, queries);

        snp->commit();
    }


    void test_insert()
    {
        for (size_t data_size = 8; data_size <= 1024 * 1024; data_size *= 2) {
            test_insert(data_size);
        }
    }

    void test_insert(size_t data_size)
    {
        auto snp = branch();

        println("test_insert: DataSize: {}", data_size);

        std::vector<SymbolsRunT> syms = make_random_sequence(data_size);
        SeqSizeT size = count(syms);

        size_t times = 16;
        auto ctr = create_sequence_ctr(syms);

        for (size_t cc = 0; cc < times; cc++)
        {
            std::vector<SymbolsRunT> src = make_random_sequence(data_size / 8);
            SeqSizeT pos = getBIRandomG(static_cast<uint64_t>(size) + 1);

            syms = insert_to_buffer(syms, src, pos);

            ctr->insert(pos, to_const_span(src));

            std::vector<SymbolsRunT> vv = ctr->read(CtrSizeT{});
            assert_spans_equal(syms, vv);

            std::vector<BlockSize> size_index = build_size_index(syms);
            size_t queries = data_size >= 128 ? data_size / 32 : data_size / 2;
            doQueries(ctr, syms, size_index, queries);
        }
    }

    void test_remove()
    {
        for (size_t data_size = 8; data_size <= 1024*1024; data_size *= 2)
        {
            test_remove(data_size);
        }
    }

    void test_remove(size_t data_size)
    {
        auto snp = branch();

        println("test_remove: DataSize: {}", data_size);

        std::vector<SymbolsRunT> syms = make_random_sequence(data_size);

        auto ctr = create_sequence_ctr(syms);

        for (size_t cc = 0; cc < 8; cc++)
        {
            uint64_t size = count(syms);

            std::vector<SymbolsRunT> src = make_random_sequence(data_size / 8);
            uint64_t start = getBIRandomG(size);
            uint64_t end   = start + getBIRandomG(size - start + 1);

            syms = remove_from_buffer(syms, start, end);


            ctr->remove(start, end);

            std::vector<SymbolsRunT> vv = ctr->read(CtrSizeT{});

            assert_spans_equal(syms, vv);

            std::vector<BlockSize> size_index = build_size_index(syms);
            size_t queries = data_size / 2;
            doQueries(ctr, syms, size_index, queries);
        }

        snp->commit();
    }

};


}}
