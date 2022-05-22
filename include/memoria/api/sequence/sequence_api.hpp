
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

namespace memoria {
    

template <size_t Bps, typename Profile>
struct SequenceChunk {

    using CtrSizeT = ApiProfileCtrSizeT<Profile>;
    using ChunkPtr = IterSharedPtr<SequenceChunk>;
    using RunT = SSRLERun<Bps>;
    using SymbolT = typename RunT::SymbolT;

    virtual ~SequenceChunk() noexcept = default;

    virtual CtrSizeT entry_offset() const = 0;
    virtual CtrSizeT collection_size() const = 0;
    virtual CtrSizeT chunk_offset() const = 0;

    virtual size_t chunk_size() const = 0;
    virtual size_t entry_offset_in_chunk() const = 0;

    virtual const Span<RunT>& runs() const = 0;
    virtual const SymbolT current_symbol() const = 0;

    virtual bool is_before_start() const = 0;
    virtual bool is_after_end() const = 0;

    virtual ChunkPtr next(CtrSizeT num = 1) const = 0;
    virtual ChunkPtr next_chunk() const = 0;

    virtual ChunkPtr prev(CtrSizeT num = 1) const = 0;
    virtual ChunkPtr prev_chunk() const = 0;

    virtual void dump(std::ostream& out = std::cout) const = 0;
};



template <size_t BitsPerSymbol, typename Profile>
class ICtrApi<Sequence<BitsPerSymbol>, Profile>: public CtrReferenceable<Profile>  {
    using Base = CtrReferenceable<Profile>;
public:




    MMA_DECLARE_ICTRAPI();
};



    
}
