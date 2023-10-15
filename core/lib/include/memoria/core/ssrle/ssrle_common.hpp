
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/result.hpp>

#include <memoria/core/tools/arena_buffer.hpp>

#include <tuple>
#include <string>


namespace memoria {

template <size_t Bps> class SSRLERun;
template <size_t Bps> struct SSRLERunTraits;

template <size_t Bps, size_t Size = 5>
using SSRLERunArray = core::StaticArray<SSRLERun<Bps>, Size>;


template <size_t Bps>
struct SSRLERunCommonTraitsBase {
    static_assert(Bps <= 8, "Unsupported bits-per-symbol value");

    using CodeUnitT     = uint16_t;
    using RunDataT      = uint64_t;
    using SymbolT       = size_t;
    using RunSizeT      = uint64_t;

    static constexpr size_t CODE_WORD_BITS = sizeof(RunDataT) * 8; // Should be 64
    static constexpr size_t CODE_WORD_SIZE_BITS = 2;

    static constexpr size_t CODE_WORD_BITS_MIN = sizeof(CodeUnitT) * 8;
    static constexpr size_t CODE_WORD_BITS_MAX = CODE_WORD_BITS;

    static constexpr size_t CODE_UNITS_PER_WORD_MAX = CODE_WORD_BITS / CODE_WORD_BITS_MIN;

    static constexpr size_t SEGMENT_SIZE_BYTES = 64;
    static constexpr size_t SEGMENT_SIZE_UNITS = SEGMENT_SIZE_BYTES / sizeof(CodeUnitT);

    static constexpr size_t SYMBOLS = size_t{1} << Bps;

    using RunTraits = SSRLERunTraits<Bps>;

    using RunT = SSRLERun<Bps>;

    static constexpr RunDataT make_mask(size_t len) noexcept
    {
        if (len >= CODE_WORD_BITS) {
            return ~static_cast<RunDataT>(0);
        }
        else {
            RunDataT value = static_cast<RunDataT>(1) << len;
            return value - 1;
        }
    }


protected:
    static constexpr RunDataT make_mask_safe(size_t len) noexcept
    {
        RunDataT atom = static_cast<RunDataT>(1) << len;
        return atom - 1;
    }

    static constexpr size_t run_length_bitsize(size_t value) noexcept {
        return value <= 1 ? 0 : Log2U(value);
    }

public:
    static constexpr RunDataT SYMBOL_MASK = make_mask_safe(Bps);


    static RunT make_run(std::initializer_list<SymbolT> symbols, RunSizeT run_length) noexcept
    {
        size_t max_len = RunTraits::max_pattern_length();

        size_t cnt{};
        RunDataT pattern{};

        for (size_t sym: symbols) {
            if (cnt < max_len) {
                RunDataT mask = make_mask_safe(Bps);
                pattern |= (sym & mask) << (cnt * Bps);
            }
            cnt++;
        }

        return RunT(cnt, pattern, run_length);
    }

protected:
    static RunT sub_pattern(const RunT& run, RunSizeT start, RunSizeT size, RunSizeT len = 1) noexcept
    {
        RunT n_run;
        n_run.pattern_ = (run.pattern() >> start * Bps) & make_mask_safe(size * Bps);
        n_run.pattern_length_ = size;
        n_run.run_length_ = len;
        return n_run;
    }

public:
    static void extract_to(std::vector<RunT>& runs, const RunT& run, RunSizeT start, RunSizeT size)
    {
        RunSizeT full_len = run.full_run_length();
        if (start + size <= full_len)
        {
            if (run.run_length() == 1) {
                runs.push_back(sub_pattern(run, start, size));
            }
            else if (run.pattern_length() == 1) {
                RunT r_run = run;
                r_run.run_length_ = size;
                runs.push_back(r_run);
            }
            else {
                RunSizeT rr_start = start / run.pattern_length();
                RunSizeT rr_start_base = rr_start * run.pattern_length();
                RunSizeT rr_start_local_pos = start - rr_start_base;

                if (rr_start_local_pos + size <= run.pattern_length())
                {
                    runs.push_back(sub_pattern(run, rr_start_local_pos, size));
                }
                else {
                    if (rr_start_local_pos) {
                        runs.push_back(sub_pattern(run, rr_start_local_pos, run.pattern_length() - rr_start_local_pos));
                        rr_start++;
                    }

                    RunSizeT rr_end = (start + size) / run.pattern_length();
                    RunSizeT rr_end_base = rr_end * run.pattern_length();
                    RunSizeT rr_end_local_pos = (start + size) - rr_end_base;

                    if (rr_end > rr_start)
                    {
                        RunT m_run = run;
                        m_run.run_length_ = rr_end - rr_start;
                        runs.push_back(m_run);
                    }

                    if (rr_end_local_pos) {
                        runs.push_back(sub_pattern(run, 0, rr_end_local_pos));
                    }
                }
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid start/size in extract_to: {} :: {} :: {}", start, size, full_len).do_throw();
        }
    }


    struct SplitResult {
        SSRLERunArray<Bps, 2> left;
        SSRLERunArray<Bps, 2> right;
    };

    static SplitResult split(const RunT& run, size_t at)
    {
        if (at > run.full_run_length()) {
            MEMORIA_MAKE_GENERIC_ERROR("Split position {} is outside of length {} for SSRLE run {}",
                at,
                run.full_run_length(),
                run
            ).do_throw();
        }

        SplitResult split;

        if (run.pattern_length() == 0) {
            MEMORIA_MAKE_GENERIC_ERROR("SSLRERun is empty").do_throw();
        }
        else if (run.run_length() == 1)
        {
            if (at == 0) {
                split.right.append(run);
            }
            else if (at == run.pattern_length()) {
                split.left.append(run);
            }
            else
            {
                auto left  = run;
                auto right = run;

                size_t remainder = run.pattern_length() - at;

                left.pattern_  &= make_mask_safe(Bps * at);

                right.pattern_ >>= Bps * at;
                right.pattern_ &= make_mask_safe(Bps * remainder);

                left.pattern_length_  = at;
                right.pattern_length_ = remainder;

                split.left.append(left);
                split.right.append(right);
            }

            return split;
        }
        else if (run.run_length() > 1)
        {
            if (run.pattern_length() == 1)
            {
                auto left  = run;
                auto right = run;

                left.run_length_ = at;
                right.run_length_ = run.run_length() - at;

                split.left.append(left);
                split.right.append(right);

                return split;
            }
            else if (run.pattern_length() > 1)
            {
                size_t at_start = at % run.pattern_length();
                size_t at_base  = at - at_start;

                if (at_start == 0)
                {
                    auto left  = run;
                    auto right = run;

                    left.run_length_  = at_base / run.pattern_length();
                    right.run_length_ = run.run_length() - left.run_length_;

                    if (left) split.left.append(left);
                    if (right) split.right.append(right);

                    return split;
                }
                else {
                    RunT prefix;

                    if (at_base) {
                        prefix = run;
                        prefix.run_length_ = at_base / run.pattern_length();
                    }

                    RunT suffix;

                    size_t suffix_run_length = run.run_length() - prefix.run_length() - 1;
                    if (suffix_run_length) {
                        suffix = run;
                        suffix.run_length_ = suffix_run_length;
                    }

                    auto left  = run;
                    auto right = run;

                    left.run_length_  = 1;
                    right.run_length_ = 1;

                    size_t remainder = run.pattern_length() - at_start;

                    left.pattern_  &= make_mask_safe(Bps * at_start);

                    right.pattern_ >>= Bps * at_start;
                    right.pattern_ &= make_mask_safe(Bps * remainder);

                    left.pattern_length_  = at_start;
                    right.pattern_length_ = remainder;

                    if (prefix) split.left.append(prefix);
                    if (left)   split.left.append(left);
                    if (right)  split.right.append(right);
                    if (suffix) split.right.append(suffix);

                    return split;
                }
            }
        }

        MEMORIA_MAKE_GENERIC_ERROR("Can't split run {} at {}", run.to_string(), at).do_throw();
    }

    static SSRLERunArray<Bps> insert(const RunT& self, const RunT& run, RunSizeT at)
    {
        if (at > self.full_run_length()) {
            MEMORIA_MAKE_GENERIC_ERROR("Insert position {} is outside of length {} for SSRLE run {}",
                at,
                self.full_run_length(),
                run
            ).do_throw();
        }

        SSRLERunArray<Bps> result;

        // This is a special case to avoid doing '%' operation.
        if (self.pattern_length() == 1 && run.pattern_length() == 1 && self.pattern() == run.pattern())
        {
            auto r_run = self;
            r_run.run_length_ += run.run_length();

            result.append(r_run);

            return result;
        }
        else if (is_same_pattern(self, run) && at % self.pattern_length() == 0)
        {
            auto r_run = self;
            r_run.run_length_ += run.run_length();

            result.append(r_run);

            return result;
        }
        else if (at == self.full_run_length())
        {
            if (self.run_length() == 1 && run.run_length() == 1)
            {
                if (RunTraits::is_fit(self.pattern_length() + run.pattern_length(), 1))
                {
                    auto r_run = self;

                    r_run.pattern_ |= run.pattern_ << (r_run.pattern_length_ * Bps);
                    r_run.pattern_length_ += run.pattern_length();

                    result.append(r_run);
                }
                else {
                    result.append(self);
                    result.append(run);
                }
            }
            else {
                result.append(self);
                result.append(run);
            }

            return result;
        }
        else
        {
            // When we are here, then either/and:
            // 1. patterns have the same length, but different content
            // 2. patterns have different length
            // 3. same content but insertion point is in the middle of the pattern

            // What is special about this branch as we will need to do a pattern split.
            // Because all other cases have been handled above.

            RunSizeT at_start = at % self.pattern_length();
            RunSizeT at_base  = at - at_start;

            RunT prefix, suffix;

            // We have prefix
            if (at_base)
            {
                prefix = self;
                prefix.run_length_ = at_base / self.pattern_length();
                result.append(prefix);
            }

            if (at_start == 0) {
                // start or end of the pattern, patterns are different

                size_t full_len = self.full_run_length();

                if (run.run_length() == 1)
                {
                    if (RunTraits::is_fit(self.pattern_length() + run.pattern_length(), 1))
                    {
                        auto r_run = self;
                        r_run.run_length_ = 1;

                        if (at == full_len){
                            r_run.pattern_ |= run.pattern() << (r_run.pattern_length() * Bps);
                        }
                        else {
                            r_run.pattern_ <<= (run.pattern_length() * Bps);
                            r_run.pattern_ |= run.pattern();

                            suffix = self;
                            suffix.run_length_ = self.run_length() - prefix.run_length() - 1;
                        }

                        r_run.pattern_length_ += run.pattern_length();

                        result.append(r_run);
                    }
                    else if (at == full_len) {
                        result.append(run);
                    }
                    else {
                        result.append(run);

                        suffix = self;
                        suffix.run_length_ = self.run_length() - prefix.run_length();
                    }
                }
                else if (at == full_len) {
                    result.append(run);
                }
                else {
                    result.append(run);

                    suffix = self;
                    suffix.run_length_ = self.run_length() - prefix.run_length();
                }
            }
            else if (run.run_length() == 1 && RunTraits::is_fit(self.pattern_length() + run.pattern_length(), 1))
            {
                auto r_run = self;
                r_run.run_length_ = 1;

                RunDataT tmp = r_run.pattern_ >> (at_start * Bps);
                r_run.pattern_ &= make_mask_safe(at_start * Bps);
                r_run.pattern_ |= run.pattern() << (at_start * Bps);
                r_run.pattern_ |= tmp << ((at_start + run.pattern_length()) * Bps);

                r_run.pattern_length_ += run.pattern_length();

                result.append(r_run);

                RunSizeT suffix_length = self.run_length() - prefix.run_length() - 1;
                if (suffix_length)
                {
                    suffix = self;
                    suffix.run_length_ = suffix_length;
                }
            }
            else {
                auto left = self, right = self;
                left.run_length_ = 1;
                right.run_length_ = 1;

                left.pattern_ &= make_mask_safe(at_start * Bps);
                left.pattern_length_ = at_start;

                right.pattern_ >>= (at_start * Bps);
                right.pattern_length_ = self.pattern_length() - at_start;

                result.append(left);
                result.append(run);
                result.append(right);

                size_t suffix_length = self.run_length() - prefix.run_length() - 1;
                if (suffix_length) {
                    suffix = self;
                    suffix.run_length_ = suffix_length;
                }
            }

            if (suffix) {
                result.append(suffix);
            }

            return result;
        }
    }


    static bool merge(RunT& self, const RunT& run) noexcept
    {
        if (self.is_same_pattern(run))
        {
            if (RunTraits::is_fit(self.pattern_length_, self.run_length_ + run.run_length())) {
                self.run_length_ += run.run_length();
                return true;
            }
        }
        else if (self.run_length_ == 1 && run.run_length() == 1)
        {
            if (RunTraits::is_fit(self.pattern_length_ + run.pattern_length(), 1)) {
                RunDataT code_word = run.pattern() << (self.pattern_length_ * Bps);
                self.pattern_ |= code_word;
                self.pattern_length_ += run.pattern_length();
                return true;
            }
        }

        return false;
    }


    static void encode_run(const RunT& run, CodeUnitT* units, size_t unit_size) noexcept
    {
        RunDataT code_word{};
        SSRLERunTraits<Bps>::encode_run(run, code_word, unit_size);
        for (size_t c = 0; c < unit_size; c++)
        {
            CodeUnitT unit = static_cast<CodeUnitT>(code_word);
            units[c] = unit;
            code_word >>= sizeof(CodeUnitT) * 8;
        }
    }

    static void encode_run(const RunT& run, RunDataT& code_word, size_t unit_size) noexcept
    {
        constexpr size_t PATTERN_LEN = RunTraits::LEN_BITS;

        code_word = unit_size - 1;
        code_word |= run.pattern_length() << CODE_WORD_SIZE_BITS;
        code_word |= (run.pattern() & make_mask_safe(run.pattern_length() * Bps)) << (CODE_WORD_SIZE_BITS + PATTERN_LEN);

        code_word |= run.run_length() << (CODE_WORD_SIZE_BITS + PATTERN_LEN + run.pattern_length() * Bps);
    }

    static size_t decode_unit_to(const CodeUnitT* units, RunT& run) noexcept
    {
        CodeUnitT unit = units[0];
        size_t unit_size = (unit & make_mask_safe(CODE_WORD_SIZE_BITS)) + 1;

        RunDataT code_word{};
        for (size_t c = 0; c < unit_size; c++) {
            code_word |= static_cast<RunDataT>(units[c]) << (c * sizeof(CodeUnitT) * 8);
        }

        return decode_unit_to(code_word, run, unit_size);
    }


    static size_t decode_unit_to(RunDataT code_word, RunT& run, size_t unit_size) noexcept
    {
        constexpr size_t PATTERN_LEN = RunTraits::LEN_BITS;

        code_word >>= CODE_WORD_SIZE_BITS;

        run.pattern_length_ = code_word & make_mask_safe(PATTERN_LEN);

        code_word >>= PATTERN_LEN;
        run.pattern_ = code_word & make_mask_safe(run.pattern_length_ * Bps);
        code_word >>= (run.pattern_length_ * Bps);

        size_t run_len_bits = unit_size * CODE_WORD_BITS_MIN - PATTERN_LEN - CODE_WORD_SIZE_BITS - run.pattern_length() * Bps;

        if (run_len_bits) {
            run.run_length_ = code_word & make_mask_safe(run_len_bits);
        }
        else {
            run.run_length_ = 1;
        }

        return unit_size;
    }

    static size_t decode_segment_to(RunDataT& code_word, Span<RunT> span) noexcept
    {
        size_t units = 0;

        for (size_t c = 0; c < CODE_UNITS_PER_WORD_MAX;)
        {
            size_t unit_size = RunTraits::decode_unit_to(code_word, span[units]);
            c += unit_size;

            code_word >>= (unit_size * CODE_WORD_BITS_MIN);
        }

        for (size_t u = units; u < span.size(); u++) {
            span[u] = RunT{};
        }

        return units;
    }

    static void write_segments_to(
            Span<const RunT> source,
            Span<CodeUnitT> target,
            size_t& tgt_idx,
            size_t next_limit
    ) noexcept
    {
        for (size_t c = 0; c < source.size(); c++)
        {
            if (source[c])
            {
                size_t units = RunTraits::estimate_size(source[c]);
                if (MMA_UNLIKELY(tgt_idx + units > next_limit))
                {
                    size_t remainder = next_limit - tgt_idx;
                    // FIXME: Padding value is not necessary a single unit (for up to 255 units).
                    // Longer paddings (for long segments) will require either multi-unit paddings,
                    // or splitting paddings into mutiple paddings runs.

                    // Code below assumes that CODE_UNITS_PER_SEGMENT_MAX is less than 256
                    static_assert(CODE_UNITS_PER_WORD_MAX < 256);

                    RunT padding = RunT::make_padding(remainder);
                    RunTraits::encode_run(
                                padding,
                                target.data() + tgt_idx,
                                1
                                );

                    tgt_idx += 1;

                    RunT null_run = RunT::make_null();

                    for (size_t d = tgt_idx; d < next_limit; d++) {
                        RunTraits::encode_run(
                                    null_run,
                                    target.data() + tgt_idx,
                                    1
                                    );

                        tgt_idx += 1;
                    }

                    next_limit += SEGMENT_SIZE_UNITS;
                }
                else if (MMA_UNLIKELY(tgt_idx + units == next_limit)) {
                    next_limit += SEGMENT_SIZE_UNITS;
                }

                RunTraits::encode_run(source[c], target.data() + tgt_idx, units);
                tgt_idx += units;
            }
        }
    }

    static size_t write_segments_to(Span<const RunT> prefix, Span<const RunT> source, Span<CodeUnitT> target) noexcept
    {
        size_t tgt_idx{};
        size_t next_limit = SEGMENT_SIZE_UNITS;

        write_segments_to(prefix, target, tgt_idx, next_limit);
        write_segments_to(source, target, tgt_idx, next_limit);
        finish_setgment(target, tgt_idx);

        return tgt_idx;
    }

    static size_t write_segments_to(Span<const RunT> source, Span<CodeUnitT> target, size_t start) noexcept
    {
        size_t tgt_idx = start;

        size_t segment_offset = start % SEGMENT_SIZE_UNITS;
        size_t next_limit = start - segment_offset + SEGMENT_SIZE_UNITS;

        write_segments_to(source, target, tgt_idx, next_limit);
        finish_setgment(target, tgt_idx);

        return tgt_idx;
    }


    static void compute_size(Span<const RunT> source, size_t& tgt_idx, size_t next_limit) noexcept
    {
        for (size_t c = 0; c < source.size();)
        {
            if (source[c])
            {
                size_t units = RunTraits::estimate_size(source[c]);
                if (MMA_UNLIKELY(tgt_idx + units > next_limit))
                {
                    tgt_idx = next_limit;
                    next_limit += SEGMENT_SIZE_UNITS;
                }
                else {
                    tgt_idx += units;
                    c++;
                }
            }
            else {
                c++;
            }
        }
    }

    static size_t compute_size(Span<const RunT> source, size_t start = 0) noexcept
    {
        size_t tgt_idx = start;

        size_t segment_offset = start % SEGMENT_SIZE_UNITS;
        size_t next_limit = start - segment_offset + SEGMENT_SIZE_UNITS;

        compute_size(source, tgt_idx, next_limit);

        return tgt_idx;
    }

    static size_t compute_size(Span<const RunT> prefix, Span<const RunT> source) noexcept
    {
        size_t tgt_idx = 0;
        size_t next_limit = SEGMENT_SIZE_UNITS;

        compute_size(prefix, tgt_idx, next_limit);
        compute_size(source, tgt_idx, next_limit);

        return tgt_idx;
    }

    static void finish_setgment(Span<CodeUnitT> span, size_t start) noexcept {
        for (size_t c = start; c < span.size(); c++) {
            span[c] = 0;
        }
    }

    static bool is_same_pattern(const RunT& self, const RunT& other) noexcept {
        return self.pattern_length_ == other.pattern_length_ &&
               self.pattern_ == other.pattern_;
    }

    static void compactify_runs(std::vector<RunT>& runs) noexcept {
        compactify_runs(Span<RunT>(runs.data(), runs.size()));
    }

    static void compactify_runs(Span<RunT> runs) noexcept
    {
        if (runs.size() > 1) {
            for (size_t c = 0; c < runs.size() - 1;)
            {
                size_t d;
                for (d = c + 1; d < runs.size(); d++)
                {
                    if (runs[c].merge(runs[d])) {
                        runs[d] = RunT{};
                    }
                    else {
                        break;
                    }
                }
                c = d;
            }
        }
    }

    template <typename T>
    static void pattern_ranks(const RunT& run, Span<T> sync) noexcept
    {
        pattern_ranks(run, run.pattern_length(), sync);
    }


    template <typename T>
    static void pattern_ranks(const RunT& run, size_t idx, Span<T> sink) noexcept
    {
        for (size_t c = 0; c < idx; c++) {
            SymbolT sym = run.pattern_symbol(c);
            ++sink[sym];
        }
    }

    static RunSizeT pattern_rank(const RunT& run, SymbolT symbol) noexcept
    {
        RunSizeT rank{};

        for (size_t c = 0; c < run.pattern_length(); c++) {
            rank += run.pattern_symbol(c) == symbol;
        }

        return rank;
    }

    static RunSizeT pattern_rank(const RunT& run, RunSizeT idx, SymbolT symbol) noexcept
    {
        RunSizeT rank{};

        for (size_t c = 0; c < idx; c++) {
            rank += run.pattern_symbol(c) == symbol;
        }

        return rank;
    }



    static size_t rank_eq(const RunT& run, RunSizeT idx, SymbolT symbol) noexcept
    {
        if (run.run_length() == 1) {
            RunSizeT pattern_rank = RunTraits::pattern_rank(run, idx, symbol);
            return pattern_rank;
        }
        else {
            RunSizeT full_rank = pattern_rank(run, symbol);

            RunSizeT run_prefix = idx / run.pattern_length();
            RunSizeT idx_prefix = run_prefix * run.pattern_length();
            RunSizeT idx_local  = idx - idx_prefix;

            RunSizeT pattern_rank = RunTraits::pattern_rank(run, idx_local, symbol);

            RunSizeT rank = run_prefix * full_rank;

            rank += pattern_rank;

            return rank;
        }
    }

    template <typename Fn>
    static RunSizeT rank_fn(const RunT& run, RunSizeT idx, SymbolT symbol, Fn&& fn) noexcept
    {
        if (run.run_length() == 1) {
            RunSizeT pattern_rank = RunTraits::pattern_rank_fn(run, idx, symbol, std::forward<Fn>(fn));
            return pattern_rank;
        }
        else {
            RunSizeT full_rank = RunTraits::pattern_rank_fn(run, symbol, std::forward<Fn>(fn));

            RunSizeT run_prefix = idx / run.pattern_length();
            RunSizeT idx_prefix = run_prefix * run.pattern_length();
            RunSizeT idx_local  = idx - idx_prefix;

            RunSizeT pattern_rank = RunTraits::pattern_rank_fn(run, idx_local, symbol, std::forward<Fn>(fn));

            RunSizeT rank = run_prefix * full_rank;

            rank += pattern_rank;

            return rank;
        }
    }



    template <typename T>
    static void full_ranks(const RunT& run, Span<T> sink) noexcept
    {
        if (run.run_length() > 1)
        {
            RunSizeT tmp[SYMBOLS]{0,};
            RunTraits::pattern_ranks(run, Span<RunSizeT>(tmp, SYMBOLS));

            for (size_t s = 0; s < sink.size(); s++)
            {
                sink[s] += tmp[s] * run.run_length();
            }
        }
        else {
            RunTraits::pattern_ranks(run, sink);
        }
    }

    template <typename T>
    static void ranks(const RunT& run, RunSizeT idx, Span<T> sink) noexcept
    {
        if (run.run_length() == 1) {
            RunTraits::pattern_ranks(run, idx, sink);
        }
        else {
            RunSizeT run_prefix = idx / run.pattern_length();
            RunSizeT idx_prefix = run_prefix * run.pattern_length();
            RunSizeT idx_local  = idx - idx_prefix;

            if (run_prefix)
            {
                RunSizeT tmp[SYMBOLS]{0,};
                RunTraits::pattern_ranks(run, Span<RunSizeT>(tmp, SYMBOLS));

                for (size_t s = 0; s < sink.size(); s++)
                {
                    sink[s] += tmp[s] * run_prefix;
                }
            }

            RunTraits::pattern_ranks(run, idx_local, sink);
        }
    }

    template <typename Fn>
    static size_t pattern_rank_fn(const RunT& run, SymbolT symbol, Fn&& fn) noexcept {
        RunSizeT rank{};
        for (size_t c = 0; c < run.pattern_length(); c++) {
            SymbolT sym = run.pattern_symbol(c);
            rank += fn(sym, symbol);
        }
        return rank;
    }


    template <typename Fn>
    static size_t pattern_rank_fn(const RunT& run, RunSizeT idx, SymbolT symbol, Fn&& fn) noexcept {
        RunSizeT rank{};
        for (size_t c = 0; c < idx; c++) {
            SymbolT sym = run.pattern_symbol(c);
            rank += fn(sym, symbol);
        }
        return rank;
    }


    static RunSizeT pattern_select_fw(const RunT& run, RunSizeT rank, SymbolT symbol) noexcept {
        return pattern_select_fw_fn(run, rank, symbol, [](size_t run_sym, size_t sym){return run_sym == sym;});
    }


    template <typename Fn>
    static RunSizeT pattern_select_fw_fn(const RunT& run, RunSizeT rank, SymbolT symbol, Fn&& compare) noexcept
    {
        RunSizeT rr{};
        ++rank;
        for (size_t c = 0; c < run.pattern_length(); c++)
        {
            SymbolT sym = run.pattern_symbol(c);
            rr += compare(sym, symbol);

            if (rr == rank) {
                return c;
            }
        }

        return run.pattern_length();
    }



    static RunSizeT select_fw_eq(const RunT& run, RunSizeT rank, SymbolT symbol) noexcept
    {
        if (run.run_length() == 1) {
            return RunTraits::pattern_select_fw(run, rank, symbol);
        }
        else {
            RunSizeT rr = RunTraits::pattern_rank(run, symbol);

            RunSizeT rank_base = rank / rr;
            RunSizeT rank_local = rank - rank_base * rr;

            RunSizeT idx = RunTraits::pattern_select_fw(run, rank_local, symbol);

            return idx + run.pattern_length() * rank_base;
        }
    }

private:
    struct FindGTFn {
        bool operator()(SymbolT run_sym, SymbolT sym) const noexcept {
            return run_sym > sym;
        }
    };

    struct FindGEFn {
        bool operator()(SymbolT run_sym, SymbolT sym) const noexcept {
            return run_sym >= sym;
        }
    };

    struct FindLTFn {
        bool operator()(SymbolT run_sym, SymbolT sym) const noexcept {
            return run_sym < sym;
        }
    };

    struct FindLEFn {
        bool operator()(SymbolT run_sym, SymbolT sym) const noexcept {
            return run_sym <= sym;
        }
    };

    struct FindEQFn {
        bool operator()(SymbolT run_sym, SymbolT sym) const noexcept {
            return run_sym == sym;
        }
    };

    struct FindNEQFn {
        bool operator()(SymbolT run_sym, SymbolT sym) const noexcept {
            return run_sym != sym;
        }
    };

public:
    static RunSizeT select_fw_gt(const RunT& run, RunSizeT rank, SymbolT symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindGTFn());
    }

    static RunSizeT select_fw_lt(const RunT& run, RunSizeT rank, SymbolT symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindLTFn());
    }

    static RunSizeT select_fw_ge(const RunT& run, RunSizeT rank, SymbolT symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindGEFn());
    }

    static RunSizeT select_fw_le(const RunT& run, RunSizeT rank, SymbolT symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindLEFn());
    }

    static RunSizeT select_fw_neq(const RunT& run, RunSizeT rank, SymbolT symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindNEQFn());
    }


    template <typename Fn>
    static RunSizeT select_fw_fn(const RunT& run, RunSizeT rank, SymbolT symbol, Fn&& compare) noexcept
    {
        if (run.run_length() == 1) {
            return RunTraits::pattern_select_fw_fn(run, rank, symbol, std::forward<Fn>(compare));
        }
        else {
            size_t rr = RunTraits::pattern_rank_fn(run, symbol, std::forward<Fn>(compare));
            size_t rank_base = rank / rr;
            size_t rank_local = rank - rank_base * rr;

            size_t idx = RunTraits::pattern_select_fw_fn(run, rank_local, symbol, std::forward<Fn>(compare));
            return idx + run.pattern_length() * rank_base;
        }
    }


    static RunSizeT full_rank_eq(const RunT& run, SymbolT symbol) noexcept {
        return RunTraits::pattern_rank(run, symbol) * run.run_length();
    }

    static RunSizeT full_rank_gt(const RunT& run, SymbolT symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindGTFn()) * run.run_length();
    }

    static RunSizeT full_rank_ge(const RunT& run, SymbolT symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindGEFn()) * run.run_length();
    }

    static RunSizeT full_rank_lt(const RunT& run, SymbolT symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindLTFn()) * run.run_length();
    }

    static RunSizeT full_rank_le(const RunT& run, SymbolT symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindLEFn()) * run.run_length();
    }

    static RunSizeT full_rank_neq(const RunT& run, SymbolT symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindNEQFn()) * run.run_length();
    }

    static RunSizeT rank_gt(const RunT& run, RunSizeT idx, SymbolT symbol) noexcept {
        return rank_fn(run, idx, symbol, FindGTFn());
    }

    static RunSizeT rank_ge(const RunT& run, RunSizeT idx, SymbolT symbol) noexcept {
        return rank_fn(run, idx, symbol, FindGEFn());
    }

    static RunSizeT rank_lt(const RunT& run, RunSizeT idx, SymbolT symbol) noexcept {
        return rank_fn(run, idx, symbol, FindLTFn());
    }

    static RunSizeT rank_le(const RunT& run, RunSizeT idx, SymbolT symbol) noexcept {
        return rank_fn(run, idx, symbol, FindLEFn());
    }

    static RunSizeT rank_neq(const RunT& run, RunSizeT idx, SymbolT symbol) noexcept {
        return rank_fn(run, idx, symbol, FindNEQFn());
    }
};

template <size_t Bps>
struct SSRLERunCommonTraits: SSRLERunCommonTraitsBase<Bps> {};

template <size_t Bps> struct SSRLERunTraits;

template <size_t Bps>
class SSRLERun {
    static_assert(Bps <= 8, "Unsupported alphabet size");

    using Traits = SSRLERunTraits<Bps>;

public:
    using RunDataT = typename Traits::RunDataT;
    using RunSizeT = typename Traits::RunSizeT;
    using SymbolT = typename Traits::SymbolT;
private:

    RunDataT pattern_;
    RunDataT pattern_length_;
    RunSizeT run_length_;

    static constexpr RunDataT SYMBOL_MASK          = Traits::SYMBOL_MASK;
    static constexpr RunDataT MAX_PATTERN_LENGTH   = Traits::MAX_PATTERN_LENGTH;

    static constexpr RunDataT MAX_RUN_LENGTH       = Traits::MAX_RUN_LENGTH;

    friend struct SSRLERunTraits<Bps>;
    friend struct SSRLERunCommonTraitsBase<Bps>;
    friend struct SSRLERunCommonTraits<Bps>;

public:
    constexpr SSRLERun() noexcept : pattern_(), pattern_length_(), run_length_() {}
    constexpr SSRLERun(
        RunDataT pattern_length,
        RunDataT pattern,
        RunSizeT run_length
    ) noexcept :
        pattern_(pattern & SSRLERunTraits<Bps>::make_mask(pattern_length * Bps)),
        pattern_length_(pattern_length),
        run_length_(run_length)
    {}


    static constexpr SSRLERun<Bps> make_null() noexcept {
        return SSRLERun<Bps>();
    }

    static constexpr SSRLERun<Bps> make_padding(size_t length) noexcept {
        return SSRLERun<Bps>(0, 0, length);
    }

    bool operator==(const SSRLERun& other) const noexcept
    {
        return pattern_length_ == other.pattern_length_ &&
               pattern_ == other.pattern_ &&
               run_length_ == other.run_length_;
    }


    bool is_same_pattern(const SSRLERun& other) const noexcept {
        return SSRLERunTraits<Bps>::is_same_pattern(*this, other);
    }


    operator bool() const noexcept {
        return (run_length_ != 0) && (pattern_length_ != 0);
    }

    bool is_null() const noexcept {
        return (run_length_ == 0) && (pattern_length_ == 0);
    }

    bool is_padding() const noexcept {
        return (pattern_length_ == 0) && (run_length_ > 0);
    }

    constexpr RunDataT run_length() const noexcept {
        return run_length_;
    }

    constexpr RunSizeT full_run_length() const noexcept {
        return run_length_ * pattern_length_;
    }

    constexpr RunDataT pattern_length() const noexcept {
        return pattern_length_;
    }

    constexpr RunDataT pattern() const noexcept {
        return pattern_;
    }

    constexpr void set_pattern(RunDataT pattern) noexcept {
        pattern_ = pattern;
    }

    constexpr SymbolT symbol(RunSizeT idx) const noexcept {
        size_t p_idx{};
        if (run_length_ == 1) {
            p_idx = idx;
        }
        else {
            p_idx = idx % pattern_length_;
        }
        return (pattern_ >> (p_idx * Bps)) & SYMBOL_MASK;
    }

    constexpr SymbolT pattern_symbol(RunSizeT idx) const noexcept {
        return (pattern_ >> (idx * Bps)) & SYMBOL_MASK;
    }

    constexpr void set_symbol(RunSizeT idx, SymbolT symbol) noexcept
    {
        RunDataT mask = ((static_cast<RunDataT>(1) << Bps) - 1);

        size_t p_idx{};
        if (run_length_ == 1) {
            p_idx = idx;
        }
        else {
            p_idx = idx % pattern_length_;
        }

        symbol &= mask;
        mask <<= (p_idx * Bps);

        pattern_ = pattern_ & ~mask ;
        pattern_ = pattern_ | (symbol << (p_idx * Bps));
    }

    constexpr void set_pattern_symbol(RunSizeT idx, SymbolT symbol) noexcept
    {
        RunDataT mask = ((static_cast<RunDataT>(1) << Bps) - 1);

        symbol &= mask;
        mask <<= (idx * Bps);

        pattern_ = pattern_ & ~mask ;
        pattern_ = pattern_ | (symbol << (idx * Bps));
    }


    typename SSRLERunTraits<Bps>::SplitResult split(RunSizeT at) const
    {
        return Traits::split(*this, at);
    }

    SSRLERunArray<Bps> remove(RunSizeT from, RunSizeT to) const
    {
        return Traits::remove(*this, from, to);
    }

    SSRLERunArray<Bps> insert(const SSRLERun& run, RunSizeT at) const
    {
        return Traits::insert(*this, run, at);
    }


    bool merge(const SSRLERun& run) noexcept
    {
        return Traits::merge(*this, run);
    }

    std::string to_string() const
    {
        std::stringstream ss;

        auto pattern = Traits::pattern_to_string(*this);

        ss << "{" << pattern_length_ << ", '" << pattern << "', " << run_length_ << "}";

        return ss.str();
    }

    RunSizeT full_rank_eq(SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_eq(*this, symbol);
    }

    RunSizeT full_rank_lt(SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_lt(*this, symbol);
    }

    RunSizeT full_rank_le(SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_le(*this, symbol);
    }

    RunSizeT full_rank_gt(SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_gt(*this, symbol);
    }

    RunSizeT full_rank_ge(SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_ge(*this, symbol);
    }

    RunSizeT full_rank_neq(SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_neq(*this, symbol);
    }

    RunSizeT rank_eq(RunSizeT idx, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_eq(*this, idx, symbol);
    }

    RunSizeT rank_lt(RunSizeT idx, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_lt(*this, idx, symbol);
    }

    RunSizeT rank_le(RunSizeT idx, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_le(*this, idx, symbol);
    }

    RunSizeT rank_gt(RunSizeT idx, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_gt(*this, idx, symbol);
    }

    RunSizeT rank_ge(RunSizeT idx, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_ge(*this, idx, symbol);
    }

    RunSizeT rank_neq(RunSizeT idx, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_neq(*this, idx, symbol);
    }

    template <typename T>
    void ranks(RunSizeT idx, Span<T> sink) const noexcept {
        return SSRLERunCommonTraits<Bps>::ranks(*this, idx, sink);
    }

    template <typename T>
    void full_ranks(Span<T> sink) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_ranks(*this, sink);
    }

    RunSizeT select_fw_eq(RunSizeT rank, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_eq(*this, rank, symbol);
    }

    RunSizeT select_fw_lt(RunSizeT rank, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_lt(*this, rank, symbol);
    }

    RunSizeT select_fw_le(RunSizeT rank, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_le(*this, rank, symbol);
    }

    RunSizeT select_fw_gt(RunSizeT rank, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_gt(*this, rank, symbol);
    }

    RunSizeT select_fw_ge(RunSizeT rank, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_ge(*this, rank, symbol);
    }

    RunSizeT select_fw_neq(RunSizeT rank, SymbolT symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_neq(*this, rank, symbol);
    }

    static SSRLERun make_run(std::initializer_list<SymbolT> symbols, RunSizeT run_length) noexcept {
        return SSRLERunTraits<Bps>::make_run(symbols, run_length);
    }

    RunSizeT to_pattern(RunSizeT global_idx) const {
        return global_idx % pattern_length_;
    }

    size_t size_in_units() const noexcept {
        return SSRLERunTraits<Bps>::estimate_size(*this);
    }

    void extract_to(std::vector<SSRLERun>& runs, const SSRLERun& run, RunSizeT start, RunSizeT size) const
    {
        SSRLERunTraits<Bps>::extract_to(runs, run, start, size);
    }
};


template <size_t BPS>
typename SSRLERun<BPS>::RunSizeT count_symbols_in(Span<const SSRLERun<BPS>> runs)
{
    using RunSizeT = typename SSRLERun<BPS>::RunSizeT;
    RunSizeT size{};
    for (const auto& run: runs) {
        size += run.full_run_length();
    }
    return size;

}

}

namespace fmt {

template <size_t Bps>
struct formatter<memoria::SSRLERun<Bps>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::SSRLERun<Bps>& run, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", run.to_string());
    }
};


}
