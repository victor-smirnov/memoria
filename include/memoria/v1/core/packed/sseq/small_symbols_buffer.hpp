
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/dump.hpp>
#include <memoria/v1/core/tools/assert.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {

template <int32_t BitsPerSymbol_, int32_t BufSize_ = BitsPerSymbol_ == 8 ? 16 : 1>
class SmallSymbolBuffer {
public:
    static constexpr int32_t BitsPerSymbol = BitsPerSymbol_;
    static constexpr int32_t BufSize = BufSize_;
private:

    uint64_t symbols_[BufSize];
    int32_t size_ = 0;


public:
    uint64_t* symbols() {
        return &symbols_[0];
    }

    const uint64_t* symbols() const {
        return &symbols_[0];
    }

    int32_t symbol(int32_t n) const
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        return GetBits(symbols(), n, BitsPerSymbol);
    }

    void set_symbol(int32_t n, int32_t symbol)
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        SetBits(symbols(), n, symbol, BitsPerSymbol);
    }

    void set_symbols(int32_t n, int32_t symbol, int32_t nsyms)
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        MEMORIA_V1_ASSERT(nsyms, <=, (int32_t)sizeof(int32_t) * 8 / BitsPerSymbol);

        return SetBits(symbols(), n, symbol, BitsPerSymbol * nsyms);
    }

    static constexpr int32_t max_size() {
        return sizeof(uint64_t) * 8 / BitsPerSymbol;
    }

    int32_t capacity() const
    {
        int32_t max_size0 = max_size();
        return max_size0 - size_;
    }

    void resize(int32_t size)
    {
        MEMORIA_V1_ASSERT(size, <=, max_size());
        this->size_ = size;
    }

    int32_t size() const {
        return size_;
    }

    void append(int32_t symbol)
    {
        MEMORIA_V1_ASSERT(size_, <, max_size());

        int32_t pos = size_;
        size_ += 1;
        set_symbol(pos, symbol);
    }

    void dump(std::ostream& out = std::cout) const
    {
        dumpSymbols<int32_t>(out, size_, BitsPerSymbol, [this](int32_t pos) -> int32_t {
            return this->symbol(pos);
        });
    }
};


template <int32_t BufSize_>
class SmallSymbolBuffer<8, BufSize_> {
public:
    static constexpr int32_t BitsPerSymbol = 8;
    static constexpr int32_t BufSize = BufSize_;
private:

    uint8_t symbols_[BufSize];
    int32_t size_ = 0;

public:
    uint8_t* symbols() {
        return &symbols_[0];
    }

    const uint8_t* symbols() const {
        return &symbols_[0];
    }

    int32_t symbol(int32_t n) const
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        return symbols_[n];
    }

    void set_symbol(int32_t n, int32_t symbol)
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        symbols_[n] = symbol;
    }

    void set_symbols(int32_t n, int32_t symbols, int32_t nsyms)
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        MEMORIA_V1_ASSERT(nsyms, <, (int32_t)sizeof(int32_t));

        for (int32_t c = 0; c < nsyms; c++)
        {
            symbols_[c + n] = (symbols & 0xFF);

            symbols >>= 8;
        }
    }

    static constexpr int32_t max_size() {
        return BufSize;
    }

    int32_t capacity() const
    {
        int32_t max_size0 = max_size();
        return max_size0 - size_;
    }

    void resize(int32_t size)
    {
        MEMORIA_V1_ASSERT(size, <=, max_size());
        this->size_ = size;
    }

    int32_t size() const {
        return size_;
    }

    void append(int32_t symbol)
    {
        MEMORIA_V1_ASSERT(size_, <, max_size());

        int32_t pos = size_;
        size_ += 1;
        set_symbol(pos, symbol);
    }

    void dump(std::ostream& out = std::cout) const
    {
        dumpSymbols<int32_t>(out, size_, 8, [this](int32_t pos) -> int32_t {
            return this->symbol(pos);
        });
    }
};





}}