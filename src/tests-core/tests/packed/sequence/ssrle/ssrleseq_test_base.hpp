
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

#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>

#include <memoria/core/packed/sseq/packed_ssrle_searchable_seq.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memoria/reactor/reactor.hpp>

#include <memory>
#include <array>

namespace memoria {
namespace tests {

template <
    size_t Bps
>
class PackedSSRLESequenceTestBase: public TestState {

    using Base = TestState;
    using MyType = PackedSSRLESequenceTestBase<Bps>;

protected:

    static constexpr size_t SIZE_INDEX_BLOCK = 128;

    using Seq    = PkdSSRLESeqT<Bps>;
    using SeqPtr = PkdStructSPtr<Seq>;

    using Value  = typename Seq::Value;

    int64_t size_{32768};
    int32_t iterations_{100};


    using SymbolsRunT   = SSRLERun<Bps>;
    using RunTraits     = SSRLERunTraits<Bps>;
    using AtomT         = typename RunTraits::AtomT;

    static constexpr size_t Symbols = 1 << Bps;

public:

    MMA_STATE_FILEDS(size_, iterations_);

    template <typename T1, typename T2>
    void push_back(std::vector<T1>& vv, Span<T2> span) const {
        for (const T2& val: span) {
            vv.push_back(val);
        }
    }

    template <typename T1, typename T2>
    void push_back(std::vector<T1>& vv, const std::vector<T2>& span) const {
        for (const T2& val: span) {
            vv.push_back(val);
        }
    }

    SeqPtr make_empty_sequence(size_t syms_block_size = 1024*1024) const
    {
        size_t block_size = Seq::block_size(syms_block_size);
        return MakeSharedPackedStructByBlock<Seq>(block_size);
    }

    std::vector<SymbolsRunT> make_random_sequence(size_t size) const {

        std::vector<SymbolsRunT> symbols;

        for (size_t c = 0; c < size; c++)
        {
            size_t pattern_length = getRandom1(RunTraits::max_pattern_length());
            size_t pattern = getBIRandom();
            size_t max_run_len = RunTraits::max_run_length(pattern_length);
            size_t run_len = max_run_len > 1 ? getRandom1(max_run_len + 1) : 1;

            symbols.push_back(SymbolsRunT(pattern_length, pattern, run_len));
        }

        return symbols;
    }

    SeqPtr make_sequence(Span<const SymbolsRunT> span) const
    {
        size_t num_atoms = RunTraits::compute_size(span);

        SeqPtr ptr = make_empty_sequence(num_atoms * sizeof(AtomT));

        ptr->append(span).get_or_throw();

        return ptr;
    }


    class SymIterator {
        Span<const SymbolsRunT> runs_;
        size_t run_idx_{};
        size_t pattern_idx_{};
        size_t len_idx_{};
    public:
        SymIterator(Span<const SymbolsRunT> runs, size_t run_idx = 0):
            runs_(runs), run_idx_(run_idx)
        {}

        bool is_eos() const {
            return run_idx_ >= runs_.size();
        }

        size_t symbol() const {
            return runs_[run_idx_].symbol(pattern_idx_);
        }

        bool is_run_start() const {return len_idx_ == 0 && is_pattern_start();}
        bool is_pattern_start() const {return pattern_idx_ == 0;}

        size_t pattern_idx() const {return pattern_idx_;}
        size_t len_remainder() const {return run().run_length() - len_idx_;}


        const SymbolsRunT& run() const {
            return runs_[run_idx_];
        }

        void next_sym() {
            pattern_idx_++;
            if (pattern_idx_ >= run().pattern_length()) {
                next_len();
            }
        }

        void next_run() {
            run_idx_++;
            pattern_idx_ = 0;
            len_idx_ = 0;
        }

        void next_len(size_t ll = 1)
        {
            pattern_idx_ = 0;
            len_idx_ += ll;

            if (len_idx_ >= run().run_length()) {
                len_idx_ = 0;
                run_idx_++;
            }
        }
    };


    void assert_spans_equal(Span<const SymbolsRunT> expected, Span<const SymbolsRunT> actual) const
    {
        SymIterator ei(expected);
        SymIterator ai(actual);

        size_t offset{};

        while ((!ei.is_eos()) && (!ai.is_eos()))
        {
            if (ei.is_pattern_start() && ai.is_pattern_start())
            {
                SymbolsRunT e_run = ei.run();
                SymbolsRunT a_run = ai.run();

                if (e_run.is_same_pattern(a_run))
                {
                    size_t e_run_len = ei.len_remainder();
                    size_t a_run_len = ai.len_remainder();

                    size_t run_len = std::min(e_run_len, a_run_len);

                    offset += ei.run().pattern_length() * run_len;

                    ei.next_len(run_len);
                    ai.next_len(run_len);

                    continue;
                }
            }

            size_t e_sym = ei.symbol();
            size_t a_sym = ai.symbol();

            if (e_sym == a_sym) {
                ei.next_sym();
                ai.next_sym();

                offset += 1;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR(
                            "SSRLE Run sequence mismatch at offset {}, expected: {}::{}, actual: {}::{}",
                            offset,
                            ei.run(),
                            ei.pattern_idx(),
                            ai.run(),
                            ai.pattern_idx()
                ).do_throw();
            }
        }

        if ((!ei.is_eos()) || (!ai.is_eos())) {
            MEMORIA_MAKE_GENERIC_ERROR(
                "SSRLE Run premature end of sequence at offset {}, expected is_eos: {}, actual is_eos: {}",
                offset,
                ei.is_eos(),
                ai.is_eos()
            ).do_throw();
        }
    }


    template <typename T>
    void assertIndexCorrect(const char* src, const T& seq)
    {
        try {
            seq->check().get_or_throw();
        }
        catch (Exception& e) {
            out()<<"Sequence structure check failed"<<std::endl;
            seq->dump(out());
            throw e;
        }
    }

    template <typename T>
    void assertEmpty(const T& seq)
    {
        assert_equals(0, seq->size());
        assert_equals(false, seq->has_index());
    }


    std::vector<SymbolsRunT> split_runs(Span<const SymbolsRunT> runs)
    {
        std::vector<SymbolsRunT> res;

        size_t idx{};
        for (SymbolsRunT run: runs)
        {
            if (this->getRandom(2))
            {
                size_t pos = getRandom1(run.full_run_length());

                auto split_res = run.split(pos);
                if (split_res) {
                    push_back(res, split_res.get().left.span());
                    push_back(res, split_res.get().right.span());
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Can't split run {} at {}", run, pos);
                }
            }
            else {
                res.push_back(run);
            }

            idx++;
        }
        return res;
    }

    struct BlockSize {
        size_t offset;
    };

    std::vector<BlockSize> build_size_index(Span<const SymbolsRunT> runs) const
    {
        std::vector<BlockSize> blocks;

        size_t offset{};
        for (size_t c = 0; c < runs.size(); c++)
        {
            if (c % SIZE_INDEX_BLOCK == 0) {
                blocks.emplace_back({offset});
            }

            size_t local_size = runs[c].full_run_size();
            offset += local_size;
        }

        return blocks;
    }

    struct LocateResult {
        size_t run_idx;
        size_t local_offset;
    };

    template <typename BlockT>
    size_t locate_block(Span<const BlockT> index, Span<const SymbolsRunT> runs, size_t idx) const
    {
        for (size_t i = 0; i < index.size(); i++) {
            if (idx <= index[i].offset) {
                return i - 1;
            }
        }

        return index.size() - 1;
    }

    template <typename BlockT>
    LocateResult locate(Span<const BlockT> index, Span<const SymbolsRunT> runs, size_t idx) const
    {
        LocateResult bs{runs.size(), 0};

        size_t i = locate_block(index, idx);
        size_t offset = index[i].offset;

        for (size_t c = i * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            size_t local_size = runs[c].full_run_size();
            if (offset + local_size <= idx) {
                offset += local_size;
            }
            else {
                bs.run_idx = c;
                bs.local_offset = idx - offset;
                break;
            }
        }

        return bs;
    }

    size_t get_symbol(Span<const BlockSize> index, Span<const SymbolsRunT> runs, size_t idx) const
    {
        LocateResult res = locate(index, runs, idx);
        if (res.run_idx < runs.size()) {
            return runs[res.run_idx].symbol(res.local_offset);
        }

        throw MEMORIA_MAKE_GENERIC_ERROR("RangeCheck failuer for get_sumbol: {}", idx);
    }


    struct BlockRank: BlockSize {
        std::array<size_t, 1 << Bps> rank;
    };


    std::vector<BlockRank> build_rank_index(Span<const SymbolsRunT> runs) const
    {
        std::vector<BlockRank> blocks;

        size_t offset{};
        std::array<size_t, Symbols> total_ranks{};
        total_ranks.fill(size_t{});

        for (size_t c = 0; c < runs.size(); c++)
        {
            if (c % SIZE_INDEX_BLOCK == 0) {
                blocks.emplace_back({offset, total_ranks});
            }

            size_t local_size = runs[c].full_run_size();
            runs[c].full_ranks(total_ranks);

            offset += local_size;            
        }

        return blocks;
    }

    size_t get_rank(Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const
    {
        size_t i = locate_block(index, idx);
        size_t offset = index[i].offset;
        size_t rank   = index[i].rank[symbol];

        for (size_t c = i * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            size_t local_size = runs[c].full_run_size();

            if (offset + local_size <= idx) {
                size_t local_rank = runs[c].full_rank(symbol);
                offset += local_size;
                rank += local_rank;
            }
            else {
                size_t local_idx = idx - offset;
                rank += runs[c].rank(local_idx, symbol);
                break;
            }
        }

        return rank;
    }


    LocateResult select(Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t rank, size_t symbol) const
    {
        LocateResult res{runs.size()};

        size_t i;
        for (i = 0; i < index.size(); i++) {
            if (rank <= index[i].rank) {
                break;
            }
        }

        size_t total_rank = index[i-1].rank[symbol];
        size_t offset     = index[i-1].offset;

        for (size_t c = (i - 1) * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            size_t run_rank = runs[c].full_rank(symbol);

            if (total_rank + run_rank < rank) {
                size_t local_size = runs[c].full_run_size();
                offset += local_size;
                rank += run_rank;
            }
            else {
                size_t local_rank = rank - total_rank;
                res.local_offset = runs[c].select(local_rank, symbol);
                res.run_idx = c;
                break;
            }
        }

        return res;
    }

    template <typename T>
    Span<T> make_span(std::vector<T>& vv) const noexcept {
        return Span<T>(vv.data(), vv.size());
    }
};


}}
