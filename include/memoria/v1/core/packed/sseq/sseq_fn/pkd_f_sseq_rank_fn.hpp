
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/tools/bitmap_select.hpp>


namespace memoria {
namespace v1 {



template <typename Seq>
class BitmapRankFn {

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 1, "BitmapRankFn<> can only be used with 1-bit sequences");


    const Seq& seq_;
public:
    BitmapRankFn(const Seq& seq): seq_(seq) {}

    Int operator()(Int start, Int end, Int symbol)
    {
        if (symbol)
        {
            return PopCount(seq_.symbols(), start, end);
        }
        else {
            return (end - start) - PopCount(seq_.symbols(), start, end);
        }
    }
};




template <typename Seq>
class SeqRankFn {
    typedef typename Seq::Value                                                 Value;

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
            "SeqRankFn<> can only be used with 2-7-bit sequences");

    const Seq& seq_;

public:
    SeqRankFn(const Seq& seq): seq_(seq) {}

    Int operator()(Int start, Int end, Value symbol)
    {
        Int cnt = 0;

        auto buf = seq_.symbols();

        for (Int c = start * BitsPerSymbol; c < end * BitsPerSymbol; c += BitsPerSymbol)
        {
            if (TestBits(buf, c, symbol, BitsPerSymbol))
            {
                cnt++;
            }
        }

        return cnt;
    }
};




template <typename Seq>
class Seq8RankFn {

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 8,
            "Seq8RankFn<> can only be used with 2-7-bit sequences");

    const Seq& seq_;
public:
    Seq8RankFn(const Seq& seq): seq_(seq) {}

    Int operator()(Int start, Int end, Int symbol)
    {
        Int cnt = 0;

        auto buf = seq_.symbols();

        for (Int c = start; c < end; c++)
        {
            if (buf[c] == symbol)
            {
                cnt++;
            }
        }

        return cnt;
    }
};


}}