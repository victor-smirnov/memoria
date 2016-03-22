
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/tools/bitmap_select.hpp>


namespace memoria {


template <typename Seq>
class BitmapSelectFn {

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 1, "BitmapSelectFn<> can only be used with 1-bit sequences");

    const Seq& seq_;
public:
    BitmapSelectFn(const Seq& seq): seq_(seq) {}

    SelectResult operator()(Int start, Int end, Int symbol, BigInt rank)
    {
        if (symbol)
        {
            return Select1FW(seq_.symbols(), start, end, rank);
        }
        else {
            return Select0FW(seq_.symbols(), start, end, rank);
        }
    }
};


template <typename Seq>
class SeqSelectFn {
    typedef typename Seq::Value                                                 Value;

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;

    const Seq& seq_;

    static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
            "SeqSelectFn<> can only be used with 2-7-bit sequences");

public:
    SeqSelectFn(const Seq& seq): seq_(seq) {}

    SelectResult operator()(Int start, Int end, Value symbol, Int rank)
    {
        Int cnt = 0;

        auto buf = seq_.symbols();

        for (Int c = start * BitsPerSymbol; c < end * BitsPerSymbol; c += BitsPerSymbol)
        {
            if (TestBits(buf, c, symbol, BitsPerSymbol))
            {
                cnt++;

                if (cnt == rank)
                {
                    return SelectResult(c / BitsPerSymbol, rank, true);
                }
            }
        }

        return SelectResult(end, cnt, false);
    }
};







template <typename Seq>
class Seq8SelectFn {

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 8,
            "Seq8SelectFn<> can only be used with 8-bit sequences");

    const Seq& seq_;

public:
    Seq8SelectFn(const Seq& seq): seq_(seq) {}

    SelectResult operator()(Int start, Int end, Int symbol, Int rank)
    {
        Int cnt = 0;

        auto buf = seq_.symbols();

        for (Int c = start; c < end; c++)
        {
            if (buf[c] == symbol)
            {
                cnt++;
                if (cnt == rank)
                {
                    return SelectResult(c, rank, true);
                }
            }
        }

        return SelectResult(end, cnt, false);
    }
};


}
