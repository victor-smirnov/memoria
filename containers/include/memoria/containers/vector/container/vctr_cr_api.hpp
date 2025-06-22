
// Copyright 2013-2025 Victor Smirnov
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

#include <memoria/api/collection/collection_api.hpp>

#include <memoria/containers/collection/collection_entry_impl.hpp>
#include <memoria/containers/collection/collection_shuttles.hpp>

#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/api/vector/vector_api.hpp>


namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(mvector::CtrApiRName)

public:
    using Types = typename Base::Types;


    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::Profile;
    using typename Base::ApiProfileT;

protected:
    using Value = typename Types::Value;
    using ValueDataType = typename Types::ValueDataType;
    using ViewType  = DTTViewType<ValueDataType>;
    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    using BufferT = typename Types::CtrInputBuffer;

    using CtrInputBuffer = typename Types::CtrInputBuffer;

    using CollectionChunkT = CollectionChunk<Value, ApiProfile<Profile>>;
    using ChunkSharedPtr = IterSharedPtr<CollectionChunkT>;

    struct CollectionChunkTypes: Types {
        using CollectionKeyType = Value;
        using ShuttleTypes = typename Base::ShuttleTypes;
    };


    using CollectionChunkImplT = CollectionChunkImpl<CollectionChunkTypes>;

    using EntriesPath = IntList<0, 1>;

    template <typename ShuttleTypes>
    using FindShuttle = bt::FindForwardShuttle<ShuttleTypes, EntriesPath, CollectionChunkImplT>;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

public:

    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;

    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) {

    }


    virtual void read_to(BufferT& buffer, CtrSizeT start, CtrSizeT length) const
    {

    }

    ProfileCtrSizeT<Profile> size() const
    {
        auto sizes = self().sizes();
        return sizes[0];
    }


    DTView<Value> get(CtrSizeT pos) const
    {
        auto& self = this->self();
        auto iter = self.ctr_seek_entry(pos);
        return iter->current_key();
    }

MEMORIA_V1_CONTAINER_PART_END

}
