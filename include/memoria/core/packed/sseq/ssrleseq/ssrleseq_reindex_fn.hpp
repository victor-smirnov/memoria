
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

#include "ssrleseq_tools.hpp"

#include <vector>

namespace memoria {
namespace ssrleseq {

template <typename Seq>
class ReindexFn {
    using SizeIndex     = typename Seq::SizeIndex;
    using SumIndex      = typename Seq::SumIndex;
    using SymbolsRunT   = typename Seq::SymbolsRunT;
    using RunTraits     = typename Seq::RunTraits;
    using SeqSizeT      = typename Seq::SeqSizeT;
    using RunSizeT      = typename Seq::RunSizeT;
    using SymbolT       = typename Seq::SymbolT;

    static const SymbolT AlphabetSize   = Seq::AlphabetSize;
    static const size_t  BytesPerBlock  = Seq::BytesPerBlock;
    static const size_t  AtomsPerBlock  = Seq::AtomsPerBlock;

public:
    VoidResult reindex(Seq& seq, Optional<Span<SymbolsRunT>> runs, bool compactify) noexcept
    {
        Span<const SymbolsRunT> syms_span;
        std::vector<SymbolsRunT> syms;

        size_t symbols_block_size_atoms;
        if (runs) {
            symbols_block_size_atoms = RunTraits::compute_size(runs.get());
            syms_span = runs.get();
        }
        else {
            syms = seq.iterator().as_vector();
            if (compactify) {
                RunTraits::compactify_runs(syms);
            }
            symbols_block_size_atoms = RunTraits::compute_size(syms);
            syms_span = to_const_span(syms);
        }

        auto meta = seq.metadata();

        if (meta->data_size() != symbols_block_size_atoms)
        {
            meta->data_size() = symbols_block_size_atoms;

            MEMORIA_TRY_VOID(seq.resizeBlock(Seq::SYMBOLS, symbols_block_size_atoms * sizeof(typename Seq::CodeUnitT)));
            RunTraits::write_segments_to(syms_span, seq.symbols(), 0);
        }

        if (symbols_block_size_atoms > AtomsPerBlock)
        {
            size_t symbols_blocks = seq.divUp(symbols_block_size_atoms, AtomsPerBlock);
            MEMORIA_TRY_VOID(seq.createIndex(symbols_blocks));

            auto size_index = seq.size_index();
            auto sum_index  = seq.sum_index();

            SeqSizeT symbols_total_{};
            typename Seq::SumIndex::Values sums(SeqSizeT{});

            size_t next_block_start_idx = AtomsPerBlock;

            size_t cnt{};
            size_t block_cnt{};

            auto push_indexes = [&]() -> VoidResult {
                cnt++;
                MEMORIA_TRY_VOID(sum_index->append(sums));

                typename Seq::SizeIndex::Values sizes(symbols_total_);
                symbols_total_ = SeqSizeT{};
                sums = typename Seq::SumIndex::Values(SeqSizeT{});
                next_block_start_idx += AtomsPerBlock;
                block_cnt = 0;

                return size_index->append(sizes);
            };

            std::vector<SymbolsRunT> vv = seq.iterator().as_vector();

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
                    MEMORIA_TRY_VOID(push_indexes());
                    break;
                }

                if (ii.idx() == next_block_start_idx) {
                    MEMORIA_TRY_VOID(push_indexes());
                }
            }

            if (block_cnt) {
                MEMORIA_TRY_VOID(push_indexes());
            }

            MEMORIA_TRY_VOID(size_index->reindex());
            return sum_index->reindex();
        }
        else {
            return seq.removeIndex();
        }

        return VoidResult::of();
    }

    VoidResult check(const Seq& seq) noexcept
    {
        auto ii = seq.iterator();

        auto symbols_block_size = ii.span().size();

        if (symbols_block_size > AtomsPerBlock)
        {
            auto size_index = seq.size_index();
            MEMORIA_TRY_VOID(size_index->check());

            auto sum_index  = seq.sum_index();
            MEMORIA_TRY_VOID(sum_index->check());

            SeqSizeT symbols_total_{};
            typename Seq::SumIndex::Values sums(SeqSizeT{});

            size_t next_block_start_idx = AtomsPerBlock;

            size_t blk_idx{};
            size_t block_cnt{};
            auto check_indexes = [&]() -> VoidResult {
                if (sum_index->access(blk_idx) != sums) {
                    return MEMORIA_MAKE_GENERIC_ERROR("SSRLESeq SumIndex check error: blk:{}, idx:{}, sum:{}",
                                                      blk_idx,
                                                      sum_index->access(blk_idx),
                                                      sums);
                }

                typename Seq::SizeIndex::Values sizes(symbols_total_);

                if (size_index->access(blk_idx) != sizes) {
                    return MEMORIA_MAKE_GENERIC_ERROR("SSRLESeq SizeIndex check error: blk:{}, idx:{}, sum:{}",
                                                      blk_idx,
                                                      size_index->access(blk_idx),
                                                      sizes);
                }

                symbols_total_ = SeqSizeT{};
                sums = typename Seq::SumIndex::Values(SeqSizeT{});
                next_block_start_idx += AtomsPerBlock;
                blk_idx++;
                block_cnt = 0;

                return VoidResult::of();
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
                    MEMORIA_TRY_VOID(check_indexes());
                    break;
                }

                if (ii.idx() == next_block_start_idx) {
                    MEMORIA_TRY_VOID(check_indexes());
                }
            }

            if (block_cnt) {
                MEMORIA_TRY_VOID(check_indexes());
            }
        }

        return VoidResult::of();
    }
};


}}
