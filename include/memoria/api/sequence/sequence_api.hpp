
// Copyright 2017 Victor Smirnov
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

#include <memoria/api/common/ctr_api_btss.hpp>
#include <memoria/core/ssrle/ssrle.hpp>

#include <memoria/api/sequence/sequence_api_factory.hpp>

#include <memoria/core/tools/bitmap.hpp>

namespace memoria {
    

template <size_t AlphabetSize, typename Profile>
struct SequenceChunk: ChunkIteratorBase<SequenceChunk<AlphabetSize, Profile>, Profile> {

    using Base = ChunkIteratorBase<SequenceChunk<AlphabetSize, Profile>, Profile>;

    using typename Base::CtrSizeT;
    using typename Base::ChunkPtr;

    static constexpr size_t BitsPerSymbol = BitsPerSymbolConstexpr(AlphabetSize);

    using RunT = SSRLERun<BitsPerSymbol>;
    using SymbolT = typename RunT::SymbolT;

    virtual const Span<const RunT>& runs() const = 0;
    virtual SymbolT current_symbol() const = 0;

    virtual CtrSizeT rank(SymbolT symbol, SeqOpType seq_op) const = 0;

    virtual ChunkPtr select_fw(SymbolT symbol, CtrSizeT rank, SeqOpType seq_op) const = 0;
    virtual ChunkPtr select_bw(SymbolT symbol, CtrSizeT rank, SeqOpType seq_op) const = 0;

    virtual CtrSizeT read_to(CtrSizeT len, std::vector<RunT>& sink) const = 0;
};



template <size_t AlphabetSize, typename Profile>
struct ICtrApi<Sequence<AlphabetSize>, Profile>: public CtrReferenceable<Profile>  {
    using Base = CtrReferenceable<Profile>;
    using ApiTypes  = ICtrApiTypes<Sequence<AlphabetSize>, Profile>;

    static constexpr size_t BitsPerSymbol = BitsPerSymbolConstexpr(AlphabetSize);

    using RunT = SSRLERun<BitsPerSymbol>;
    using SymbolT = typename RunT::SymbolT;
    using CtrSizeT = ApiProfileCtrSizeT<Profile>;
    using ChunkPtr = IterSharedPtr<SequenceChunk<AlphabetSize, Profile>>;

    using CtrInputBuffer = typename ApiTypes::CtrInputBuffer;

    virtual ChunkPtr first_entry() const {
        return seek_entry(CtrSizeT{});
    }

    virtual ChunkPtr seek_entry(CtrSizeT pos) const = 0;
    virtual CtrSizeT size() const = 0;

    virtual ChunkPtr last_entry() const
    {
        CtrSizeT size = this->size();
        if (size) {
            return seek_entry(size - 1);
        }
        else {
            return seek_entry(0);
        }
    }

    virtual ChunkPtr select(SymbolT symbol, CtrSizeT rank, SeqOpType seq_op) const = 0;
    virtual CtrSizeT rank(CtrSizeT pos, SymbolT symbol, SeqOpType seq_op) const = 0;

    virtual CtrSizeT rank(CtrSizeT start, CtrSizeT end, SymbolT symbol, SeqOpType seq_op) const {
        return rank(end, symbol, seq_op) - rank(start, symbol, seq_op);
    }


    virtual ChunkPtr insert(CtrSizeT at, Span<const RunT> runs) {
        size_t runs_per_block = 1024;
        size_t start {};

        return insert(at, [&](auto& ss) {
            size_t remainder = runs.size() - start;
            size_t limit = remainder > runs_per_block ? runs_per_block : remainder;

            if (limit) {
                ss.append(runs.subspan(start, limit));
            }

            start += limit;

            return remainder <= runs_per_block;
        });
    }


    virtual ChunkPtr insert(CtrSizeT at, CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API
    virtual ChunkPtr append(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API
    virtual ChunkPtr prepend(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    virtual void remove(CtrSizeT from, CtrSizeT to) MEMORIA_READ_ONLY_API
    virtual void remove_from(CtrSizeT from) MEMORIA_READ_ONLY_API
    virtual void remove_up_to(CtrSizeT to) MEMORIA_READ_ONLY_API

    virtual std::vector<RunT> read(CtrSizeT start, CtrSizeT len) const
    {
        std::vector<RunT> runs;
        seek_entry(start)->read_to(len, runs);
        return runs;
    }

    virtual std::vector<RunT> read(CtrSizeT start) const
    {
        std::vector<RunT> runs;
        seek_entry(start)->read_to(size() - start, runs);
        return runs;
    }

    MMA_DECLARE_ICTRAPI();
};



    
}
