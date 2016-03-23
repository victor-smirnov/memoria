
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

template <Int BitsPerSymbol_, Int BufSize_ = BitsPerSymbol_ == 8 ? 16 : 1>
class SmallSymbolBuffer {
public:
    static constexpr Int BitsPerSymbol = BitsPerSymbol_;
    static constexpr Int BufSize = BufSize_;
private:

    UBigInt symbols_[BufSize];
    Int size_ = 0;


public:
    UBigInt* symbols() {
        return &symbols_[0];
    }

    const UBigInt* symbols() const {
        return &symbols_[0];
    }

    Int symbol(Int n) const
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        return GetBits(symbols(), n, BitsPerSymbol);
    }

    void set_symbol(Int n, Int symbol)
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        SetBits(symbols(), n, symbol, BitsPerSymbol);
    }

    void set_symbols(Int n, Int symbol, Int nsyms)
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        MEMORIA_V1_ASSERT(nsyms, <=, (Int)sizeof(Int) * 8 / BitsPerSymbol);

        return SetBits(symbols(), n, symbol, BitsPerSymbol * nsyms);
    }

    static constexpr Int max_size() {
        return sizeof(UBigInt) * 8 / BitsPerSymbol;
    }

    Int capacity() const
    {
        Int max_size0 = max_size();
        return max_size0 - size_;
    }

    void resize(Int size)
    {
        MEMORIA_V1_ASSERT(size, <=, max_size());
        this->size_ = size;
    }

    Int size() const {
        return size_;
    }

    void append(Int symbol)
    {
        MEMORIA_V1_ASSERT(size_, <, max_size());

        Int pos = size_;
        size_ += 1;
        set_symbol(pos, symbol);
    }

    void dump(std::ostream& out = std::cout) const
    {
        dumpSymbols<Int>(out, size_, BitsPerSymbol, [this](Int pos) -> Int {
            return this->symbol(pos);
        });
    }
};


template <Int BufSize_>
class SmallSymbolBuffer<8, BufSize_> {
public:
    static constexpr Int BitsPerSymbol = 8;
    static constexpr Int BufSize = BufSize_;
private:

    UByte symbols_[BufSize];
    Int size_ = 0;

public:
    UByte* symbols() {
        return &symbols_[0];
    }

    const UByte* symbols() const {
        return &symbols_[0];
    }

    Int symbol(Int n) const
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        return symbols_[n];
    }

    void set_symbol(Int n, Int symbol)
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        symbols_[n] = symbol;
    }

    void set_symbols(Int n, Int symbols, Int nsyms)
    {
        MEMORIA_V1_ASSERT(n, <, size_);
        MEMORIA_V1_ASSERT(nsyms, <, (Int)sizeof(Int));

        for (Int c = 0; c < nsyms; c++)
        {
            symbols_[c + n] = (symbols & 0xFF);

            symbols >>= 8;
        }
    }

    static constexpr Int max_size() {
        return BufSize;
    }

    Int capacity() const
    {
        Int max_size0 = max_size();
        return max_size0 - size_;
    }

    void resize(Int size)
    {
        MEMORIA_V1_ASSERT(size, <=, max_size());
        this->size_ = size;
    }

    Int size() const {
        return size_;
    }

    void append(Int symbol)
    {
        MEMORIA_V1_ASSERT(size_, <, max_size());

        Int pos = size_;
        size_ += 1;
        set_symbol(pos, symbol);
    }

    void dump(std::ostream& out = std::cout) const
    {
        dumpSymbols<Int>(out, size_, 8, [this](Int pos) -> Int {
            return this->symbol(pos);
        });
    }
};





}}