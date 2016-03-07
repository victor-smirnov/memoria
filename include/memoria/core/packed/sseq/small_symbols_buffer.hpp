
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_SMALL_SYMBOLS_BUFFER_HPP_
#define MEMORIA_CORE_PACKED_SMALL_SYMBOLS_BUFFER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/core/tools/bitmap.hpp>

namespace memoria {

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
        MEMORIA_ASSERT(n, <, size_);
        return GetBits(symbols(), n, BitsPerSymbol);
    }

    void set_symbol(Int n, Int symbol)
    {
        MEMORIA_ASSERT(n, <, size_);
        SetBits(symbols(), n, symbol, BitsPerSymbol);
    }

    void set_symbols(Int n, Int symbol, Int nsyms)
    {
        MEMORIA_ASSERT(n, <, size_);
        MEMORIA_ASSERT(nsyms, <=, (Int)sizeof(Int) * 8 / BitsPerSymbol);

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
        MEMORIA_ASSERT(size, <=, max_size());
        this->size_ = size;
    }

    Int size() const {
        return size_;
    }

    void append(Int symbol)
    {
        MEMORIA_ASSERT(size_, <, max_size());

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
        MEMORIA_ASSERT(n, <, size_);
        return symbols_[n];
    }

    void set_symbol(Int n, Int symbol)
    {
        MEMORIA_ASSERT(n, <, size_);
        symbols_[n] = symbol;
    }

    void set_symbols(Int n, Int symbols, Int nsyms)
    {
        MEMORIA_ASSERT(n, <, size_);
        MEMORIA_ASSERT(nsyms, <, (Int)sizeof(Int));

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
        MEMORIA_ASSERT(size, <=, max_size());
        this->size_ = size;
    }

    Int size() const {
        return size_;
    }

    void append(Int symbol)
    {
        MEMORIA_ASSERT(size_, <, max_size());

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





}


#endif
