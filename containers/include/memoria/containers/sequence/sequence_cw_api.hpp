
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

MEMORIA_V1_CONTAINER_PART_BEGIN(sequence::CtrWApiName)

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


    //using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    using typename Base::CollectionChunkTypes;

    using CtrInputBuffer = typename Types::CtrInputBuffer;

public:

    using typename Base::ChunkT;
    using typename Base::ChunkPtr;

    using typename Base::SequenceChunkImplT;
    using typename Base::ChunkImplPtr;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using RunT = SSRLERun<BitsPerSymbol>;
    using SymbolT = typename RunT::SymbolT;

    using typename Base::SeqPath;

    ChunkPtr append(CtrBatchInputFn<CtrInputBuffer> producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_seek_entry(self.size());

        auto jj = self.ctr_insert_batch(std::move(iter), producer);
        return memoria_static_pointer_cast<SequenceChunkImplT>(jj);
    }

    ChunkPtr prepend(CtrBatchInputFn<CtrInputBuffer> producer)
    {
        auto& self = this->self();
        auto iter = self.ctr_seek_entry(0);
        auto jj = self.ctr_insert_batch(std::move(iter), producer);
        return memoria_static_pointer_cast<SequenceChunkImplT>(jj);
    }

    ChunkPtr insert(CtrSizeT at, CtrBatchInputFn<CtrInputBuffer> producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_seek_entry(at);

        auto jj = self.ctr_insert_batch(std::move(iter), producer);
        return memoria_static_pointer_cast<SequenceChunkImplT>(jj);
    }




    void remove(CtrSizeT from, CtrSizeT to)
    {
        self().ctr_remove(from, to);
    }

    void remove_from(CtrSizeT from) {
        self().ctr_remove_from(from);
    }

    void remove_up_to(CtrSizeT pos) {
        self().ctr_remove_up_to(pos);
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrWApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
