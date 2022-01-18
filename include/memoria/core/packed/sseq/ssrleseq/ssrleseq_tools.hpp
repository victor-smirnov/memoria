
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/strings/format.hpp>

namespace memoria {
namespace ssrleseq {

class CountResult {
    uint64_t count_;
    int32_t symbol_;
public:
    CountResult(uint64_t count, int32_t symbol): count_(count), symbol_(symbol) {}

    uint64_t count() const {return count_;}
    int32_t symbol() const {return symbol_;}
};


class RLESymbolsRun {
    int32_t symbol_;
    uint64_t length_;
public:
    constexpr RLESymbolsRun(): symbol_(0), length_(0) {}
    constexpr RLESymbolsRun(int32_t symbol, uint64_t length): symbol_(symbol), length_(length) {}

    int32_t symbol() const {return symbol_;}
    uint64_t length() const {return length_;}
};


template <int32_t Symbols>
static constexpr RLESymbolsRun DecodeRun(uint64_t value)
{
    constexpr uint64_t BitsPerSymbol = NumberOfBits(Symbols - 1);
    constexpr int32_t SymbolMask = (1 << BitsPerSymbol) - 1;

    return RLESymbolsRun(value & SymbolMask, value >> BitsPerSymbol);
}

template <int32_t Symbols, int32_t MaxRunLength>
static constexpr uint64_t EncodeRun(int32_t symbol, uint64_t length)
{
    constexpr uint64_t BitsPerSymbol = NumberOfBits(Symbols - 1);
    constexpr int32_t SymbolMask = (1 << BitsPerSymbol) - 1;

    if (length <= MaxRunLength)
    {
        if (length > 0)
        {
            return (symbol & SymbolMask) | (length << BitsPerSymbol);
        }
        else {
            MMA_THROW(Exception()) << WhatInfo(format_u8("Symbols run length must be >= 0 : {}", length));
        }
    }
    else {
        MMA_THROW(Exception()) << WhatInfo(format_u8("Symbols run length of {} exceeds limit ({})", length, (size_t)MaxRunLength));
    }
}


struct Location {
    size_t data_pos_;
    size_t data_length_;
    size_t local_idx_;
    size_t block_base_;
    size_t run_base_;

    RLESymbolsRun run_;
    bool out_of_range_;

    Location(size_t data_pos, size_t data_length, size_t local_idx, size_t block_base, size_t run_base, RLESymbolsRun run, bool out_of_range = false):
        data_pos_(data_pos), data_length_(data_length), local_idx_(local_idx), block_base_(block_base), run_base_(run_base), run_(run), out_of_range_(out_of_range)
    {}

    Location() {}

    size_t run_suffix() const {return run_.length() - local_idx_;}
    size_t run_prefix() const {return local_idx_;}

    size_t local_idx()  const {return local_idx_;}
    //auto symbol()       const {return run_.symbol();}
    //auto length()       const {return run_.length();}

    auto data_pos()     const {return data_pos_;}
    auto data_length()  const {return data_length_;}
    auto data_end()     const {return data_pos_ + data_length_;}
    auto block_base()   const {return block_base_;}
    auto run_base()     const {return run_base_;}

    bool out_of_range1() const {return out_of_range_;}

    const RLESymbolsRun& run() const {return run_;}
};



}}
