
// Copyright 2022 Victor Smirnov
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


#include <memoria/containers/sequence/sequence_chunk_impl.hpp>
#include <memoria/containers/sequence/sequence_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/containers/sequence/sequence_shuttles.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(sequence::CtrRApiName)

    using typename Base::ApiProfileT;

public:
    using Types = typename Base::Types;


    using typename Base::TreeNodeConstPtr;
    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::ShuttleTypes;
    using typename Base::TreePathT;


protected:
    static constexpr size_t AlphabetSize  = Types::AlphabetSize;
    static constexpr size_t BitsPerSymbol = Types::BitsPerSymbol;

    using CtrSizeT = typename Types::CtrSizeT;
    using Profile   = typename Types::Profile;


    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    struct CollectionChunkTypes: Types {
        using ShuttleTypes = typename Base::ShuttleTypes;
    };


public:

    using ChunkT = SequenceChunk<AlphabetSize, ApiProfile<Profile>>;
    using ChunkPtr = IterSharedPtr<ChunkT>;

    using SequenceChunkImplT = SequenceChunkImpl<CollectionChunkTypes>;
    using ChunkImplPtr = IterSharedPtr<SequenceChunkImplT>;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using RunT = SSRLERun<BitsPerSymbol>;
    using SymbolT = typename RunT::SymbolT;

    static constexpr size_t Stream = 0;

    using SeqPath = IntList<Stream, 1>;

    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;

    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) {

    }


    ChunkPtr seek_entry(CtrSizeT num) const
    {
        auto& self = this->self();
        return self.ctr_seek_entry(num);
    }

    ChunkImplPtr ctr_seek_entry(CtrSizeT num) const
    {
        auto& self = this->self();
        return self.ctr_descend(
                    TypeTag<SequenceChunkImplT>{},
                    TypeTag<bt::SkipForwardShuttle<ShuttleTypes, Stream, SequenceChunkImplT>>{},
                    num
        );
    }


    ChunkPtr select(SymbolT symbol, CtrSizeT rank, SeqOpType seq_op) const {
        auto& self = this->self();
        return self.ctr_descend(
                    TypeTag<SequenceChunkImplT>{},
                    TypeTag<bt::SelectForwardShuttle<ShuttleTypes, SeqPath, SequenceChunkImplT>>{},
                    rank, symbol, seq_op
        );
    }

    CtrSizeT rank(CtrSizeT pos, SymbolT symbol, SeqOpType seq_op) const
    {
        auto& self = this->self();
        ChunkImplPtr chunk = self.ctr_seek_entry(pos);
        return chunk->rank(symbol, seq_op);
    }




MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrRApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
