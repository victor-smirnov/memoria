
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

    using AtomT     = uint16_t;
    using CodeUnitT = uint64_t;

    // A group of up to 4 atoms.
    using SegmentT  = uint64_t;

    static constexpr size_t SEGMENT_BITS = sizeof(SegmentT) * 8; // Should be 64
    static constexpr size_t CODE_UNIT_SIZE_BITS = 2;

    static constexpr size_t CODE_UNIT_BITS_MIN = sizeof(AtomT) * 8;
    static constexpr size_t CODE_UNIT_BITS_MAX = SEGMENT_BITS;

    static constexpr size_t CODE_UNITS_PER_SEGMENT_MAX = SEGMENT_BITS / CODE_UNIT_BITS_MIN;

    static constexpr size_t SEGMENT_SIZE_BYTES = 64;
    static constexpr size_t SEGMENT_SIZE_UNITS = SEGMENT_SIZE_BYTES / sizeof(AtomT);

    static constexpr size_t SYMBOLS = size_t(1) << Bps;

    using RunTraits = SSRLERunTraits<Bps>;

    using RunT = SSRLERun<Bps>;

    static constexpr CodeUnitT make_mask(size_t len)
    {
        if (len >= SEGMENT_BITS) {
            return ~static_cast<CodeUnitT>(0);
        }
        else {
            CodeUnitT atom = static_cast<CodeUnitT>(1) << len;
            return atom - 1;
        }
    }


protected:
    static constexpr CodeUnitT make_mask_safe(size_t len)
    {
        CodeUnitT atom = static_cast<CodeUnitT>(1) << len;
        return atom - 1;
    }

    static constexpr size_t run_length_bitsize(size_t value) {
        return value <= 1 ? 0 : Log2U(value);
    }

public:
    static constexpr CodeUnitT SYMBOL_MASK = make_mask_safe(Bps);


    static RunT make_run(std::initializer_list<size_t> symbols, size_t run_length) noexcept
    {
        size_t max_len = RunTraits::max_pattern_length();

        size_t cnt{};
        size_t pattern{};

        for (size_t sym: symbols) {
            if (cnt < max_len) {
                size_t mask = make_mask_safe(Bps);
                pattern |= (sym & mask) << (cnt * Bps);
            }
            cnt++;
        }

        return RunT(cnt, pattern, run_length);
    }


    struct SplitResult {
        SSRLERunArray<Bps, 2> left;
        SSRLERunArray<Bps, 2> right;
    };

    static Optional<SplitResult> split(const RunT& run, size_t at) noexcept
    {
        using OptionalT = Optional<SplitResult>;

        SplitResult split;

        if (run.pattern_length() == 0) {
            return OptionalT{};
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

        return OptionalT{};
    }

    static Optional<SSRLERunArray<Bps>> remove(const RunT& run, size_t from, size_t to) noexcept
    {
        using OptionalT = Optional<SSRLERunArray<Bps>>;

        SSRLERunArray<Bps> result;

        if (MMA_UNLIKELY(run.run_length() == 1))
        {
            // This is an uncompressed segment
            if (MMA_LIKELY(from <= to && to <= run.pattern_length()))
            {
                auto r_run = run;

                if (MMA_LIKELY(from < to))
                {
                    r_run.pattern_ &= make_mask_safe(from * Bps);
                    r_run.pattern_ |= run.pattern() >> ((to - from) * Bps);

                    r_run.pattern_length_ -= (to - from);
                }

                result.append(r_run);

                return result;
            }
        }
        else if (run.pattern_length() == 1)
        {
            // This is classical RLE segment

            if (MMA_LIKELY(from <= to && to <= run.run_length()))
            {
                auto r_run = run;
                r_run.run_length_ -= to - from;

                result.append(r_run);

                return result;
            }
        }
        else if (MMA_LIKELY(run.run_length() > 1))
        {
            // Repeating pattern segment

            size_t len = to - from;
            size_t from_start = from % run.pattern_length();
            size_t from_base  = from - from_start;

            if (from_start == 0 && (len % run.pattern_length() == 0))
            {
                // Removing entire patterns

                auto r_run = run;
                r_run.run_length_ -= (len / run.pattern_length());

                result.append(r_run);

                return result;
            }
            else if (from_start + len <= run.pattern_length())
            {
                // Removing bits from a single pattern

                auto prefix = run;
                auto r_run  = run;
                auto suffix = run;

                r_run.run_length_ = 1;
                r_run.pattern_ &= make_mask_safe(from_start * Bps);
                r_run.pattern_ |= (run.pattern() >> ((to - from_base) * Bps)) << (from_start * Bps);
                r_run.pattern_length_ -= len;

                prefix.run_length_ = (from_base / run.pattern_length());
                suffix.run_length_ = run.run_length() - prefix.run_length() - 1;

                if (prefix.run_length()) result.append(prefix);
                if (r_run)  result.append(r_run);
                if (suffix.run_length()) result.append(suffix);

                return result;
            }
            else {
                // Removing bits across patterns

                size_t r_len   = len / run.pattern_length();
                size_t to_start = to % run.pattern_length();

                size_t splits = 0;

                RunT prefix, r_run, extra;

                if (MMA_UNLIKELY(from_base))
                {
                    prefix = run;
                    prefix.run_length_ = from_base / run.pattern_length();
                    result.append(prefix);
                }

                if (from_start == 0 && to_start != 0)
                {
                    r_run = run;
                    r_run.pattern_ = run.pattern() >> (to_start * Bps);
                    r_run.pattern_length_ -= to_start;
                    r_run.run_length_ = 1;
                    splits = 1;
                }
                else if (to_start == 0)
                {
                    r_run = run;
                    r_run.pattern_ &= make_mask_safe(from_start * Bps);
                    r_run.pattern_length_ = from_start;
                    r_run.run_length_ = 1;

                    splits = 1;
                }
                else if ((run.pattern_length() - to_start) + from_start <= SSRLERunTraits<Bps>::max_pattern_length())
                {
                    r_run = run;
                    r_run.pattern_ &= make_mask_safe(from_start * Bps);
                    r_run.pattern_ |= (run.pattern() >> (to_start * Bps)) << (from_start * Bps);

                    r_run.pattern_length_ = from_start + run.pattern_length() - to_start;
                    r_run.run_length_ = 1;

                    splits = 2;
                }
                else  {
                    r_run = run;
                    r_run.pattern_ &= make_mask_safe(from_start * Bps);
                    r_run.pattern_length_ = from_start;
                    r_run.run_length_ = 1;

                    extra = run;
                    extra.pattern_ = run.pattern() >> (to_start * Bps);
                    extra.pattern_length_ -= to_start;
                    extra.run_length_ = 1;

                    splits = 2;
                }

                if (r_run) {
                    result.append(r_run);
                }

                if (extra) {
                    result.append(extra);
                }

                CodeUnitT suffix_len = run.run_length() - prefix.run_length() - r_len - splits;
                if (suffix_len)
                {
                    auto suffix = run;
                    suffix.run_length_ = suffix_len;
                    result.append(suffix);
                }

                return result;
            }
        }

        return OptionalT{};
    }

    static Optional<SSRLERunArray<Bps>> insert(const RunT& self, const RunT& run, size_t at) noexcept
    {
        using OptionalT = Optional<SSRLERunArray<Bps>>;

        SSRLERunArray<Bps> result;

        // This is a special case to avoid doing '%' operation.
        if (self.pattern_length() == 1 && run.pattern_length() == 1 && self.pattern() == run.pattern())
        {
            auto r_run = self;
            r_run.run_length_ += run.run_length();

            result.append(r_run);

            return result;
        }
        else if (self.pattern_length() == run.pattern_length() && self.pattern() == run.pattern() && at % self.pattern_length() == 0)
        {
            auto r_run = self;
            r_run.run_length_ += run.run_length();

            result.append(r_run);

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

            size_t at_start = at % self.pattern_length();
            size_t at_base  = at - at_start;

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

                CodeUnitT tmp = r_run.pattern_ >> (at_start * Bps);
                r_run.pattern_ &= make_mask_safe(at_start * Bps);
                r_run.pattern_ |= run.pattern() << (at_start * Bps);
                r_run.pattern_ |= tmp << ((at_start + run.pattern_length()) * Bps);

                r_run.pattern_length_ += run.pattern_length();

                result.append(r_run);

                size_t suffix_length = self.run_length() - prefix.run_length() - 1;
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

        return OptionalT{};
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
                CodeUnitT atom = run.pattern() << self.pattern_length_;
                self.pattern_ |= atom;
                self.pattern_length_ += run.pattern_length();
                return true;
            }
        }

        return false;
    }


    static void encode_run(const RunT& run, AtomT* atoms, size_t unit_size) noexcept
    {
        SegmentT segment{};
        SSRLERunTraits<Bps>::encode_run(run, segment, unit_size);
        for (size_t c = 0; c < unit_size; c++)
        {
            AtomT atom = static_cast<AtomT>(segment);
            atoms[c] = atom;
            segment >>= sizeof(AtomT) * 8;
        }
    }

    static void encode_run(const RunT& run, SegmentT& segment, size_t unit_size) noexcept
    {
        constexpr size_t PATTERN_LEN = RunTraits::LEN_BITS;

        if (run.pattern_length() == 56) {
            int a = 0;
            a++;
        }

        segment = unit_size - 1;
        segment |= run.pattern_length() << CODE_UNIT_SIZE_BITS;
        segment |= run.pattern() << (CODE_UNIT_SIZE_BITS + PATTERN_LEN);

        segment |= run.run_length() << (CODE_UNIT_SIZE_BITS + PATTERN_LEN + run.pattern_length() * Bps);
    }

    static size_t decode_unit_to(const AtomT* atoms, RunT& run) noexcept
    {
        AtomT atom = atoms[0];
        size_t unit_size = (atom & make_mask_safe(CODE_UNIT_SIZE_BITS)) + 1;

        SegmentT segment{};
        for (size_t c = 0; c < unit_size; c++) {
            segment |= static_cast<SegmentT>(atoms[c]) << (c * sizeof(AtomT) * 8);
        }

        return decode_unit_to(segment, run, unit_size);
    }


    static size_t decode_unit_to(SegmentT segment, RunT& run, size_t unit_size) noexcept
    {
        constexpr size_t PATTERN_LEN = RunTraits::LEN_BITS;

        segment >>= CODE_UNIT_SIZE_BITS;

        run.pattern_length_ = segment & make_mask_safe(PATTERN_LEN);

        segment >>= PATTERN_LEN;
        run.pattern_ = segment & make_mask_safe(run.pattern_length_);
        segment >>= run.pattern_length_;

        size_t run_len_bits = unit_size * CODE_UNIT_BITS_MIN - PATTERN_LEN - CODE_UNIT_SIZE_BITS - run.pattern_length() * Bps;

        if (run_len_bits) {
            run.run_length_ = segment & make_mask_safe(run_len_bits);
        }
        else {
            run.run_length_ = 1;
        }

        return unit_size;
    }

    static size_t decode_segment_to(SegmentT& segment, Span<RunT> span) noexcept
    {
        size_t units = 0;

        for (size_t c = 0; c < CODE_UNITS_PER_SEGMENT_MAX;)
        {
            size_t unit_size = RunTraits::decode_unit_to(segment, span[units]);
            c += unit_size;

            segment >>= (unit_size * CODE_UNIT_BITS_MIN);
        }

        for (size_t u = units; u < span.size(); u++) {
            span[u] = RunT{};
        }

        return units;
    }

    static void write_segments_to(
            Span<const RunT> source,
            Span<AtomT> target,
            size_t& tgt_idx,
            size_t next_limit
    ) noexcept
    {
        for (size_t c = 0; c < source.size(); c++)
        {
            if (source[c])
            {
                size_t units = RunTraits::estimate_size(source[c]);

                if (units > 4) {
                    println("Run size: {} {} {}", units, c, source[c]);
                }

                if (MMA_UNLIKELY(tgt_idx + units > next_limit))
                {
                    size_t remainder = next_limit - tgt_idx;
                    // FIXME: Padding value is not necessary a single unit (for up to 255 units).
                    // Longer paddings (for long segments) will require either multi-unit paddings,
                    // or splitting paddings into mutiple paddings runs.

                    // Code below assumes that CODE_UNITS_PER_SEGMENT_MAX is less than 256
                    static_assert(CODE_UNITS_PER_SEGMENT_MAX < 256);

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

    static size_t write_segments_to(Span<const RunT> prefix, Span<const RunT> source, Span<AtomT> target) noexcept
    {
        size_t tgt_idx{};
        size_t next_limit = SEGMENT_SIZE_UNITS;

        write_segments_to(prefix, target, tgt_idx, next_limit);
        write_segments_to(source, target, tgt_idx, next_limit);
        finish_setgment(target, tgt_idx);

        return tgt_idx;
    }

    static size_t write_segments_to(Span<const RunT> source, Span<AtomT> target, size_t start) noexcept
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

                if (units > 4) {
                    println("Compute size: {} :: {}", units, source[c]);
                }

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

    static void finish_setgment(Span<AtomT> span, size_t start) noexcept {
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

    static size_t pattern_rank(const RunT& run, size_t symbol) noexcept
    {
        size_t rank{};

        for (size_t c = 0; c < run.pattern_length(); c++) {
            rank += run.symbol(c) == symbol;
        }

        return rank;
    }

    static size_t pattern_rank(const RunT& run, size_t idx, size_t symbol) noexcept
    {
        size_t rank{};

        for (size_t c = 0; c < idx; c++) {
            rank += run.symbol(c) == symbol;
        }

        return rank;
    }



    static size_t rank_eq(const RunT& run, size_t idx, size_t symbol) noexcept
    {
        if (run.run_length() == 1) {
            size_t pattern_rank = RunTraits::pattern_rank(run, idx, symbol);
            return pattern_rank;
        }
        else {
            size_t full_rank = pattern_rank(run, symbol);

            size_t run_prefix = idx / run.pattern_length();
            size_t idx_prefix = run_prefix * run.pattern_length();
            size_t idx_local  = idx - idx_prefix;

            size_t pattern_rank = RunTraits::pattern_rank(run, idx_local, symbol);

            size_t rank = run_prefix * full_rank;

            rank += pattern_rank;

            return rank;
        }
    }

    template <typename Fn>
    static size_t rank_fn(const RunT& run, size_t idx, size_t symbol, Fn&& fn) noexcept
    {
        if (run.run_length() == 1) {
            size_t pattern_rank = RunTraits::pattern_rank_fn(run, idx, symbol, std::forward<Fn>(fn));
            return pattern_rank;
        }
        else {
            size_t full_rank = RunTraits::pattern_rank_fn(run, symbol, std::forward<Fn>(fn));

            size_t run_prefix = idx / run.pattern_length();
            size_t idx_prefix = run_prefix * run.pattern_length();
            size_t idx_local  = idx - idx_prefix;

            size_t pattern_rank = RunTraits::pattern_rank_fn(run, idx_local, symbol, std::forward<Fn>(fn));

            size_t rank = run_prefix * full_rank;

            rank += pattern_rank;

            return rank;
        }
    }

    template <typename T>
    static void pattern_ranks(const RunT& run, Span<T> sink) noexcept {
        for (size_t c = 0; c < run.pattern_length(); c++) {
            size_t sym = run.symbol(c);
            ++sink[sym];
        }
    }

    template <typename T>
    static void full_ranks(const RunT& run, Span<T> sink) noexcept
    {
        if (run.run_length() > 1)
        {
            uint64_t tmp[SYMBOLS]{0,};
            RunTraits::pattern_ranks(run, Span<uint64_t>(tmp, SYMBOLS));

            for (size_t s = 0; s < SYMBOLS; s++)
            {
                sink[s] += tmp[s] * run.run_length();
            }
        }
        else {
            RunTraits::pattern_ranks(run, sink);
        }
    }

    template <typename T>
    static void ranks(const RunT& run, size_t idx, Span<T> sink) noexcept
    {
        if (run.run_length() == 1) {
            RunTraits::pattern_ranks(run, idx, sink);
        }
        else {
            size_t run_prefix = idx / run.pattern_length();
            size_t idx_prefix = run_prefix * run.pattern_length();
            size_t idx_local  = idx - idx_prefix;

            if (run_prefix)
            {
                uint64_t tmp[SYMBOLS]{0,};
                RunTraits::pattern_ranks(run, Span<uint64_t>(tmp, SYMBOLS));

                for (size_t s = 0; s < SYMBOLS; s++)
                {
                    sink[s] += tmp[s] * run_prefix;
                }
            }

            RunTraits::pattern_ranks(run, idx_local, sink);
        }
    }

    template <typename Fn>
    static size_t pattern_rank_fn(const RunT& run, size_t symbol, Fn&& fn) noexcept {
        size_t rank{};
        for (size_t c = 0; c < run.pattern_length(); c++) {
            size_t sym = run.symbol(c);
            rank += fn(sym, symbol);
        }
        return rank;
    }


    template <typename Fn>
    static size_t pattern_rank_fn(const RunT& run, size_t idx, size_t symbol, Fn&& fn) noexcept {
        size_t rank{};
        for (size_t c = 0; c < idx; c++) {
            size_t sym = run.symbol(c);
            rank += fn(sym, symbol);
        }
        return rank;
    }


    static size_t pattern_select_fw(const RunT& run, size_t rank, size_t symbol) noexcept {
        return pattern_select_fw_fn(run, rank, symbol, [](size_t run_sym, size_t sym){return run_sym == sym;});
    }


    template <typename Fn>
    static size_t pattern_select_fw_fn(const RunT& run, size_t rank, size_t symbol, Fn&& compare) noexcept
    {
        size_t rr{};
        ++rank;
        for (size_t c = 0; c < run.pattern_length(); c++)
        {
            size_t sym = run.symbol(c);
            rr += compare(sym, symbol);

            if (rr == rank) {
                return c;
            }
        }

        return run.pattern_length();
    }



    template <typename Fn>
    static size_t pattern_select_bw_fn(const RunT& run, size_t rank, size_t symbol, Fn&& compare) noexcept
    {
        size_t rr{};

        ++rank;
        for (size_t c = run.pattern_length(); c > 0; c--)
        {
            size_t sym = run.symbol(c - 1);
            rr += compare(sym, symbol);

            if (rr == rank) {
                return c - 1;
            }
        }

        return run.pattern_length();
    }





    static size_t select_fw_eq(const RunT& run, size_t rank, size_t symbol) noexcept
    {
        if (run.run_length() == 1) {
            return RunTraits::pattern_select_fw(run, rank, symbol);
        }
        else {
            size_t rr = RunTraits::pattern_rank(run, symbol);

            size_t rank_base = rank / rr;
            size_t rank_local = rank - rank_base * rr;

            size_t idx = RunTraits::pattern_select_fw(run, rank_local, symbol);

            return idx + run.pattern_length() * rank_base;
        }
    }

private:
    struct FindGTFn {
        bool operator()(size_t run_sym, size_t sym) const noexcept {
            return run_sym > sym;
        }
    };

    struct FindGEFn {
        bool operator()(size_t run_sym, size_t sym) const noexcept {
            return run_sym >= sym;
        }
    };

    struct FindLTFn {
        bool operator()(size_t run_sym, size_t sym) const noexcept {
            return run_sym < sym;
        }
    };

    struct FindLEFn {
        bool operator()(size_t run_sym, size_t sym) const noexcept {
            return run_sym <= sym;
        }
    };

    struct FindEQFn {
        bool operator()(size_t run_sym, size_t sym) const noexcept {
            return run_sym == sym;
        }
    };

    struct FindNEQFn {
        bool operator()(size_t run_sym, size_t sym) const noexcept {
            return run_sym != sym;
        }
    };

public:
    static size_t select_fw_gt(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindGTFn());
    }

    static size_t select_fw_lt(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindLTFn());
    }

    static size_t select_fw_ge(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindGEFn());
    }

    static size_t select_fw_le(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindLEFn());
    }

    static size_t select_fw_neq(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_fw_fn(run, rank, symbol, FindNEQFn());
    }


    template <typename Fn>
    static size_t select_bw_eq(const RunT& run, size_t rank, size_t symbol) noexcept
    {
        if (run.run_length() == 1) {
            return RunTraits::pattern_select_bw_fn(run, rank, symbol, FindEQFn());
        }
        else {
            size_t rr = RunTraits::pattern_rank(run, symbol);

            size_t rank_base = rank / rr;
            size_t rank_local = rank - rank_base * rr;

            size_t idx = RunTraits::pattern_select_bw(run, rank_local, symbol, FindEQFn());

            return run.full_run_length() - idx + run.patter_length() * rank_base;
        }
    }



    static size_t select_bw_gt(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_bw_fn(run, rank, symbol, FindGTFn());
    }

    static size_t select_bw_lt(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_bw_fn(run, rank, symbol, FindLTFn());
    }

    static size_t select_bw_ge(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_bw_fn(run, rank, symbol, FindGEFn());
    }

    static size_t select_bw_le(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_bw_fn(run, rank, symbol, FindLEFn());
    }

    static size_t select_bw_neq(const RunT& run, size_t rank, size_t symbol) noexcept {
        return select_bw_fn(run, rank, symbol, FindNEQFn());
    }



    template <typename Fn>
    static size_t select_fw_fn(const RunT& run, size_t rank, size_t symbol, Fn&& compare) noexcept
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



    template <typename Fn>
    static size_t select_bw_fn(const RunT& run, size_t rank, size_t symbol, Fn&& compare) noexcept
    {
        if (run.run_length() == 1) {
            return RunTraits::pattern_select_bw_fn(run, rank, symbol, std::forward<Fn>(compare));
        }
        else {
            size_t rr = RunTraits::pattern_rank_fn(run, symbol, std::forward<Fn>(compare));
            size_t rank_base = rank / rr;
            size_t rank_local = rank - rank_base * rr;

            size_t idx = RunTraits::pattern_select_bw_fn(run, rank_local, symbol, std::forward<Fn>(compare));

            return run.full_run_length() - ((run.pattern_length() - idx) + run.pattern_length() * rank_base);
        }
    }


    static size_t full_rank_eq(const RunT& run, size_t symbol) noexcept {
        return RunTraits::pattern_rank(run, symbol) * run.run_length();
    }

    static size_t full_rank_gt(const RunT& run, size_t symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindGTFn()) * run.run_length();
    }

    static size_t full_rank_ge(const RunT& run, size_t symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindGEFn()) * run.run_length();
    }

    static size_t full_rank_lt(const RunT& run, size_t symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindLTFn()) * run.run_length();
    }

    static size_t full_rank_le(const RunT& run, size_t symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindLEFn()) * run.run_length();
    }

    static size_t full_rank_neq(const RunT& run, size_t symbol) noexcept {
        return RunTraits::pattern_rank_fn(run, symbol, FindNEQFn()) * run.run_length();
    }





    static size_t rank_gt(const RunT& run, size_t idx, size_t symbol) noexcept {
        return rank_fn(run, idx, symbol, FindGTFn());
    }

    static size_t rank_ge(const RunT& run, size_t idx, size_t symbol) noexcept {
        return rank_fn(run, idx, symbol, FindGEFn());
    }

    static size_t rank_lt(const RunT& run, size_t idx, size_t symbol) noexcept {
        return rank_fn(run, idx, symbol, FindLTFn());
    }

    static size_t rank_le(const RunT& run, size_t idx, size_t symbol) noexcept {
        return rank_fn(run, idx, symbol, FindLEFn());
    }

    static size_t rank_neq(const RunT& run, size_t idx, size_t symbol) noexcept {
        return rank_fn(run, idx, symbol, FindNEQFn());
    }



    static size_t pattern_count_fw(const RunT& run, size_t start_idx, size_t symbol) noexcept
    {
        size_t c = start_idx;
        for (; c < run.pattern_length(); c++) {
            if (run.symbol(c) != symbol) {
                break;
            }
        }

        return c - start_idx;
    }

    static size_t pattern_count_bw(const RunT& run, size_t start_idx, size_t symbol) noexcept
    {
        size_t c = start_idx + 1;
        for (; c > 0; c--) {
            if (run.symbol(c - 1) != symbol) {
                return start_idx - (c - 1);
            }
        }
        return start_idx + 1;
    }


    static size_t count_fw(const RunT& run, size_t start_idx, size_t symbol) noexcept
    {
        if (run.run_length() == 1) {
            return RunTraits::pattern_count_fw(run, start_idx, symbol);
        }
        else {
            size_t idx_base  = start_idx / run.pattern_length();
            size_t idx_local = start_idx - idx_base * run.pattern_length();

            size_t suffix_cnt = RunTraits::pattern_count_fw(run, idx_local, symbol);

            if (MMA_UNLIKELY(suffix_cnt == run.pattern_length() - idx_local) && (idx_base + 1 < run.run_length()))
            {
                size_t prefix_cnt = RunTraits::pattern_count_fw(run, 0, symbol);
                if (prefix_cnt == run.pattern_length()) {
                    return suffix_cnt + run.pattern_length() * (run.run_length() - idx_base - 1);
                }
                else {
                    return suffix_cnt + prefix_cnt;
                }
            }
            else {
                return suffix_cnt;
            }
        }
    }

    static size_t count_bw(const RunT& run, size_t start_idx, size_t symbol) noexcept
    {
        if (run.run_length() == 1) {
            return RunTraits::pattern_count_bw(run, start_idx, symbol);
        }
        else {
            size_t idx_base  = start_idx / run.pattern_length();
            size_t idx_local = start_idx - idx_base * run.pattern_length();

            size_t prefix_cnt = RunTraits::pattern_count_bw(run, idx_local, symbol);

            if (MMA_UNLIKELY(prefix_cnt == (idx_local + 1) && idx_base > 0))
            {
                size_t suffix_cnt = RunTraits::pattern_count_bw(run, run.pattern_length() - 1, symbol);
                if (prefix_cnt == run.pattern_length())
                {
                    return prefix_cnt + run.pattern_length() * idx_base;
                }
                else {
                    return suffix_cnt + prefix_cnt;
                }
            }
            else {
                return prefix_cnt;
            }
        }
    }
};

template <size_t Bps>
struct SSRLERunCommonTraits: SSRLERunCommonTraitsBase<Bps> {};

template <size_t Bps> struct SSRLERunTraits;

template <size_t Bps>
class SSRLERun {
    static_assert(Bps <= 8, "Unsupported alphabet size");

    using Traits = SSRLERunTraits<Bps>;

    using CodeUnitT = typename Traits::CodeUnitT;

    CodeUnitT pattern_;
    CodeUnitT pattern_length_;
    CodeUnitT run_length_;

    static constexpr CodeUnitT SYMBOL_MASK          = Traits::SYMBOL_MASK;
    static constexpr CodeUnitT MAX_PATTERN_LENGTH   = Traits::MAX_PATTERN_LENGTH;

    static constexpr CodeUnitT MAX_RUN_LENGTH       = Traits::MAX_RUN_LENGTH;

    friend struct SSRLERunTraits<Bps>;
    friend struct SSRLERunCommonTraitsBase<Bps>;
    friend struct SSRLERunCommonTraits<Bps>;

public:
    constexpr SSRLERun() noexcept : pattern_(), pattern_length_(), run_length_() {}
    constexpr SSRLERun(
        CodeUnitT pattern_length,
        CodeUnitT pattern,
        CodeUnitT run_length
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

    CodeUnitT run_length() const noexcept {
        return run_length_;
    }

    CodeUnitT full_run_length() const noexcept {
        return run_length_ * pattern_length_;
    }

    CodeUnitT pattern_length() const noexcept {
        return pattern_length_;
    }

    CodeUnitT pattern() const noexcept {
        return pattern_;
    }

    void set_pattern(CodeUnitT pattern) noexcept {
        pattern_ = pattern;
    }

    CodeUnitT symbol(size_t idx) const noexcept {
        size_t p_idx;
        if (run_length_ == 1) {
            p_idx = idx;
        }
        else {
            p_idx = idx % pattern_length_;
        }
        return (pattern_ >> (p_idx * Bps)) & SYMBOL_MASK;
    }

    void set_symbol(size_t idx, CodeUnitT symbol) noexcept
    {
        CodeUnitT mask = ((static_cast<CodeUnitT>(1) << Bps) - 1);

        size_t p_idx;
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


    Optional<typename SSRLERunTraits<Bps>::SplitResult> split(size_t at) const noexcept
    {
        return Traits::split(*this, at);
    }

    Optional<SSRLERunArray<Bps>> remove(size_t from, size_t to) const noexcept
    {
        return Traits::remove(*this, from, to);
    }

    Optional<SSRLERunArray<Bps>> insert(const SSRLERun& run, size_t at) const noexcept
    {
        return Traits::insert(*this, run, at);
    }


    bool merge(const SSRLERun& run) noexcept
    {
        return Traits::merge(*this, run);

        if (pattern_ == run.pattern())
        {
            if (Traits::is_fit(pattern_length_, run_length_ + run.run_length())) {
                run_length_ += run.run_length();
                return true;
            }
        }
        else if (run_length_ == 1 && run.run_length() == 1)
        {
            if (Traits::is_fit(pattern_length_ + run.pattern_length(), 1)) {
                CodeUnitT atom = run.pattern() << run.pattern_length();
                pattern_ |= atom;
                pattern_length_ += run.pattern_length();

                return true;
            }
        }

        return false;
    }

    std::string to_string() const
    {
        std::stringstream ss;

        auto pattern = Traits::pattern_to_string(*this);

        ss << "{" << pattern_length_ << ", '" << pattern << "', " << run_length_ << "}";

        return ss.str();
    }

    size_t full_rank_eq(size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_eq(*this, symbol);
    }

    size_t full_rank_lt(size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_lt(*this, symbol);
    }

    size_t full_rank_le(size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_le(*this, symbol);
    }

    size_t full_rank_gt(size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_gt(*this, symbol);
    }

    size_t full_rank_ge(size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_ge(*this, symbol);
    }

    size_t full_rank_neq(size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_rank_neq(*this, symbol);
    }

    size_t rank_eq(size_t idx, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_eq(*this, idx, symbol);
    }

    size_t rank_lt(size_t idx, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_lt(*this, idx, symbol);
    }

    size_t rank_le(size_t idx, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_le(*this, idx, symbol);
    }

    size_t rank_gt(size_t idx, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_gt(*this, idx, symbol);
    }

    size_t rank_ge(size_t idx, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_ge(*this, idx, symbol);
    }

    size_t rank_neq(size_t idx, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::rank_neq(*this, idx, symbol);
    }

    template <typename T>
    void ranks(size_t idx, Span<T> sink) const noexcept {
        return SSRLERunCommonTraits<Bps>::ranks(*this, idx, sink);
    }

    template <typename T>
    void full_ranks(Span<T> sink) const noexcept {
        return SSRLERunCommonTraits<Bps>::full_ranks(*this, sink);
    }

    size_t count_fw(size_t start_idx, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::count_fw(*this, start_idx, symbol);
    }

    size_t count_bw(size_t start_idx, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::count_bw(*this, start_idx, symbol);
    }

    size_t select_fw_eq(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_eq(*this, rank, symbol);
    }

    size_t select_fw_lt(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_lt(*this, rank, symbol);
    }

    size_t select_fw_le(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_le(*this, rank, symbol);
    }

    size_t select_fw_gt(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_gt(*this, rank, symbol);
    }

    size_t select_fw_ge(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_ge(*this, rank, symbol);
    }

    size_t select_fw_neq(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_fw_neq(*this, rank, symbol);
    }

    size_t select_bw_eq(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_bw_eq(*this, rank, symbol);
    }

    size_t select_bw_lt(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_bw_lt(*this, rank, symbol);
    }

    size_t select_bw_le(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_bw_le(*this, rank, symbol);
    }

    size_t select_bw_gt(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_bw_gt(*this, rank, symbol);
    }

    size_t select_bw_ge(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_bw_ge(*this, rank, symbol);
    }

    size_t select_bw_neq(size_t rank, size_t symbol) const noexcept {
        return SSRLERunCommonTraits<Bps>::select_bw_neq(*this, rank, symbol);
    }

    static SSRLERun make_run(std::initializer_list<size_t> symbols, size_t run_length) noexcept {
        return SSRLERunTraits<Bps>::make_run(symbols, run_length);
    }

    size_t to_pattern(size_t global_idx) const {
        return global_idx % pattern_length_;
    }

    size_t atom_size() const noexcept {
        return SSRLERunTraits<Bps>::estimate_size(*this);
    }
};


}

namespace fmt {

template <size_t Bps>
struct formatter<memoria::SSRLERun<Bps>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::SSRLERun<Bps>& run, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", run.to_string());
    }
};


}
