
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

#include <memoria/prototypes/bt/shuttles/bt_find_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_skip_shuttle.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(collection::CtrApiName)

    using typename Base::ApiProfileT;

public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

    using typename Base::TreeNodeConstPtr;
    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::ShuttleTypes;
    using typename Base::TreePathT;
    using typename Base::NodeChain;



protected:
    using Key = typename Types::Key;

    using CtrSizeT = typename Types::CtrSizeT;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;

    using Profile   = typename Types::Profile;

    using BufferT   = DataTypeBuffer<Key>;
    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    struct CollectionEntryTypes: Types {
        using CollectionKeyType = Key;
        using ShuttleTypes = typename Base::ShuttleTypes;
    };


public:

    using CollectionEntryT = CollectionEntry<Key, ApiProfile<Profile>>;
    using EntrySharedPtr = IterSharedPtr<CollectionEntryT>;

    using CollectionEntryImplT = CollectionEntryImpl<CollectionEntryTypes>;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;


    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;

    virtual EntrySharedPtr seek_entry(CtrSizeT num) const
    {
        auto& self = this->self();
        return self.ctr_descend(
                    TypeTag<CollectionEntryImplT>{},
                    TypeTag<bt::SkipForwardShuttle<ShuttleTypes, 0, CollectionEntryImplT>>{},
                    num
        );
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
