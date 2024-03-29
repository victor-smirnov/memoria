
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

#include <memoria/core/ssrle/ssrle_common.hpp>

#include <memoria/core/tools/bitmap_select.hpp>

#include <sstream>
#include <x86intrin.h>

namespace memoria {

template<>
struct SSRLERunTraits<1>: SSRLERunCommonTraits<1> {
    using Base = SSRLERunCommonTraits<1>;
    using Base::CODE_WORD_BITS;
    using Base::CODE_WORD_SIZE_BITS;
    using Base::CODE_UNITS_PER_WORD_MAX;
    using Base::make_mask;
    using Base::run_length_bitsize;

    using typename Base::RunDataT;

    static constexpr size_t LEN_BITS = 6;
    static constexpr size_t HEADER_BITS = LEN_BITS + CODE_WORD_SIZE_BITS;

    static constexpr size_t DATA_BITS  = CODE_WORD_BITS - CODE_WORD_SIZE_BITS - LEN_BITS;

    static constexpr bool is_fit(RunDataT pattern_length, size_t run_length)
    {
        size_t rl_size = run_length_bitsize(run_length);
        return pattern_length + rl_size <= DATA_BITS;
    }

    static std::string pattern_to_string(const SSRLERun<1>& run)
    {
        std::stringstream ss;

        RunDataT mask = static_cast<RunDataT>(1);

        for (size_t i = 0; i < run.pattern_length(); i++, mask <<= 1)
        {
            ss << ((bool) (run.pattern() & mask));
        }

        return ss.str();
    }

    static size_t max_pattern_length() {
        return DATA_BITS;
    }

    static size_t max_run_length(size_t pattern_length)
    {
        if (pattern_length < DATA_BITS) {
            size_t run_len_size = DATA_BITS - pattern_length;
            return (static_cast<size_t>(1) << run_len_size) - 1;
        }
        else {
            return 1;
        }
    }

    static constexpr size_t estimate_size(const SSRLERun<1>& run) noexcept
    {
        size_t rl_size = run_length_bitsize(run.run_length());
        size_t bitlen = HEADER_BITS + run.pattern_length() + rl_size;

        return div_up(bitlen, sizeof(CodeUnitT) * 8);
    }

    static size_t popcount(uint64_t arg) noexcept {
        return __builtin_popcountll(arg);
    }


    static size_t pattern_rank(const RunT& run, size_t symbol) noexcept {
        size_t popcnt = popcount(run.pattern_);
        if (symbol) {
            return popcnt;
        }
        else {
            return (run.pattern_length() - popcnt);
        }
    }

    static size_t pattern_rank(const RunT& run, size_t idx, size_t symbol) noexcept
    {
        size_t popcnt = popcount(run.pattern_ & make_mask_safe(idx));
        if (symbol) {
            return popcnt;
        }
        else {
            return (idx - popcnt);
        }
    }


    template <typename T>
    static void pattern_ranks(const RunT& run, Span<T> sync) noexcept
    {
        size_t popcnt = popcount(run.pattern_);
        sync[0] += run.pattern_length() - popcnt;
        sync[1] += popcnt;
    }

    template <typename T>
    static void pattern_ranks(const RunT& run, size_t idx, Span<T> sync) noexcept
    {
        size_t popcnt = popcount(run.pattern_ & make_mask_safe(idx));
        sync[0] += idx - popcnt;
        sync[1] += popcnt;
    }

    static uint64_t bitmap_select(uint64_t arg, uint64_t rank) noexcept
    {
#ifdef __BMI2__
        return _tzcnt_u64(_pdep_u64(1ULL << rank, arg));
#else
        // Naive implementation for now
        uint64_t mask = 1;
        uint64_t rr{};
        ++rank;
        for (size_t c = 0; c < 64; c++, mask <<= 1) {
            rr += (arg & mask) != 0;
            if (rr == rank) {
                return c;
            }
        }
        return 64;
#endif
    }

    static size_t pattern_select_fw(const RunT& run, size_t rank, size_t symbol) noexcept
    {
        if (symbol) {
            return bitmap_select(run.pattern_, rank);
        }
        else {
            return bitmap_select((~run.pattern_) & make_mask_safe(run.pattern_length()), rank);
        }
    }

    static size_t pattern_count_fw(const RunT& run, size_t start_idx, size_t symbol) noexcept {
        if (symbol) {
            return bitmap_select((~run.pattern_) >> start_idx, 0);
        }
        else {
            size_t mask = ~make_mask_safe(run.pattern_length());
            return bitmap_select((run.pattern_ | mask) >> start_idx, 0);
        }
    }
};






}
