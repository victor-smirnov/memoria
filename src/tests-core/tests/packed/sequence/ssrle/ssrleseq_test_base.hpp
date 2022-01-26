
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

            if (max_run_len > 10000) {
                max_run_len = 10000;
            }

            size_t run_len = max_run_len > 1 ? getRandom1(max_run_len + 1) : 1;

            symbols.push_back(SymbolsRunT(pattern_length, pattern, run_len));
        }

        return symbols;
    }

    SeqPtr make_sequence(Span<const SymbolsRunT> span, size_t capacity_multiplier = 1) const
    {
        size_t num_atoms = RunTraits::compute_size(span);

        SeqPtr ptr = make_empty_sequence(num_atoms * sizeof(AtomT) * capacity_multiplier);

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
                push_back(res, split_res.left.span());
                push_back(res, split_res.right.span());
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
                blocks.push_back(BlockSize{offset});
            }

            size_t local_size = runs[c].full_run_length();
            offset += local_size;
        }

        return blocks;
    }

    struct LocateResult {
        size_t run_idx;
        size_t local_offset;
        size_t size_prefix;

        size_t global_pos() const {return size_prefix + local_offset;}
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
    LocateResult locate(Span<const BlockT> index, Span<const SymbolsRunT> runs, uint64_t idx) const
    {
        size_t i = locate_block(index, runs, idx);
        size_t offset = index[i].offset;

        return do_locate(runs, idx, i, offset);
    }

    LocateResult do_locate(Span<const SymbolsRunT> runs, uint64_t idx, uint64_t block = 0, uint64_t offset = 0) const
    {
        LocateResult bs{runs.size(), 0, 0};

        for (size_t c = block * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            size_t local_size = runs[c].full_run_length();
            if (offset + local_size <= idx) {
                offset += local_size;
            }
            else {
                bs.run_idx = c;
                bs.local_offset = idx - offset;
                bs.size_prefix = offset;
                break;
            }
        }

        return bs;
    }



    size_t get_symbol(Span<const BlockSize> index, Span<const SymbolsRunT> runs, size_t idx) const
    {
        LocateResult res = locate(index, runs, idx);
        if (res.run_idx < runs.size())
        {
            return runs[res.run_idx].symbol(res.local_offset);
        }

        throw MEMORIA_MAKE_GENERIC_ERROR("RangeCheck failuer for get_sumbol: {}", idx);
    }


    struct BlockRank: BlockSize {
        std::array<uint64_t, Symbols> rank;
    };


    std::vector<BlockRank> build_rank_index(Span<const SymbolsRunT> runs) const
    {
        std::vector<BlockRank> blocks;

        size_t offset{};
        std::array<uint64_t, Symbols> total_ranks{};
        total_ranks.fill(size_t{});

        for (size_t c = 0; c < runs.size(); c++)
        {
            if (c % SIZE_INDEX_BLOCK == 0) {
                blocks.push_back(BlockRank{offset, total_ranks});
            }

            size_t local_size = runs[c].full_run_length();
            runs[c].full_ranks(Span<uint64_t>(total_ranks.data(), total_ranks.size()));

            offset += local_size;            
        }

        return blocks;
    }

    uint64_t get_rank_eq(Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const
    {
        size_t i = locate_block(index, runs, idx);
        uint64_t offset = index[i].offset;
        uint64_t rank   = index[i].rank[symbol];

        for (size_t c = i * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            uint64_t local_size = runs[c].full_run_length();

            if (offset + local_size <= idx) {
                uint64_t local_rank = runs[c].full_rank_eq(symbol);
                offset += local_size;
                rank += local_rank;
            }
            else {
                uint64_t local_idx = idx - offset;
                rank += runs[c].rank_eq(local_idx, symbol);
                break;
            }
        }

        return rank;
    }

    uint64_t get_rank_gt(Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const
    {
        uint64_t ranks[Symbols]{0, };
        get_ranks(index, runs, idx, Span<uint64_t>(ranks, Symbols));

        uint64_t sum{};

        for (size_t c = symbol + 1; c < Symbols; c++) {
            sum += ranks[c];
        }

        return sum;
    }

    uint64_t get_rank_ge(Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const
    {
        uint64_t ranks[Symbols]{0, };
        get_ranks(index, runs, idx, Span<uint64_t>(ranks, Symbols));

        uint64_t sum{};

        for (size_t c = symbol; c < Symbols; c++) {
            sum += ranks[c];
        }

        return sum;
    }


    uint64_t get_rank_lt(Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const
    {
        uint64_t ranks[Symbols]{0, };
        get_ranks(index, runs, idx, Span<uint64_t>(ranks, Symbols));

        uint64_t sum{};

        for (size_t c = 0; c < symbol; c++) {
            sum += ranks[c];
        }

        return sum;
    }

    uint64_t get_rank_le(Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const
    {
        uint64_t ranks[Symbols]{0, };
        get_ranks(index, runs, idx, Span<uint64_t>(ranks, Symbols));

        uint64_t sum{};

        for (size_t c = 0; c <= symbol; c++) {
            sum += ranks[c];
        }

        return sum;
    }

    uint64_t get_rank_neq(Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, size_t symbol) const
    {
        uint64_t ranks[Symbols]{0, };
        get_ranks(index, runs, idx, Span<uint64_t>(ranks, Symbols));

        uint64_t sum{};

        for (size_t c = 0; c < Symbols; c++) {
            sum += c != symbol ? ranks[c] : 0;
        }

        return sum;
    }

    template <typename T>
    void get_ranks(Span<const BlockRank> index, Span<const SymbolsRunT> runs, size_t idx, Span<T> symbols) const
    {
        size_t i = locate_block(index, runs, idx);
        uint64_t offset = index[i].offset;

        for (size_t c = 0; c < Symbols; c++) {
            symbols[c] += index[i].rank[c];
        }

        for (size_t c = i * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            uint64_t local_size = runs[c].full_run_length();

            if (offset + local_size <= idx) {
                offset += local_size;
                runs[c].full_ranks(symbols);
            }
            else {
                size_t local_idx = idx - offset;
                runs[c].ranks(local_idx, symbols);
                break;
            }
        }
    }

private:

    struct SelectFwEqFn {
        uint64_t index_fn(const BlockRank& index, size_t symbol) const {
            return index.rank[symbol];
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_eq(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_eq(rank, symbol);
        }
    };

    struct SelectFwNeqFn {
        uint64_t index_fn(const BlockRank& index, size_t symbol) const {
            uint64_t sum{};
            for (size_t c = 0; c < Symbols; c++) {
                sum += c != symbol ? index.rank[c] : 0;
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_neq(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_neq(rank, symbol);
        }
    };

    struct SelectFwGtFn {
        uint64_t index_fn(const BlockRank& index, size_t symbol) const {
            uint64_t sum{};
            for (size_t c = symbol + 1; c < Symbols; c++) {
                sum += index.rank[c];
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_gt(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_gt(rank, symbol);
        }
    };

    struct SelectFwGeFn {
        uint64_t index_fn(const BlockRank& index, size_t symbol) const {
            uint64_t sum{};
            for (size_t c = symbol; c < Symbols; c++) {
                sum += index.rank[c];
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_ge(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_ge(rank, symbol);
        }
    };

    struct SelectFwLtFn {
        uint64_t index_fn(const BlockRank& index, size_t symbol) const {
            uint64_t sum{};
            for (size_t c = 0; c < symbol; c++) {
                sum += index.rank[c];
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_lt(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_lt(rank, symbol);
        }
    };

    struct SelectFwLeFn {
        uint64_t index_fn(const BlockRank& index, size_t symbol) const {
            uint64_t sum{};
            for (size_t c = 0; c <= symbol; c++) {
                sum += index.rank[c];
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_le(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_le(rank, symbol);
        }
    };


    template <typename Fn>
    LocateResult select_fw_fn(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            uint64_t rank,
            size_t symbol,
            Fn&& fn
    ) const
    {
        LocateResult res{runs.size()};

        size_t i;
        uint64_t total_rank{};
        for (i = 1; i < index.size(); i++) {
            uint64_t sum = fn.index_fn(index[i], symbol);
            if (rank < sum) {
                break;
            }
            total_rank = sum;
        }

        size_t offset = index[i - 1].offset;
        for (size_t c = (i - 1) * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            size_t run_rank = fn.full_rank_fn(runs[c], symbol);

            if (MMA_UNLIKELY(rank < total_rank + run_rank))
            {
                size_t local_rank = rank - total_rank;
                res.local_offset = fn.select_fn(runs[c], local_rank, symbol);
                res.run_idx = c;
                res.size_prefix = offset;
                break;
            }
            else {
                size_t local_size = runs[c].full_run_length();
                offset += local_size;
                total_rank += run_rank;
            }
        }

        return res;
    }

public:

    LocateResult select_fw_eq(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            size_t rank,
            size_t symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwEqFn());
    }

    LocateResult select_fw_neq(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            size_t rank,
            size_t symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwNeqFn());
    }

    LocateResult select_fw_lt(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            size_t rank,
            size_t symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwLtFn());
    }

    LocateResult select_fw_le(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            size_t rank,
            size_t symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwLeFn());
    }

    LocateResult select_fw_gt(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            size_t rank,
            size_t symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwGtFn());
    }

    LocateResult select_fw_ge(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            size_t rank,
            size_t symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwGeFn());
    }

    template <typename T>
    Span<T> make_span(std::vector<T>& vv) const noexcept {
        return Span<T>(vv.data(), vv.size());
    }

    uint64_t count(Span<const SymbolsRunT> runs) const noexcept
    {
        uint64_t size{};
        for (const SymbolsRunT& run: runs) {
            size += run.full_run_length();
        }
        return size;
    }

    /*uint64_t count_fw(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            uint64_t idx,
            size_t symbol
    ) const
    {
        LocateResult res = locate(index, runs, idx);

        uint64_t total_count{};
        size_t lpos = res.local_offset;

        for (size_t c = res.run_idx; c < runs.size(); c++)
        {
            size_t epos = runs[c].count_fw(lpos, symbol);
            if (lpos + epos < runs[c].full_run_length()) {
                total_count += epos;
                break;
            }
            else {
                total_count += epos;
                lpos = 0;
            }
        }

        return total_count;
    }*/

    uint64_t count_fw(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            uint64_t idx,
            size_t symbol
    ) const
    {
        uint64_t rank = get_rank_neq(index, runs, idx, symbol);
        uint64_t next_idx = select_fw_neq(index, runs, rank, symbol).global_pos();
        return next_idx - idx;
    }

    uint64_t count_bw(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            uint64_t idx,
            size_t symbol
    ) const
    {
        uint64_t rank = get_rank_neq(index, runs, idx, symbol);
        if (rank > 0) {
            uint64_t next_idx = select_fw_neq(index, runs, rank - 1, symbol).global_pos();
            return idx + 1 - next_idx;
        }
        else {
            return idx + 1;
        }
    }

    struct SplitBufResult {
        std::vector<SymbolsRunT> left;
        std::vector<SymbolsRunT> right;
    };

    template <typename T>
    void append_all(std::vector<T>& vv, Span<const T> span) const {
        vv.insert(vv.end(), span.begin(), span.end());
    }

    template <typename T1, typename T2>
    void append_all(std::vector<T1>& vv, Span<T2> span) const {
        vv.insert(vv.end(), span.begin(), span.end());
    }

    SplitBufResult split_buffer(Span<const SymbolsRunT> runs, uint64_t pos) const
    {
        LocateResult res = do_locate(runs, pos);

        auto s_res = runs[res.run_idx].split(res.local_offset);

        std::vector<SymbolsRunT> left(runs.begin(), runs.begin() + res.run_idx);
        append_all(left, s_res.left.span());

        std::vector<SymbolsRunT> right;
        append_all(right, s_res.right.span());

        if (res.run_idx + 1 < runs.size()) {
            append_all(right, runs.subspan(res.run_idx + 1));
        }

        return SplitBufResult{left, right};
    }

    std::vector<SymbolsRunT> insert_to_buffer(const std::vector<SymbolsRunT>& target, Span<const SymbolsRunT> source, uint64_t at) const
    {
        SplitBufResult res = split_buffer(target, at);

        append_all(res.left, source);
        append_all(res.left, to_const_span(res.right));

        return res.left;
    }

    std::vector<SymbolsRunT> remove_from_buffer(const std::vector<SymbolsRunT>& buffer, uint64_t start, uint64_t end) const
    {
        SplitBufResult res1 = split_buffer(buffer, start);
        SplitBufResult res2 = split_buffer(res1.right, end - start);

        append_all(res1.left, to_const_span(res2.right));

        return res1.left;
    }
};


}}
