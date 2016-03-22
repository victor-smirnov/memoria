
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/tools/bitmap_select.hpp>


namespace memoria {


template <typename Seq>
class BitmapToolsFn {
    typedef typename Seq::Values                                               Values;
    typedef typename Seq::Value                                                 Value;

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 1, "BitmapToolsFn<> can only be used with 1-bit sequences");

    const Seq& seq_;
public:
    BitmapToolsFn(const Seq& seq): seq_(seq) {}

    Values sum(Int start, Int end)
    {
        Values values;

        BigInt rank1 = PopCount(seq_.symbols(), start, end);

        values[1] = rank1;
        values[0] = end - start - rank1;

        return values;
    }

    Value get(const Value* symbols, Int idx)
    {
        return GetBit(symbols, idx);
    }

    bool test(const Value* symbols, Int idx, Value value)
    {
        return TestBit(symbols, idx) == value;
    }

    void set(Value* symbols, Int idx, Value value)
    {
        return SetBit(symbols, idx, value);
    }

    void move(Value* symbols, Int from, Int to, Int lenght)
    {
        MoveBits(symbols, symbols, from, to, lenght);
    }

    void move(const Value* src, Value* dst, Int from, Int to, Int lenght)
    {
        MoveBits(src, dst, from, to, lenght);
    }
};




template <typename Seq>
class SeqToolsFn {
    typedef typename Seq::Values           Values;
    typedef typename Seq::Value             Value;

    static const Int BitsPerSymbol          = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
                "SeqToolsFn<> can only be used with 2-7-bit sequences");

    const Seq& seq_;
public:
    SeqToolsFn(const Seq& seq): seq_(seq) {}

    Values sum(Int start, Int end)
    {
        Values values;

        auto symbols = seq_.symbols();

        for (Int idx = start * BitsPerSymbol; idx < end * BitsPerSymbol; idx += BitsPerSymbol)
        {
            Int symbol = GetBits(symbols, idx, BitsPerSymbol);
            values[symbol]++;
        }

        return values;
    }

    Value get(const Value* symbols, Int idx)
    {
        return GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
    }

    bool test(const Value* symbols, Int idx, Value value)
    {
        return TestBits(symbols, idx * BitsPerSymbol, value, BitsPerSymbol);
    }

    void set(Value* symbols, Int idx, Value value)
    {
        return SetBits(symbols, idx * BitsPerSymbol, value, BitsPerSymbol);
    }

    void move(Value* symbols, Int from, Int to, Int lenght)
    {
        MoveBits(symbols, symbols, from * BitsPerSymbol, to * BitsPerSymbol, lenght * BitsPerSymbol);
    }

    void move(const Value* src, Value* dst, Int from, Int to, Int lenght)
    {
        MoveBits(src, dst, from * BitsPerSymbol, to * BitsPerSymbol, lenght * BitsPerSymbol);
    }
};




template <typename Seq>
class Seq8ToolsFn {
    typedef typename Seq::Values           Values;
    typedef typename Seq::Value             Value;

    static const Int BitsPerSymbol          = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 8, "Seq8ToolsFn<> can only be used with 8-bit sequences");

    const Seq& seq_;
public:
    Seq8ToolsFn(const Seq& seq): seq_(seq) {}

    Values sum(Int start, Int end)
    {
        Values values;

        auto symbols = seq_.symbols();

        for (Int idx = start; idx < end; idx++)
        {
            Int symbol = symbols[idx];
            values[symbol]++;
        }

        return values;
    }

    Value get(const Value* symbols, Int idx)
    {
        return symbols[idx];
    }

    bool test(const Value* symbols, Int idx, Value value)
    {
        return symbols[idx] == value;
    }

    void set(Value* symbols, Int idx, Value value)
    {
        symbols[idx] = value;
    }

    void move(Value* symbols, Int from, Int to, Int lenght)
    {
        CopyByteBuffer(symbols + from, symbols + to, lenght);
    }

    void move(const Value* src, Value* dst, Int from, Int to, Int lenght)
    {
        CopyByteBuffer(src + from, dst + to, lenght);
    }
};




}
