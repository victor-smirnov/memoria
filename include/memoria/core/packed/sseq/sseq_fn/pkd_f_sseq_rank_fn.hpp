
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
class BitmapRankFn {

    static const int32_t BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 1, "BitmapRankFn<> can only be used with 1-bit sequences");


    const Seq& seq_;
public:
    BitmapRankFn(const Seq& seq): seq_(seq) {}

    int32_t operator()(int32_t start, int32_t end, int32_t symbol)
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

    static const int32_t BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
            "SeqRankFn<> can only be used with 2-7-bit sequences");

    const Seq& seq_;

public:
    SeqRankFn(const Seq& seq): seq_(seq) {}

    int32_t operator()(int32_t start, int32_t end, Value symbol)
    {
        int32_t cnt = 0;

        auto buf = seq_.symbols();

        for (int32_t c = start * BitsPerSymbol; c < end * BitsPerSymbol; c += BitsPerSymbol)
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

    static const int32_t BitsPerSymbol                                              = Seq::BitsPerSymbol;

    static_assert(BitsPerSymbol == 8,
            "Seq8RankFn<> can only be used with 2-7-bit sequences");

    const Seq& seq_;
public:
    Seq8RankFn(const Seq& seq): seq_(seq) {}

    int32_t operator()(int32_t start, int32_t end, int32_t symbol)
    {
        int32_t cnt = 0;

        auto buf = seq_.symbols();

        for (int32_t c = start; c < end; c++)
        {
            if (buf[c] == symbol)
            {
                cnt++;
            }
        }

        return cnt;
    }
};


}
