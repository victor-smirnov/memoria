
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

#include <memoria/core/tools/bitmap_select.hpp>


namespace memoria {

template <typename Seq>
class BitmapToolsFn {
    typedef typename Seq::Values                                               Values;
    typedef typename Seq::Value                                                 Value;

    static const int32_t BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 1, "BitmapToolsFn<> can only be used with 1-bit sequences");

    const Seq& seq_;
public:
    BitmapToolsFn(const Seq& seq): seq_(seq) {}

    Values sum(int32_t start, int32_t end)
    {
        Values values;

        int64_t rank1 = PopCount(seq_.symbols(), start, end);

        values[1] = rank1;
        values[0] = end - start - rank1;

        return values;
    }

    Value get(const Value* symbols, int32_t idx)
    {
        return GetBit(symbols, idx);
    }

    bool test(const Value* symbols, int32_t idx, Value value)
    {
        return TestBit(symbols, idx) == value;
    }

    void set(Value* symbols, int32_t idx, Value value)
    {
        return SetBit(symbols, idx, value);
    }

    void move(Value* symbols, int32_t from, int32_t to, int32_t lenght)
    {
        MoveBits(symbols, symbols, from, to, lenght);
    }

    void move(const Value* src, Value* dst, int32_t from, int32_t to, int32_t lenght)
    {
        MoveBits(src, dst, from, to, lenght);
    }
};




template <typename Seq>
class SeqToolsFn {
    typedef typename Seq::Values           Values;
    typedef typename Seq::Value             Value;

    static const int32_t BitsPerSymbol          = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
                "SeqToolsFn<> can only be used with 2-7-bit sequences");

    const Seq& seq_;
public:
    SeqToolsFn(const Seq& seq): seq_(seq) {}

    Values sum(int32_t start, int32_t end)
    {
        Values values;

        auto symbols = seq_.symbols();

        for (int32_t idx = start * BitsPerSymbol; idx < end * BitsPerSymbol; idx += BitsPerSymbol)
        {
            int32_t symbol = GetBits(symbols, idx, BitsPerSymbol);
            values[symbol]++;
        }

        return values;
    }

    Value get(const Value* symbols, int32_t idx)
    {
        return GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
    }

    bool test(const Value* symbols, int32_t idx, Value value)
    {
        return TestBits(symbols, idx * BitsPerSymbol, value, BitsPerSymbol);
    }

    void set(Value* symbols, int32_t idx, Value value)
    {
        return SetBits(symbols, idx * BitsPerSymbol, value, BitsPerSymbol);
    }

    void move(Value* symbols, int32_t from, int32_t to, int32_t lenght)
    {
        MoveBits(symbols, symbols, from * BitsPerSymbol, to * BitsPerSymbol, lenght * BitsPerSymbol);
    }

    void move(const Value* src, Value* dst, int32_t from, int32_t to, int32_t lenght)
    {
        MoveBits(src, dst, from * BitsPerSymbol, to * BitsPerSymbol, lenght * BitsPerSymbol);
    }
};




template <typename Seq>
class Seq8ToolsFn {
    typedef typename Seq::Values           Values;
    typedef typename Seq::Value             Value;

    static const int32_t BitsPerSymbol          = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 8, "Seq8ToolsFn<> can only be used with 8-bit sequences");

    const Seq& seq_;
public:
    Seq8ToolsFn(const Seq& seq): seq_(seq) {}

    Values sum(int32_t start, int32_t end)
    {
        Values values;

        auto symbols = seq_.symbols();

        for (int32_t idx = start; idx < end; idx++)
        {
            int32_t symbol = symbols[idx];
            values[symbol]++;
        }

        return values;
    }

    Value get(const Value* symbols, int32_t idx)
    {
        return symbols[idx];
    }

    bool test(const Value* symbols, int32_t idx, Value value)
    {
        return symbols[idx] == value;
    }

    void set(Value* symbols, int32_t idx, Value value)
    {
        symbols[idx] = value;
    }

    void move(Value* symbols, int32_t from, int32_t to, int32_t lenght)
    {
        CopyByteBuffer(symbols + from, symbols + to, lenght);
    }

    void move(const Value* src, Value* dst, int32_t from, int32_t to, int32_t lenght)
    {
        CopyByteBuffer(src + from, dst + to, lenght);
    }
};




}
