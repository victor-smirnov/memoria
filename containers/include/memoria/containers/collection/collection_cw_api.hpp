
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


#include <memoria/containers/collection/collection_entry_impl.hpp>
#include <memoria/containers/collection/collection_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/containers/collection/collection_shuttles.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(collection::CtrApiWName)

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

    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    using typename Base::CollectionChunkTypes;


public:

    using typename Base::CollectionChunkT;
    using typename Base::ChunkSharedPtr;

    using typename Base::CollectionChunkImplT;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;


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

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrApiWName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
