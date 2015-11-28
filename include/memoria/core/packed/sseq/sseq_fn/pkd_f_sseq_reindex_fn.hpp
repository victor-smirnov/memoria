
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_PACKED_SEQFN_REINDEXFN_HPP_
#define MEMORIA_CORE_PACKED_PACKED_SEQFN_REINDEXFN_HPP_

#include <memoria/core/tools/bitmap_select.hpp>


namespace memoria {





template <typename Seq>
class BitmapReindexFn {
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const Int ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const Int Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    static_assert(BitsPerSymbol == 1,
            "BitmapReindexFn<> can only be used with 1-bit sequences");

    static_assert(FixedSizeElementIndex,
            "BitmapReindexFn<> can only be used with PkdFTree<>-indexed sequences ");

public:
    void operator()(Seq& seq)
    {
        Int size = seq.size();

        if (size > ValuesPerBranch)
        {
            Int index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);
            seq.createIndex(index_size);

            Index* index = seq.index();

            Int pos = 0;

            auto symbols = seq.symbols();

            index->_insert(0, index_size, [&](Int c) -> Values
            {
                Int next = pos + ValuesPerBranch;
                Int max = next <= size ? next : size;

                Values values;

                values[1] = PopCount(symbols, pos, max);
                values[0] = (max - pos) - values[1];

                pos = next;

                return values;
            });
        }
        else {
            seq.removeIndex();
        }
    }
};


template <typename Seq>
class ReindexFn {
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const Int ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const Int Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    static_assert(BitsPerSymbol >= 2,
                "ReindexFn<> can only be used with 2-8-bit sequences");

    static_assert(FixedSizeElementIndex,
                    "ReindexFn<> can only be used with PkdFTree<>-indexed sequences ");

public:
    void operator()(Seq& seq)
    {
        Int size = seq.size();

        if (size > ValuesPerBranch)
        {
            Int index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);
            seq.createIndex(index_size);

            Index* index = seq.index();

            Int pos = 0;

            index->_insert(0, index_size, [&](Int c) -> Values
            {
                auto symbols = seq.symbols();

                Int next = pos + ValuesPerBranch;
                Int max = next <= size ? next : size;

                Values values;

                for (; pos < max; pos++)
                {
                    Int symbol = GetBits(symbols, pos * BitsPerSymbol, BitsPerSymbol);
                    values[symbol]++;
                }

                return values;
            });
        }
        else {
            seq.removeIndex();
        }
    }
};


template <typename Seq>
class VLEReindexFn {
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;
    typedef typename Index::Codec                                               Codec;
    typedef typename Index::SizesT                                              SizesT;

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const Int ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const Int Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
                "VLEReindexFn<> can only be used with 2-7-bit sequences");

    static_assert(!FixedSizeElementIndex,
                "VLEReindexFn<> can only be used with PkdVTree<>-indexed sequences ");

public:
    void operator()(Seq& seq)
    {
        Int size = seq.size();

        if (size > ValuesPerBranch)
        {
            Codec codec;

            SizesT length;

            auto symbols = seq.symbols();

            for (Int b = 0; b < size; b += ValuesPerBranch)
            {
                Int next = b + ValuesPerBranch;
                Int max = next <= size ? next : size;

                Values values;

                for (Int pos = b; pos < max; pos++)
                {
                    Int symbol = GetBits(symbols, pos * BitsPerSymbol, BitsPerSymbol);
                    values[symbol]++;
                }

                for (Int c = 0; c < Blocks; c++)
                {
                    length[c] += codec.length(values[c]);
                }
            }

            seq.createIndex(length);

            Index* index = seq.index();

            symbols = seq.symbols();

            SizesT at;

            for (Int b = 0; b < size; b += ValuesPerBranch)
            {
            	Int next = b + ValuesPerBranch;
            	Int max = next <= size ? next : size;

            	Values values;

            	for (Int pos = b; pos < max; pos++)
            	{
            		Int symbol = GetBits(symbols, pos * BitsPerSymbol, BitsPerSymbol);
            		values[symbol]++;
            	}

            	SizesT lengths;

            	for (Int c = 0; c < Blocks; c++)
            	{
            		lengths[c] = codec.length(values[c]);
            	}

            	at = index->populate(at, lengths, 1, [&](Int block, Int idx){
            		return values[block];
            	});
            }

            index->reindex();
        }
        else {
            seq.removeIndex();
        }
    }
};



template <typename Seq>
class VLEReindex8Fn {
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;
    typedef typename Index::Codec                                               Codec;
    using SizesT = typename Index::SizesT;

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const Int ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const Int Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    static_assert(BitsPerSymbol == 8,
                "VLEReindex8Fn<> can only be used with 8-bit sequences");

    static_assert(!FixedSizeElementIndex,
                "VLEReindex8Fn<> can only be used with PkdVTree<>-indexed sequences ");

public:
    void operator()(Seq& seq)
    {
        Int size = seq.size();

        if (size > ValuesPerBranch)
        {
            Codec codec;

            SizesT length;

            auto symbols = seq.symbols();

            for (Int b = 0; b < size; b += ValuesPerBranch)
            {
                Int next = b + ValuesPerBranch;
                Int max = next <= size ? next : size;

                Values values;

                for (Int pos = b; pos < max; pos++)
                {
                    Int symbol = symbols[pos];
                    values[symbol]++;
                }

                for (Int c = 0; c < Blocks; c++)
                {
                    length[c] += codec.length(values[c]);
                }
            }

            seq.createIndex(length);

            symbols = seq.symbols();

            Index* index = seq.index();

            SizesT at;

            for (Int b = 0; b < size; b += ValuesPerBranch)
            {
            	Int next = b + ValuesPerBranch;
            	Int max = next <= size ? next : size;

            	Values values;

            	for (Int pos = b; pos < max; pos++)
            	{
            		Int symbol = symbols[pos];
            		values[symbol]++;
            	}

            	SizesT lengths;

            	for (Int c = 0; c < Blocks; c++)
            	{
            		lengths[c] = codec.length(values[c]);
            	}

            	at = index->populate(at, lengths, 1, [&](Int block, Int idx){
            		return values[block];
            	});
            }

            index->reindex();
        }
        else {
            seq.removeIndex();
        }
    }
};



template <typename Seq>
class VLEReindex8BlkFn: public VLEReindex8Fn<Seq> {

    typedef VLEReindex8Fn<Seq>                                                  Base;
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;
    typedef typename Index::Codec                                               Codec;

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const Int ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const Int Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    static_assert(BitsPerSymbol == 8,
                "VLEReindex8Fn<> can only be used with 8-bit sequences");

    static_assert(!FixedSizeElementIndex,
                "VLEReindex8Fn<> can only be used with PkdVTree<>-indexed sequences ");

public:
    void operator()(Seq& seq)
    {
        Int size = seq.size();

        if (size <= ValuesPerBranch)
        {
            seq.removeIndex();
        }
//        else if (size > ValuesPerBranch && size <= 4096)
//        {
//            Codec codec;
//
//            const Int LineWidth = 4096/ValuesPerBranch;
//
//            UShort frequences[LineWidth * 256];
//            memset(frequences, 0, sizeof(frequences));
//
//            Int length = 0;
//
//            auto symbols = seq.symbols();
//
//            Int sum = 0;
//
//            Int block = 0;
//            for (Int b = 0; b < size; b += ValuesPerBranch, block++)
//            {
//                Int next = b + ValuesPerBranch;
//                Int max = next <= size ? next : size;
//
//
//                for (Int pos = b; pos < max; pos++)
//                {
//                    Int symbol = symbols[pos];
//                    frequences[symbol * LineWidth + block]++;
//                }
//
//                for (Int c = 0; c < Blocks; c++)
//                {
//                    UShort freq = frequences[c * LineWidth + block];
//                    length += codec.length(freq);
//
//                    sum += freq;
//                }
//            }
//
//            MEMORIA_ASSERT(sum, ==, size);
//
//            seq.createIndex(length);
//
//            Index* index = seq.index();
//
//            index->template insertBlock<LineWidth>(frequences, block);
//
////            index->_insert(0, )
//        }
        else {
            Base::operator ()(seq);
        }
    }
};


}


#endif
