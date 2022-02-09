
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
#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/assert.hpp>

namespace memoria {

namespace packed_seq {

    template <typename Values, size_t WindowSize, size_t RawBufferSize = 1024>
    struct IndexBuffer {
        static constexpr size_t NSymbols = Values::Indexes;
        using IndexType = typename Values::ElementType;

        static constexpr size_t BatchSize = RawBufferSize / sizeof(IndexType) / NSymbols;
    private:

        Values buffer_[BatchSize];
        size_t window_start_   = 0;
        size_t window_end_     = WindowSize;

        size_t size_;

    public:
        IndexBuffer(size_t size): size_(size)
    {}

        Values* buffer() {return buffer_;}
        const Values* buffer() const {return buffer_;}

        template <typename Fn>
        size_t process(Fn&& fn)
        {
            if (window_start_ < size_)
            {
                size_t buffer_pos;

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

    static const size_t BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const size_t ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const size_t Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    using BufferType = packed_seq::IndexBuffer<Values, ValuesPerBranch>;
    static constexpr size_t BatchSize = BufferType::BatchSize;


    static_assert(BitsPerSymbol == 1,
            "BitmapReindexFn<> can only be used with 1-bit sequences");

    static_assert(FixedSizeElementIndex,
            "BitmapReindexFn<> can only be used with PkdFTree<>-indexed sequences ");

public:
    VoidResult reindex(Seq& seq) noexcept
    {
        size_t size = seq.size();

        if (size > ValuesPerBranch)
        {
            size_t index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);
            MEMORIA_TRY_VOID(seq.createIndex(index_size));

            Index* index = seq.index();

            auto symbols = seq.symbols();

            BufferType buffer(size);

            auto fn = [&](size_t start, size_t end) {
                Values histogramm;

                histogramm[1] = PopCount(symbols, start, end);
                histogramm[0] = (end - start) - histogramm[1];

                return histogramm;
            };

            size_t at = 0;

            size_t buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
                MEMORIA_TRY_VOID(index->insert_entries(at, buffer_size, [&](size_t block, size_t idx) noexcept {
                    return buffer.buffer()[idx][block];
                }, false));

                at += buffer_size;
            }

            return index->reindex();
        }
        else {
            return seq.removeIndex();
        }

        return VoidResult::of();
    }

    void check(const Seq& seq)
    {
        size_t size = seq.size();

        if (size > ValuesPerBranch)
        {
            size_t index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);

            auto index = seq.index();

            MEMORIA_ASSERT(index->size(), ==, index_size);


            auto symbols = seq.symbols();

            BufferType buffer(size);

            auto fn = [&](size_t start, size_t end) {
                Values histogramm;

                histogramm[1] = PopCount(symbols, start, end);
                histogramm[0] = (end - start) - histogramm[1];

                return histogramm;
            };

            size_t at = 0;

            size_t buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
                for (size_t b = 0; b < Blocks; b++)
                {
                    for (size_t c = 0; c < buffer_size; c++)
                    {
                        MEMORIA_ASSERT(index->value(b, c + at), ==, buffer.buffer()[c][b]);
                    }
                }

                at += buffer_size;
            }
        }
        else {
            MEMORIA_ASSERT(seq.has_index(), ==, false);
        }
    }
};


template <typename Seq>
class ReindexFn {
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;

    static const size_t BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const size_t ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const size_t Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    using BufferType = packed_seq::IndexBuffer<Values, ValuesPerBranch>;
    static constexpr size_t BatchSize = BufferType::BatchSize;


    static_assert(BitsPerSymbol >= 2,
                "ReindexFn<> can only be used with 2-8-bit sequences");

    static_assert(FixedSizeElementIndex,
                    "ReindexFn<> can only be used with PkdFTree<>-indexed sequences ");

public:
    VoidResult reindex(Seq& seq) noexcept
    {
        size_t size = seq.size();

        if (size > ValuesPerBranch)
        {
            size_t index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);
            MEMORIA_TRY_VOID(seq.createIndex(index_size));

            Index* index = seq.index();

            auto symbols = seq.symbols();

            auto fn = [&](size_t start, size_t end) {
                Values histogramm;

                for (size_t idx = start; idx < end; idx++)
                {
                    size_t symbol = GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
                    histogramm[symbol]++;
                }

                return histogramm;
            };

            size_t at = 0;

            BufferType buffer(size);

            size_t buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
                MEMORIA_TRY_VOID(index->insert_entries(at, buffer_size, [&](size_t block, size_t idx) noexcept {
                    return buffer.buffer()[idx][block];
                }, false));

                at += buffer_size;
            }

            return index->reindex();
        }
        else {
            return seq.removeIndex();
        }

        return VoidResult::of();
    }


    void check(const Seq& seq)
    {
        size_t size = seq.size();

        if (size > ValuesPerBranch)
        {
            size_t index_size  = size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);

            auto index = seq.index();

            MEMORIA_ASSERT(index->size(), ==, index_size);

            auto symbols = seq.symbols();

            BufferType buffer(size);

            auto fn = [&](size_t start, size_t end) {
                Values histogramm;

                for (size_t idx = start; idx < end; idx++)
                {
                    size_t symbol = GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
                    histogramm[symbol]++;
                }

                return histogramm;
            };

            size_t at = 0;

            size_t buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
                for (size_t b = 0; b < Blocks; b++)
                {
                    for (size_t c = 0; c < buffer_size; c++)
                    {
                        MEMORIA_ASSERT(index->value(b, c + at), ==, buffer.buffer()[c][b]);
                    }
                }

                at += buffer_size;
            }
        }
        else {
            MEMORIA_ASSERT(seq.has_index(), ==, false);
        }
    }
};


template <typename Seq>
class VLEReindexFn {
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;
    typedef typename Index::Codec                                               Codec;
    typedef typename Index::SizesT                                              SizesT;



    static const size_t BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const size_t ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const size_t Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    using BufferType = packed_seq::IndexBuffer<Values, ValuesPerBranch>;
    static constexpr size_t BatchSize = BufferType::BatchSize;

    static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
                "VLEReindexFn<> can only be used with 2-7-bit sequences");

    static_assert(!FixedSizeElementIndex,
                "VLEReindexFn<> can only be used with PkdVTree<>-indexed sequences ");

public:
    VoidResult reindex(Seq& seq) noexcept
    {
        size_t size = seq.size();

        if (size > ValuesPerBranch)
        {
            Codec codec;

            SizesT length;

            auto symbols = seq.symbols();

            for (size_t b = 0; b < size; b += ValuesPerBranch)
            {
                size_t next = b + ValuesPerBranch;
                size_t max = next <= size ? next : size;

                Values values;

                for (size_t pos = b; pos < max; pos++)
                {
                    size_t symbol = GetBits(symbols, pos * BitsPerSymbol, BitsPerSymbol);
                    values[symbol]++;
                }

                for (size_t c = 0; c < Blocks; c++)
                {
                    length[c] += codec.length(values[c]);
                }
            }

            MEMORIA_TRY_VOID(seq.createIndex(length));

            Index* index = seq.index();

            symbols = seq.symbols();

            BufferType buffer(size);

            auto fn = [&](size_t start, size_t end) {
                Values histogramm;

                for (size_t idx = start; idx < end; idx++)
                {
                    size_t symbol = GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
                    histogramm[symbol]++;
                }

                return histogramm;
            };

            size_t buffer_size;
            SizesT at;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
                MEMORIA_TRY(at_s, index->populate(at, buffer_size, [&](size_t block, size_t idx) {
                    return buffer.buffer()[idx][block];
                }));

                at = at_s;
            }

            return index->reindex();
        }
        else {
            return seq.removeIndex();
        }

        return VoidResult::of();
    }


    void check(const Seq& seq)
    {
        size_t size = seq.size();

        if (size > ValuesPerBranch)
        {
            auto index = seq.index();

            auto symbols = seq.symbols();

            auto fn = [&](size_t start, size_t end) {
                Values histogramm;

                for (size_t idx = start; idx < end; idx++)
                {
                    size_t symbol = GetBits(symbols, idx * BitsPerSymbol, BitsPerSymbol);
                    histogramm[symbol]++;
                }

                return histogramm;
            };

            BufferType buffer(size);
            size_t at = 0;

            size_t buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
                for (size_t b = 0; b < Blocks; b++)
                {
                    for (size_t c = 0; c < buffer_size; c++)
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
            MEMORIA_ASSERT(seq.has_index(), ==, false);
        }
    }
};



template <typename Seq>
class VLEReindex8Fn {
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;
    typedef typename Index::Codec                                               Codec;
    using SizesT = typename Index::SizesT;

    static const size_t BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const size_t ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const size_t Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    using BufferType = packed_seq::IndexBuffer<Values, ValuesPerBranch, 4096>;
    static constexpr size_t BatchSize = BufferType::BatchSize;

    static_assert(BitsPerSymbol == 8,
                "VLEReindex8Fn<> can only be used with 8-bit sequences");

    static_assert(!FixedSizeElementIndex,
                "VLEReindex8Fn<> can only be used with PkdVTree<>-indexed sequences ");

public:
    VoidResult reindex(Seq& seq) noexcept
    {
        size_t size = seq.size();

        if (size > ValuesPerBranch)
        {
            Codec codec;

            SizesT length;

            auto symbols = seq.symbols();

            for (size_t b = 0; b < size; b += ValuesPerBranch)
            {
                size_t next = b + ValuesPerBranch;
                size_t max = next <= size ? next : size;

                Values values;

                for (size_t pos = b; pos < max; pos++)
                {
                    size_t symbol = symbols[pos];
                    values[symbol]++;
                }

                for (size_t c = 0; c < Blocks; c++)
                {
                    length[c] += codec.length(values[c]);
                }
            }

            MEMORIA_TRY_VOID(seq.createIndex(length));

            symbols = seq.symbols();

            Index* index = seq.index();

            BufferType buffer(size);

            SizesT at;

            auto fn = [&](size_t start, size_t end) {
                Values histogramm;

                for (size_t idx = start; idx < end; idx++)
                {
                    size_t symbol = symbols[idx];
                    histogramm[symbol]++;
                }

                return histogramm;
            };

            size_t buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
                MEMORIA_TRY(at_s, index->populate(at, buffer_size, [&](size_t block, size_t idx) {
                    return buffer.buffer()[idx][block];
                }));

                at = at_s;
            }

            return index->reindex();
        }
        else {
            return seq.removeIndex();
        }

        return VoidResult::of();
    }

    void check(const Seq& seq)
    {
        size_t size = seq.size();

        if (size > ValuesPerBranch)
        {
            auto index = seq.index();

            auto symbols = seq.symbols();

            auto fn = [&](size_t start, size_t end) {
                Values histogramm;

                for (size_t idx = start; idx < end; idx++)
                {
                    size_t symbol = symbols[idx];
                    histogramm[symbol]++;
                }

                return histogramm;
            };

            BufferType buffer(size);
            size_t at = 0;

            size_t buffer_size;
            while ((buffer_size = buffer.process(fn)) > 0)
            {
                for (size_t b = 0; b < Blocks; b++)
                {
                    for (size_t c = 0; c < buffer_size; c++)
                    {
                        MEMORIA_ASSERT(index->value(b, c + at), ==, buffer.buffer()[c][b]);
                    }
                }

                at += buffer_size;
            }
        }
        else {
            MEMORIA_ASSERT(seq.has_index(), ==, false);
        }


    }

};



template <typename Seq>
class VLEReindex8BlkFn: public VLEReindex8Fn<Seq> {

    typedef VLEReindex8Fn<Seq>                                                  Base;
    typedef typename Seq::Index                                                 Index;
    typedef typename Index::Values                                              Values;
    typedef typename Index::Codec                                               Codec;

    static const size_t BitsPerSymbol                                              = Seq::BitsPerSymbol;
    static const size_t ValuesPerBranch                                            = Seq::ValuesPerBranch;
    static const size_t Blocks                                                     = Index::Blocks;
    static const bool FixedSizeElementIndex                                     = Index::FixedSizeElement;

    static_assert(BitsPerSymbol == 8,
                "VLEReindex8Fn<> can only be used with 8-bit sequences");

    static_assert(!FixedSizeElementIndex,
                "VLEReindex8Fn<> can only be used with PkdVTree<>-indexed sequences ");

public:
    VoidResult reindex(Seq& seq) noexcept
    {
        size_t size = seq.size();

        if (size <= ValuesPerBranch)
        {
            return seq.removeIndex();
        }
//        else if (size > ValuesPerBranch && size <= 4096)
//        {
//            Codec codec;
//
//            const size_t LineWidth = 4096/ValuesPerBranch;
//
//            uint16_t frequences[LineWidth * 256];
//            memset(frequences, 0, sizeof(frequences));
//
//            size_t length = 0;
//
//            auto symbols = seq.symbols();
//
//            size_t sum = 0;
//
//            size_t block = 0;
//            for (size_t b = 0; b < size; b += ValuesPerBranch, block++)
//            {
//                size_t next = b + ValuesPerBranch;
//                size_t max = next <= size ? next : size;
//
//
//                for (size_t pos = b; pos < max; pos++)
//                {
//                    size_t symbol = symbols[pos];
//                    frequences[symbol * LineWidth + block]++;
//                }
//
//                for (size_t c = 0; c < Blocks; c++)
//                {
//                    uint16_t freq = frequences[c * LineWidth + block];
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
            return Base::reindex(seq);
        }

        return VoidResult::of();
    }
};


}
