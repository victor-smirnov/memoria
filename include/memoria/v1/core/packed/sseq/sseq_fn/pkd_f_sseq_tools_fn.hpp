
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/tools/bitmap_select.hpp>


namespace memoria {
namespace v1 {


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




}}