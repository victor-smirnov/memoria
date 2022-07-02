
// Copyright 2016 Victor Smirnov
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

#include <memoria/prototypes/bt/shuttles/bt_shuttle_base.hpp>


#include <memoria/containers/set/set_names.hpp>
#include <memoria/containers/set/set_tools.hpp>
#include <memoria/containers/set/set_api_impl.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/api/set/set_api.hpp>

#include <memoria/containers/collection/collection_entry_impl.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(set::CtrRApiName)

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

    using Key = typename Types::Key;

    using CtrSizeT = typename Types::CtrSizeT;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;

    using Profile   = typename Types::Profile;

    using BufferT   = DataTypeBuffer<Key>;
    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

public:

    using CollectionChunkT = CollectionChunk<Key, ApiProfile<Profile>>;
    using ChunkSharedPtr = IterSharedPtr<CollectionChunkT>;

    struct CollectionChunkTypes: Types {
        using CollectionKeyType = Key;
        using ShuttleTypes = typename Base::ShuttleTypes;
    };


    using CollectionChunkImplT = CollectionChunkImpl<CollectionChunkTypes>;

    using EntriesPath = IntList<0, 1>;

    template <typename ShuttleTypes>
    using FindShuttle = bt::FindForwardShuttle<ShuttleTypes, EntriesPath, CollectionChunkImplT>;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;


    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;

    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) {

    }

    IterSharedPtr<CollectionChunkImplT> ctr_set_find(const KeyView& k) const
    {
        return self().ctr_descend(
            TypeTag<CollectionChunkImplT>{},
            bt::ShuttleTag<FindShuttle>{},
            k, 0, SearchType::GE
        );
    }


    ChunkSharedPtr find(KeyView key) const
    {
        return self().ctr_set_find(key);
    }



    /**
     * Returns true if the set contains the element
     */
    bool contains(KeyView k)
    {
        auto iter = self().ctr_set_find(k);
        return iter->is_found(k);
    }


    virtual void read_to(BufferT& buffer, CtrSizeT start, CtrSizeT length) const
    {
        auto& self = this->self();

        auto ii = self.seek_entry(start);

        CtrSizeT cnt{};

        size_t local_cnt;
        while (cnt < length && is_valid_chunk(ii))
        {
            local_cnt = 0;
            CtrSizeT remainder   = length - cnt;
            CtrSizeT values_size = static_cast<CtrSizeT>(ii->keys().size());

            if (values_size <= remainder)
            {
                buffer.append(ii->keys());
                cnt += values_size;
            }
            else {
                buffer.append(ii->keys().first(remainder));
                cnt += remainder;
            }

            if (cnt < length)
            {
                ii = ii->next_chunk();
            }
        }
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrRApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
