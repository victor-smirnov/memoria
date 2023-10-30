
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

#include <sstream>


namespace memoria {

template <>
struct SSRLERunTraits<7>: SSRLERunCommonTraits<7> {
    using Base = SSRLERunCommonTraits<7>;
    using Base::CODE_WORD_BITS;
    using Base::CODE_WORD_SIZE_BITS;
    using Base::CODE_UNITS_PER_WORD_MAX;
    using Base::make_mask;
    using Base::run_length_bitsize;

    using typename Base::RunDataT;

    static constexpr size_t Bps = 7;
    static constexpr size_t LEN_BITS = 3;
    static constexpr size_t HEADER_BITS = LEN_BITS + CODE_WORD_SIZE_BITS;

    static constexpr size_t DATA_BITS  = CODE_WORD_BITS - CODE_WORD_SIZE_BITS - LEN_BITS;

    static constexpr bool is_fit(RunDataT pattern_length, size_t run_length)
    {
        size_t rl_size = run_length_bitsize(run_length);
        return pattern_length * Bps + rl_size <= DATA_BITS;
    }

    static std::string pattern_to_string(const SSRLERun<Bps>& run)
    {
        std::stringstream ss;

        RunDataT mask = make_mask_safe(Bps);

        for (size_t i = 0; i < run.pattern_length(); i++) {
            ss << ((run.pattern() >> (Bps * i)) & mask);

            if (i < run.pattern_length() - 1) {
                ss << ".";
            }
        }

        return ss.str();
    }

    static size_t max_pattern_length() {
        return DATA_BITS / Bps;
    }

    static size_t max_run_length(size_t pattern_length)
    {
        size_t pattern_bit_length = pattern_length * Bps;
        if (pattern_bit_length < DATA_BITS) {
            size_t run_len_size = DATA_BITS - pattern_bit_length;
            return (size_t{1} << run_len_size) - 1;
        }
        else {
            return 1;
        }
    }

    static constexpr size_t estimate_size(const SSRLERun<Bps>& run) noexcept
    {
        size_t rl_size = run_length_bitsize(run.run_length());
        size_t bitlen = HEADER_BITS + run.pattern_length() * Bps + rl_size;

        return div_up(bitlen, sizeof(CodeUnitT) * 8);
    }
};

}
