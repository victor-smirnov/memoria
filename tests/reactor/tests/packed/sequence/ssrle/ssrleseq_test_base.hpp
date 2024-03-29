
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

#include <memoria/core/packed/sseq/packed_ssrle_seq.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memory>
#include <array>

namespace memoria {
namespace tests {


template <size_t AlphabetSize_, bool Use64BitSize>
class SymbolsRunTools {

    TestState* test_base_;

public:

    static constexpr size_t SIZE_INDEX_BLOCK = 128;
    static constexpr size_t Bps = BitsPerSymbolConstexpr(AlphabetSize_);
    static constexpr size_t AlphabetSize = AlphabetSize_;

    using PackedSeq     = PkdSSRLESeqT<AlphabetSize_, 256, Use64BitSize>;

    using SymbolsRunT   = SSRLERun<Bps>;
    using RunTraits     = SSRLERunTraits<Bps>;
    using CodeUnitT     = typename RunTraits::CodeUnitT;
    using SymbolT       = typename SymbolsRunT::SymbolT;
    using RunSizeT      = typename SymbolsRunT::RunSizeT;
    using SeqSizeT      = typename PackedSeq::SeqSizeT;


    SymbolsRunTools(TestState* test_base): test_base_(test_base) {}

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

    std::vector<SymbolsRunT> make_random_sequence(size_t size) const
    {
        std::vector<SymbolsRunT> symbols;

        for (size_t c = 0; c < size; c++)
        {
            size_t pattern_length = test_base_->getRandom1(RunTraits::max_pattern_length());
            size_t max_run_len = RunTraits::max_run_length(pattern_length);

            if (max_run_len > 10000) {
                max_run_len = 10000;
            }

            size_t run_len = max_run_len > 1 ? test_base_->getRandom1(max_run_len + 1) : 1;

            SymbolsRunT run(pattern_length, 0, run_len);

            for (size_t s = 0; s < run.pattern_length(); s++) {
                run.set_pattern_symbol(s, test_base_->getRandom(AlphabetSize));
            }

            symbols.push_back(run);
        }

        return symbols;
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

        size_t run_idx() const {return run_idx_;}

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
                            "SSRLE Run sequence mismatch at offset {}, expected: {}::{} at {}, actual: {}::{} at {}",
                            offset,
                            ei.run(),
                            ei.pattern_idx(),
                            ei.run_idx(),
                            ai.run(),
                            ai.pattern_idx(),
                            ai.run_idx()
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


    std::vector<SymbolsRunT> split_runs(Span<const SymbolsRunT> runs)
    {
        std::vector<SymbolsRunT> res;

        size_t idx{};
        for (SymbolsRunT run: runs)
        {
            if (test_base_->getRandom(2))
            {
                size_t pos = test_base_->getRandom1(run.full_run_length());
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
        SeqSizeT offset;
    };

    std::vector<BlockSize> build_size_index(Span<const SymbolsRunT> runs) const
    {
        std::vector<BlockSize> blocks;

        SeqSizeT offset{};
        for (size_t c = 0; c < runs.size(); c++)
        {
            if (c % SIZE_INDEX_BLOCK == 0) {
                blocks.push_back(BlockSize{offset});
            }

            RunSizeT local_size = runs[c].full_run_length();
            offset += SeqSizeT{local_size};
        }

        return blocks;
    }

    struct LocateResult {
        size_t run_idx;
        RunSizeT local_offset;
        SeqSizeT size_prefix;

        SeqSizeT global_pos() const {return size_prefix + SeqSizeT{local_offset};}
    };

    template <typename BlockT>
    size_t locate_block(Span<const BlockT> index, Span<const SymbolsRunT> runs, SeqSizeT idx) const
    {
        for (size_t i = 0; i < index.size(); i++) {
            if (idx < index[i].offset) {
                return i - 1;
            }
        }

        return index.size() - 1;
    }



    template <typename BlockT>
    LocateResult locate(Span<const BlockT> index, Span<const SymbolsRunT> runs, SeqSizeT idx) const
    {
        size_t i = locate_block(index, runs, idx);
        SeqSizeT offset = index[i].offset;

        return do_locate(runs, idx, i, offset);
    }

    LocateResult do_locate(Span<const SymbolsRunT> runs, SeqSizeT idx, size_t block = 0, SeqSizeT offset = SeqSizeT{}) const
    {
        LocateResult bs{runs.size()};

        for (size_t c = block * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            RunSizeT local_size = runs[c].full_run_length();

            SeqSizeT tmp = offset + SeqSizeT{local_size};
            if (tmp <= idx) {
                offset += SeqSizeT{local_size};
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



    SymbolT get_symbol(Span<const BlockSize> index, Span<const SymbolsRunT> runs, SeqSizeT idx) const
    {
        LocateResult res = locate(index, runs, idx);
        if (res.run_idx < runs.size())
        {
            return runs[res.run_idx].symbol(res.local_offset);
        }

        throw MEMORIA_MAKE_GENERIC_ERROR("RangeCheck failuer for get_sumbol: {}", idx);
    }


    struct BlockRank: BlockSize {
        std::array<SeqSizeT, AlphabetSize> rank;
    };


    std::vector<BlockRank> build_rank_index(Span<const SymbolsRunT> runs) const
    {
        std::vector<BlockRank> blocks;

        SeqSizeT offset{};
        std::array<SeqSizeT, AlphabetSize> total_ranks{};
        total_ranks.fill(SeqSizeT{});

        for (size_t c = 0; c < runs.size(); c++)
        {
            if (c % SIZE_INDEX_BLOCK == 0) {
                blocks.push_back(BlockRank{offset, total_ranks});
            }

            SeqSizeT local_size = runs[c].full_run_length();
            runs[c].full_ranks(Span<SeqSizeT>(total_ranks.data(), total_ranks.size()));

            offset += local_size;
        }

        return blocks;
    }

    uint64_t get_rank(Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT idx, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
        case SeqOpType::EQ : return get_rank_eq(index, runs, idx, symbol);
        case SeqOpType::NEQ: return get_rank_neq(index, runs, idx, symbol);
        case SeqOpType::LT : return get_rank_lt(index, runs, idx, symbol);
        case SeqOpType::LE : return get_rank_le(index, runs, idx, symbol);
        case SeqOpType::GT : return get_rank_gt(index, runs, idx, symbol);
        case SeqOpType::GE : return get_rank_ge(index, runs, idx, symbol);
        default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", static_cast<uint32_t>(op_type)).do_throw();
        }
    }

    uint64_t get_rank_eq(Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT idx, SymbolT symbol) const
    {
        size_t i = locate_block(index, runs, idx);
        SeqSizeT offset = index[i].offset;
        SeqSizeT rank   = index[i].rank[symbol];

        for (size_t c = i * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            SeqSizeT local_size = runs[c].full_run_length();

            if (offset + local_size <= idx) {
                SeqSizeT local_rank = runs[c].full_rank_eq(symbol);
                offset += local_size;
                rank += local_rank;
            }
            else {
                RunSizeT local_idx = idx - offset;
                rank += runs[c].rank_eq(local_idx, symbol);
                break;
            }
        }

        return rank;
    }

    SeqSizeT get_rank_gt(Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT idx, SymbolT symbol) const
    {
        SeqSizeT ranks[AlphabetSize]{0, };
        get_ranks(index, runs, idx, Span<SeqSizeT>(ranks, AlphabetSize));

        SeqSizeT sum{};

        for (size_t c = symbol + 1; c < AlphabetSize; c++) {
            sum += ranks[c];
        }

        return sum;
    }

    SeqSizeT get_rank_ge(Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT idx, SymbolT symbol) const
    {
        SeqSizeT ranks[AlphabetSize]{0, };
        get_ranks(index, runs, idx, Span<SeqSizeT>(ranks, AlphabetSize));

        SeqSizeT sum{};

        for (size_t c = symbol; c < AlphabetSize; c++) {
            sum += ranks[c];
        }

        return sum;
    }


    SeqSizeT get_rank_lt(Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT idx, SymbolT symbol) const
    {
        SeqSizeT ranks[AlphabetSize]{0, };
        get_ranks(index, runs, idx, Span<SeqSizeT>(ranks, AlphabetSize));

        SeqSizeT sum{};

        for (size_t c = 0; c < symbol; c++) {
            sum += ranks[c];
        }

        return sum;
    }

    SeqSizeT get_rank_le(Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT idx, SymbolT symbol) const
    {
        SeqSizeT ranks[AlphabetSize]{0, };
        get_ranks(index, runs, idx, Span<SeqSizeT>(ranks, AlphabetSize));

        SeqSizeT sum{};

        for (size_t c = 0; c <= symbol; c++) {
            sum += ranks[c];
        }

        return sum;
    }

    SeqSizeT get_rank_neq(Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT idx, SymbolT symbol) const
    {
        SeqSizeT ranks[AlphabetSize]{0, };
        get_ranks(index, runs, idx, Span<SeqSizeT>(ranks, AlphabetSize));

        SeqSizeT sum{};

        for (size_t c = 0; c < AlphabetSize; c++) {
            sum += c != symbol ? ranks[c] : SeqSizeT{0};
        }

        return sum;
    }

    template <typename T>
    void get_ranks(Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT idx, Span<T> symbols) const
    {
        size_t i = locate_block(index, runs, idx);
        SeqSizeT offset = index[i].offset;

        for (size_t c = 0; c < AlphabetSize; c++) {
            symbols[c] += index[i].rank[c];
        }

        for (size_t c = i * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            SeqSizeT local_size = runs[c].full_run_length();

            if (offset + local_size <= idx) {
                offset += local_size;
                runs[c].full_ranks(symbols);
            }
            else {
                SeqSizeT local_idx = idx - offset;
                runs[c].ranks(local_idx, symbols);
                break;
            }
        }
    }

private:

    struct SelectFwEqFn {
        SeqSizeT index_fn(const BlockRank& index, SymbolT symbol) const {
            return index.rank[symbol];
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_eq(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_eq(rank, symbol);
        }
    };

    struct SelectFwNeqFn {
        SeqSizeT index_fn(const BlockRank& index, SymbolT symbol) const {
            SeqSizeT sum{};
            for (size_t c = 0; c < AlphabetSize; c++) {
                sum += c != symbol ? index.rank[c] : SeqSizeT{0};
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_neq(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_neq(rank, symbol);
        }
    };

    struct SelectFwGtFn {
        SeqSizeT index_fn(const BlockRank& index, SymbolT symbol) const {
            SeqSizeT sum{};
            for (size_t c = symbol + 1; c < AlphabetSize; c++) {
                sum += index.rank[c];
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_gt(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_gt(rank, symbol);
        }
    };

    struct SelectFwGeFn {
        SeqSizeT index_fn(const BlockRank& index, SymbolT symbol) const {
            SeqSizeT sum{};
            for (size_t c = symbol; c < AlphabetSize; c++) {
                sum += index.rank[c];
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_ge(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_ge(rank, symbol);
        }
    };

    struct SelectFwLtFn {
        SeqSizeT index_fn(const BlockRank& index, SymbolT symbol) const {
            SeqSizeT sum{};
            for (size_t c = 0; c < symbol; c++) {
                sum += index.rank[c];
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_lt(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, uint64_t rank, SymbolT symbol) const {
            return run.select_fw_lt(rank, symbol);
        }
    };

    struct SelectFwLeFn {
        SeqSizeT index_fn(const BlockRank& index, SymbolT symbol) const {
            SeqSizeT sum{};
            for (size_t c = 0; c <= symbol; c++) {
                sum += index.rank[c];
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_le(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_le(rank, symbol);
        }
    };


    template <typename Fn>
    LocateResult select_fw_fn(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            SeqSizeT rank,
            SymbolT symbol,
            Fn&& fn
    ) const
    {
        LocateResult res{runs.size()};

        size_t i;
        SeqSizeT total_rank{};
        for (i = 1; i < index.size(); i++) {
            SeqSizeT sum = fn.index_fn(index[i], symbol);
            if (rank < sum) {
                break;
            }
            total_rank = sum;
        }

        SeqSizeT offset = index[i - 1].offset;
        for (size_t c = (i - 1) * SIZE_INDEX_BLOCK; c < runs.size(); c++)
        {
            SeqSizeT run_rank = fn.full_rank_fn(runs[c], symbol);

            if (MMA_UNLIKELY(rank < total_rank + run_rank))
            {
                SeqSizeT local_rank = rank - total_rank;
                res.local_offset = fn.select_fn(runs[c], local_rank, symbol);
                res.run_idx = c;
                res.size_prefix = offset;
                return res;
            }
            else {
                SeqSizeT local_size = runs[c].full_run_length();
                offset += local_size;
                total_rank += run_rank;
            }
        }

        res.local_offset = 0;
        res.size_prefix = offset;

        return res;
    }

public:

    LocateResult select_fw_eq(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            SeqSizeT rank,
            SymbolT symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwEqFn());
    }

    LocateResult select_fw_neq(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            SeqSizeT rank,
            SymbolT symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwNeqFn());
    }

    LocateResult select_fw_lt(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            SeqSizeT rank,
            SymbolT symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwLtFn());
    }

    LocateResult select_fw_le(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            SeqSizeT rank,
            SymbolT symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwLeFn());
    }

    LocateResult select_fw_gt(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            SeqSizeT rank,
            SymbolT symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwGtFn());
    }

    LocateResult select_fw_ge(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            SeqSizeT rank,
            SymbolT symbol
    ) const {
        return select_fw_fn(index, runs, rank, symbol, SelectFwGeFn());
    }


    LocateResult select(Span<const BlockRank> index, Span<const SymbolsRunT> runs, SeqSizeT rank, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
        case SeqOpType::EQ : return select_fw_eq(index, runs, rank, symbol);
        case SeqOpType::NEQ: return select_fw_neq(index, runs, rank, symbol);
        case SeqOpType::LT : return select_fw_lt(index, runs, rank, symbol);
        case SeqOpType::LE : return select_fw_le(index, runs, rank, symbol);
        case SeqOpType::GT : return select_fw_gt(index, runs, rank, symbol);
        case SeqOpType::GE : return select_fw_ge(index, runs, rank, symbol);
        default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", static_cast<uint32_t>(op_type)).do_throw();
        }
    }

    template <typename T>
    Span<T> make_span(std::vector<T>& vv) const noexcept {
        return Span<T>(vv.data(), vv.size());
    }

    SeqSizeT count(Span<const SymbolsRunT> runs) const noexcept
    {
        SeqSizeT size{};
        for (const SymbolsRunT& run: runs) {
            size += run.full_run_length();
        }
        return size;
    }

    SeqSizeT count_fw(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            SeqSizeT idx,
            SymbolT symbol
    ) const
    {
        SeqSizeT rank = get_rank_neq(index, runs, idx, symbol);
        SeqSizeT next_idx = select_fw_neq(index, runs, rank, symbol).global_pos();
        return next_idx - idx;
    }

    SeqSizeT count_bw(
            Span<const BlockRank> index,
            Span<const SymbolsRunT> runs,
            SeqSizeT idx,
            SymbolT symbol
    ) const
    {
        SeqSizeT rank = get_rank_neq(index, runs, idx, symbol);
        if (rank) {
            SeqSizeT next_idx = select_fw_neq(index, runs, rank - SeqSizeT{1}, symbol).global_pos();
            return idx + SeqSizeT{1} - next_idx;
        }
        else {
            return idx + SeqSizeT{1};
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

    SplitBufResult split_buffer(Span<const SymbolsRunT> runs, SeqSizeT pos) const
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

    std::vector<SymbolsRunT> insert_to_buffer(
            const std::vector<SymbolsRunT>& target,
            Span<const SymbolsRunT> source,
            SeqSizeT at
    ) const
    {
        SplitBufResult res = split_buffer(target, at);

        append_all(res.left, source);
        append_all(res.left, to_const_span(res.right));

        return res.left;
    }

    std::vector<SymbolsRunT> remove_from_buffer(
            const std::vector<SymbolsRunT>& buffer,
            SeqSizeT start, SeqSizeT end
    ) const
    {
        SplitBufResult res1 = split_buffer(buffer, start);
        SplitBufResult res2 = split_buffer(res1.right, end - start);

        append_all(res1.left, to_const_span(res2.right));

        return res1.left;
    }


};


template <
    size_t AlphabetSize_,
    bool Use64BitSize
>
class PackedSSRLESequenceTestBase: public TestState, public SymbolsRunTools<AlphabetSize_, Use64BitSize> {

    using Base = TestState;
    using ToolsBase = SymbolsRunTools<AlphabetSize_, Use64BitSize>;

    using MyType = PackedSSRLESequenceTestBase<AlphabetSize_, Use64BitSize>;

protected:

    static constexpr size_t SIZE_INDEX_BLOCK = 128;
    static constexpr size_t Bps = BitsPerSymbolConstexpr(AlphabetSize_);
    static constexpr size_t AlphabetSize = AlphabetSize_;

    using Seq    = PkdSSRLESeqT<AlphabetSize, 256, Use64BitSize>;
    using SeqSO  = typename Seq::SparseObject;

    using SeqPtr = std::shared_ptr<PkdStructHolder<Seq>>;

    size_t size_{32768};

    using SymbolsRunT   = SSRLERun<Bps>;
    using RunTraits     = SSRLERunTraits<Bps>;
    using CodeUnitT     = typename RunTraits::CodeUnitT;
    using SeqSizeT      = typename Seq::SeqSizeT;
    using RunSizeT      = typename Seq::RunSizeT;
    using SymbolT       = typename Seq::SymbolT;

public:

    MMA_STATE_FILEDS(size_);

    using ToolsBase::push_back;

    PackedSSRLESequenceTestBase(): ToolsBase(this) {}

    SeqSO get_so(SeqPtr ptr) const {
        return ptr->get_so();
    }



    SeqPtr make_empty_sequence(size_t syms_block_size = 1024*1024) const
    {
        size_t block_size = Seq::compute_block_size(syms_block_size * 2);
        return PkdStructHolder<Seq>::make_empty(block_size);
    }



    SeqPtr make_sequence(Span<const SymbolsRunT> span, size_t capacity_multiplier = 1) const
    {
        size_t num_atoms = RunTraits::compute_size(span);

        SeqPtr ptr = make_empty_sequence(num_atoms * capacity_multiplier);
        SeqSO seq = get_so(ptr);

        auto update_state = seq.make_update_state();
        assert_success(seq.prepare_insert(0, update_state.first, span));
        seq.commit_insert(0, update_state.first, span);

        seq.check();

        return ptr;
    }

    template <typename T>
    void assertIndexCorrect(const char* src, const T& seq)
    {
        try {
            seq->check();
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
};


}}
