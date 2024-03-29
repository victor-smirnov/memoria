
// Copyright 2021 Victor Smirnov
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
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/core/tools/result.hpp>

#include <memoria/core/memory/memory.hpp>

#include <memoria/core/datatypes/datatypes.hpp>

#include "ssrleseq_tools.hpp"

#include <vector>

namespace memoria {
namespace ssrleseq {

template <typename Seq>
class ReindexFn {
    using SizeIndexSO   = typename Seq::SizeIndexSO;
    using SumIndexSO    = typename Seq::SumIndexSO;
    using SymbolsRunT   = typename Seq::SymbolsRunT;
    using RunTraits     = typename Seq::RunTraits;
    using SeqSizeT      = typename Seq::SeqSizeT;
    using RunSizeT      = typename Seq::RunSizeT;
    using SymbolT       = typename Seq::SymbolT;

    using SizeDataType = typename SizeIndexSO::DataType;
    using SumDataType  = typename SumIndexSO::DataType;

    static const SymbolT AlphabetSize   = Seq::AlphabetSize;
    static const size_t  AtomsPerBlock  = Seq::AtomsPerBlock;

public:
    void reindex(Seq& seq, Optional<Span<SymbolsRunT>> runs, bool compactify) noexcept
    {
        Span<const SymbolsRunT> syms_span;
        std::vector<SymbolsRunT> syms;

        size_t symbols_block_size_atoms;
        if (runs) {
            symbols_block_size_atoms = RunTraits::compute_size(runs.value());
            syms_span = runs.value();
        }
        else {
            syms = seq.iterator().as_vector();
            if (compactify) {
                RunTraits::compactify_runs(syms);
            }
            symbols_block_size_atoms = RunTraits::compute_size(syms);
            syms_span = to_const_span(syms);
        }

        auto meta = seq.data()->metadata();

        if (meta->code_units() != symbols_block_size_atoms)
        {
            meta->set_code_units(symbols_block_size_atoms);

            seq.data()->resize_block(Seq::SYMBOLS, symbols_block_size_atoms * sizeof(typename Seq::CodeUnitT));
            RunTraits::write_segments_to(syms_span, seq.data()->symbols_block(), 0);
        }

        if (symbols_block_size_atoms > AtomsPerBlock)
        {
            using SumDTView = DTTViewType<SumDataType>;
            using SizeDTView = DTTViewType<SizeDataType>;

            std::vector<
                    IterSharedPtr<std::vector<SumDTView>>
            > sums_bufs(AlphabetSize);

            for (size_t c = 0; c < AlphabetSize; c++) {
                sums_bufs[c] = TL_allocate_shared<std::vector<SizeDTView>>();
            }


            IterSharedPtr<std::vector<SizeDTView>> sizes_buf = TL_allocate_shared<std::vector<SizeDTView>>();

            SeqSizeT symbols_total_{};
            typename SumIndexSO::Values sums(SeqSizeT{});

            size_t next_block_start_idx = AtomsPerBlock;

            size_t cnt{};
            size_t block_cnt{};

            auto push_indexes = [&]() {
                cnt++;
                for (size_t c = 0; c < AlphabetSize; c++) {
                    sums_bufs[c]->push_back(sums[c]);
                }
                sizes_buf->push_back(symbols_total_);


                symbols_total_ = SeqSizeT{};
                sums = typename SumIndexSO::Values(SeqSizeT{});

                next_block_start_idx += AtomsPerBlock;
                block_cnt = 0;
            };

            for (auto ii = seq.iterator(); !ii.is_eos();)
            {
                auto run = ii.get();

                if (run)
                {
                    symbols_total_ += run.full_run_length();

                    for (size_t s = 0; s < run.pattern_length(); s++)
                    {
                        auto sym = run.symbol(s);
                        sums[sym] += run.run_length();
                    }

                    block_cnt++;

                    ii.next();
                }
                else if (run.is_padding())
                {
                    ii.next(run.run_length());
                }
                else {
                    push_indexes();
                    break;
                }

                if (ii.idx() == next_block_start_idx) {
                    push_indexes();
                }
            }

            if (block_cnt) {
                push_indexes();
            }

            seq.data()->createIndex(sizes_buf->size());

            seq.size_index().insert_from_fn(0, sizes_buf->size(), [&](size_t, size_t row){
                return sizes_buf->operator[](row);
            });

            seq.size_index().reindex();

            seq.sum_index().insert_from_fn(0, sizes_buf->size(), [&](size_t column, size_t row){
                return sums_bufs[column]->operator[](row);
            });
            return seq.sum_index().reindex();
        }
        else {
            return seq.data()->removeIndex();
        }
    }

    void check(const Seq& seq)
    {
        auto ii = seq.iterator();

        auto symbols_block_size = ii.span().size();

        if (false && symbols_block_size > AtomsPerBlock)
        {
            auto size_index = seq.size_index();
            size_index.check();

            auto sum_index  = seq.sum_index();
            sum_index.check();

            SeqSizeT symbols_total_{};
            typename SumIndexSO::Values sums(SeqSizeT{});

            size_t next_block_start_idx = AtomsPerBlock;

            size_t blk_idx{};
            size_t block_cnt{};
            auto check_indexes = [&]() {
                if (sum_index.access(blk_idx) != sums) {
                    MEMORIA_MAKE_GENERIC_ERROR("SSRLESeq SumIndex check error: blk:{}, index:{}, seq:{}",
                                                      blk_idx,
                                                      sum_index.access(blk_idx),
                                                      sums).do_throw();
                }

                typename SizeIndexSO::Values sizes(symbols_total_);

                if (size_index.access(blk_idx) != sizes) {
                    MEMORIA_MAKE_GENERIC_ERROR("SSRLESeq SizeIndex check error: blk:{}, index:{}, seq:{}",
                                                      blk_idx,
                                                      size_index.access(blk_idx),
                                                      sizes).do_throw();
                }

                symbols_total_ = SeqSizeT{};
                sums = typename SumIndexSO::Values(SeqSizeT{});
                next_block_start_idx += AtomsPerBlock;
                blk_idx++;
                block_cnt = 0;
            };

            while (!ii.is_eos())
            {
                auto run = ii.get();
                if (run)
                {
                    symbols_total_ += run.full_run_length();

                    for (size_t s = 0; s < run.pattern_length(); s++)
                    {
                        auto sym = run.symbol(s);
                        sums[sym] += run.run_length();
                    }

                    block_cnt++;

                    ii.next();                    
                }
                else if (run.is_padding())
                {
                    ii.next(run.run_length());
                }
                else {
                    check_indexes();
                    break;
                }

                if (ii.idx() == next_block_start_idx) {
                    check_indexes();
                }
            }

            if (block_cnt) {
                check_indexes();
            }
        }
    }
};


}}
