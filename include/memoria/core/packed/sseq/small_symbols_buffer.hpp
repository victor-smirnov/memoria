
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/core/tools/bitmap.hpp>

namespace memoria {

template <size_t BitsPerSymbol_, size_t BufSize_ = BitsPerSymbol_ == 8 ? 16 : 1>
class SmallSymbolBuffer {
public:
    static constexpr size_t BitsPerSymbol = BitsPerSymbol_;
    static constexpr size_t BufSize = BufSize_;
private:

    uint64_t symbols_[BufSize];
    size_t size_ = 0;


public:
    uint64_t* symbols() {
        return &symbols_[0];
    }

    const uint64_t* symbols() const {
        return &symbols_[0];
    }

    size_t symbol(size_t n) const
    {
        MEMORIA_ASSERT(n, <, size_);
        return GetBits(symbols(), n, BitsPerSymbol);
    }

    void set_symbol(size_t n, size_t symbol)
    {
        MEMORIA_ASSERT(n, <, size_);
        SetBits(symbols(), n, symbol, BitsPerSymbol);
    }

    void set_symbols(size_t n, size_t symbol, size_t nsyms)
    {
        MEMORIA_ASSERT(n, <, size_);
        MEMORIA_ASSERT(nsyms, <=, (size_t)sizeof(size_t) * 8 / BitsPerSymbol);

        return SetBits(symbols(), n, symbol, BitsPerSymbol * nsyms);
    }

    static constexpr size_t max_size() {
        return sizeof(uint64_t) * 8 / BitsPerSymbol;
    }

    size_t capacity() const
    {
        size_t max_size0 = max_size();
        return max_size0 - size_;
    }

    void resize(size_t size)
    {
        MEMORIA_ASSERT(size, <=, max_size());
        this->size_ = size;
    }

    size_t size() const {
        return size_;
    }

    void append(size_t symbol)
    {
        MEMORIA_ASSERT(size_, <, max_size());

        size_t pos = size_;
        size_ += 1;
        set_symbol(pos, symbol);
    }

    void dump(std::ostream& out = std::cout) const
    {
        dumpSymbols<size_t>(out, size_, BitsPerSymbol, [this](size_t pos) -> size_t {
            return this->symbol(pos);
        });
    }
};


template <size_t BufSize_>
class SmallSymbolBuffer<8, BufSize_> {
public:
    static constexpr size_t BitsPerSymbol = 8;
    static constexpr size_t BufSize = BufSize_;
private:

    uint8_t symbols_[BufSize];
    size_t size_ = 0;

public:
    uint8_t* symbols() {
        return &symbols_[0];
    }

    const uint8_t* symbols() const {
        return &symbols_[0];
    }

    size_t symbol(size_t n) const
    {
        MEMORIA_ASSERT(n, <, size_);
        return symbols_[n];
    }

    void set_symbol(size_t n, size_t symbol)
    {
        MEMORIA_ASSERT(n, <, size_);
        symbols_[n] = symbol;
    }

    void set_symbols(size_t n, size_t symbols, size_t nsyms)
    {
        MEMORIA_ASSERT(n, <, size_);
        MEMORIA_ASSERT(nsyms, <, (size_t)sizeof(size_t));

        for (size_t c = 0; c < nsyms; c++)
        {
            symbols_[c + n] = (symbols & 0xFF);

            symbols >>= 8;
        }
    }

    static constexpr size_t max_size() {
        return BufSize;
    }

    size_t capacity() const
    {
        size_t max_size0 = max_size();
        return max_size0 - size_;
    }

    void resize(size_t size)
    {
        MEMORIA_ASSERT(size, <=, max_size());
        this->size_ = size;
    }

    size_t size() const {
        return size_;
    }

    void append(size_t symbol)
    {
        MEMORIA_ASSERT(size_, <, max_size());

        size_t pos = size_;
        size_ += 1;
        set_symbol(pos, symbol);
    }

    void dump(std::ostream& out = std::cout) const
    {
        dumpSymbols<size_t>(out, size_, 8, [this](size_t pos) -> size_t {
            return this->symbol(pos);
        });
    }
};





}
