
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_PACKED_SEQFN_REINDEXFN_HPP_
#define MEMORIA_CORE_PACKED_PACKED_SEQFN_REINDEXFN_HPP_

#include <memoria/core/tools/bitmap_select.hpp>


namespace memoria {

namespace packed_seq {

	template <typename Values, Int WindowSize, Int RawBufferSize = 1024>
	struct IndexBuffer {
		static constexpr Int NSymbols = Values::Indexes;
		using IndexType = typename Values::ElementType;

		static constexpr Int BatchSize = RawBufferSize / sizeof(IndexType) / NSymbols;
	private:

		Values buffer_[BatchSize];
		Int window_start_ 	= 0;
		Int window_end_		= WindowSize;

		Int size_;

	public:
		IndexBuffer(Int size): size_(size)
	{}

		Values* buffer() {return buffer_;}
		const Values* buffer() const {return buffer_;}

		template <typename Fn>
		Int process(Fn&& fn)
		{
			if (window_start_ < size_)
			{
				Int buffer_pos;

				for (buffer_pos = 0; buffer_pos < BatchSize; buffer_pos++)
				{
					if (window_end_ < size_)
					{
						buffer_[buffer_pos] = fn(window_start_, window_end_);

						window_start_ += WindowSize;
						window_end_ += WindowSize;
					}
					else {
						buffer_[buffer_pos] = fn(window_start_, size_);

						window_start_ += WindowSize;
						window_end_ += WindowSize;

						return buffer_pos + 1;
					}
				}

				return buffer_pos;
			}
			else
			{
				return 0;
			}
		}
	};

}


template <typename Seq>
class BitmapReindexFn {
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;

    static const Int BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const Int ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const Int Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    using BufferType = packed_seq::IndexBuffer<Values, ValuesPerBranch>;
    static constexpr Int BatchSize = BufferType::BatchSize;


    static_assert(BitsPerSymbol == 1,
            "BitmapReindexFn<> can only be used with 1-bit sequences");

    static_assert(FixedSizeElementIndex,
            "BitmapReindexFn<> can only be used with PkdFTree<>-indexed sequences ");

public:
    void reindex(Seq& seq)
    {
        Int size = seq.size();

        if (size > ValuesPerBranch)
        {
            Int index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);
            seq.createIndex(index_size);

            Index* index = seq.index();

            auto symbols = seq.symbols();

            BufferType buffer(size);

            auto fn = [&](Int start, Int end) {
            	Values histogramm;

            	histogramm[1] = PopCount(symbols, start, end);
            	histogramm[0] = (end - start) - histogramm[1];

            	return histogramm;
            };

            Int at = 0;

            Int buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
            	index->populate(at, buffer_size, [&](Int block, Int idx) {
            		return buffer.buffer()[idx][block];
            	});

            	at += buffer_size;
            }

            index->reindex();
        }
        else {
            seq.removeIndex();
        }
    }

    void check(const Seq& seq)
    {
        Int size = seq.size();

        if (size > ValuesPerBranch)
        {
            Int index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);

            auto index = seq.index();

            MEMORIA_ASSERT(index->size(), ==, index_size);


            auto symbols = seq.symbols();

            BufferType buffer(size);

            auto fn = [&](Int start, Int end) {
            	Values histogramm;

            	histogramm[1] = PopCount(symbols, start, end);
            	histogramm[0] = (end - start) - histogramm[1];

            	return histogramm;
            };

            Int at = 0;

            Int buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
            	for (Int b = 0; b < Blocks; b++)
            	{
            		for (Int c = 0; c < buffer_size; c++)
            		{
            			MEMORIA_ASSERT(index->value(b, c + at), ==, buffer.buffer()[c][b]);
            		}
            	}

            	at += buffer_size;
            }
        }
        else {
            MEMORIA_ASSERT_FALSE(seq.has_index());
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

    using BufferType = packed_seq::IndexBuffer<Values, ValuesPerBranch>;
    static constexpr Int BatchSize = BufferType::BatchSize;


    static_assert(BitsPerSymbol >= 2,
                "ReindexFn<> can only be used with 2-8-bit sequences");

    static_assert(FixedSizeElementIndex,
                    "ReindexFn<> can only be used with PkdFTree<>-indexed sequences ");

public:
    void reindex(Seq& seq)
    {
        Int size = seq.size();

        if (size > ValuesPerBranch)
        {
            Int index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);
            seq.createIndex(index_size);

            Index* index = seq.index();

            auto symbols = seq.symbols();

            auto fn = [&](Int start, Int end) {
            	Values histogramm;

            	for (Int idx = start; idx < end; idx++)
            	{
            		Int symbol = GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
            		histogramm[symbol]++;
            	}

            	return histogramm;
            };

            Int at = 0;

            BufferType buffer(size);

            Int buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
            	index->populate(at, buffer_size, [&](Int block, Int idx) {
            		return buffer.buffer()[idx][block];
            	});

            	at += buffer_size;
            }

            index->reindex();
        }
        else {
            seq.removeIndex();
        }
    }


    void check(const Seq& seq)
    {
        Int size = seq.size();

        if (size > ValuesPerBranch)
        {
            Int index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);

            auto index = seq.index();

            MEMORIA_ASSERT(index->size(), ==, index_size);

            auto symbols = seq.symbols();

            BufferType buffer(size);

            auto fn = [&](Int start, Int end) {
            	Values histogramm;

            	for (Int idx = start; idx < end; idx++)
            	{
            		Int symbol = GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
            		histogramm[symbol]++;
            	}

            	return histogramm;
            };

            Int at = 0;

            Int buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
            	for (Int b = 0; b < Blocks; b++)
            	{
            		for (Int c = 0; c < buffer_size; c++)
            		{
            			MEMORIA_ASSERT(index->value(b, c + at), ==, buffer.buffer()[c][b]);
            		}
            	}

            	at += buffer_size;
            }
        }
        else {
            MEMORIA_ASSERT_FALSE(seq.has_index());
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

    using BufferType = packed_seq::IndexBuffer<Values, ValuesPerBranch>;
    static constexpr Int BatchSize = BufferType::BatchSize;

    static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
                "VLEReindexFn<> can only be used with 2-7-bit sequences");

    static_assert(!FixedSizeElementIndex,
                "VLEReindexFn<> can only be used with PkdVTree<>-indexed sequences ");

public:
    void reindex(Seq& seq)
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

            BufferType buffer(size);

            auto fn = [&](Int start, Int end) {
            	Values histogramm;

            	for (Int idx = start; idx < end; idx++)
            	{
            		Int symbol = GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
            		histogramm[symbol]++;
            	}

            	return histogramm;
            };

            Int buffer_size;
            SizesT at;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
            	at = index->populate(at, buffer_size, [&](Int block, Int idx) {
            		return buffer.buffer()[idx][block];
            	});
            }

            index->reindex();
        }
        else {
        	seq.removeIndex();
        }
    }


    void check(const Seq& seq)
    {
    	Int size = seq.size();

    	if (size > ValuesPerBranch)
    	{
    		auto index = seq.index();

    		auto symbols = seq.symbols();

    		auto fn = [&](Int start, Int end) {
    			Values histogramm;

    			for (Int idx = start; idx < end; idx++)
    			{
    				Int symbol = GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
    				histogramm[symbol]++;
    			}

    			return histogramm;
    		};

    		BufferType buffer(size);
            Int at = 0;

            Int buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
            	for (Int b = 0; b < Blocks; b++)
            	{
            		for (Int c = 0; c < buffer_size; c++)
            		{
            			auto idx_value = index->value(b, c + at);
            			auto buf_value = buffer.buffer()[c][b];

            			MEMORIA_ASSERT(idx_value, ==, buf_value);
            		}
            	}

            	at += buffer_size;
            }
    	}
    	else {
    		MEMORIA_ASSERT_FALSE(seq.has_index());
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

    using BufferType = packed_seq::IndexBuffer<Values, ValuesPerBranch, 4096>;
    static constexpr Int BatchSize = BufferType::BatchSize;

    static_assert(BitsPerSymbol == 8,
                "VLEReindex8Fn<> can only be used with 8-bit sequences");

    static_assert(!FixedSizeElementIndex,
                "VLEReindex8Fn<> can only be used with PkdVTree<>-indexed sequences ");

public:
    void reindex(Seq& seq)
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

            BufferType buffer(size);

            SizesT at;

            auto fn = [&](Int start, Int end) {
            	Values histogramm;

            	for (Int idx = start; idx < end; idx++)
            	{
            		Int symbol = symbols[idx];
            		histogramm[symbol]++;
            	}

            	return histogramm;
            };

            Int buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
            	at = index->populate(at, buffer_size, [&](Int block, Int idx) {
            		return buffer.buffer()[idx][block];
            	});
            }

            index->reindex();
        }
        else {
            seq.removeIndex();
        }
    }

    void check(const Seq& seq)
    {
    	Int size = seq.size();

    	if (size > ValuesPerBranch)
    	{
    		auto index = seq.index();

    		auto symbols = seq.symbols();

    		auto fn = [&](Int start, Int end) {
    			Values histogramm;

            	for (Int idx = start; idx < end; idx++)
            	{
            		Int symbol = symbols[idx];
            		histogramm[symbol]++;
            	}

    			return histogramm;
    		};

    		BufferType buffer(size);
            Int at = 0;

            Int buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
            	for (Int b = 0; b < Blocks; b++)
            	{
            		for (Int c = 0; c < buffer_size; c++)
            		{
            			MEMORIA_ASSERT(index->value(b, c + at), ==, buffer.buffer()[c][b]);
            		}
            	}

            	at += buffer_size;
            }
    	}
    	else {
    		MEMORIA_ASSERT_FALSE(seq.has_index());
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
    void reindex(Seq& seq)
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
            Base::reindex(seq);
        }
    }
};


}


#endif
