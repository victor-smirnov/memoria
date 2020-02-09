
// Copyright 2014 Victor Smirnov
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


#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>
#include <memoria/containers/map/map_api_impl.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(map::CtrApiName)

    using Types = typename Base::Types;

    using typename Base::NodeBaseG;
    using typename Base::IteratorPtr;

    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::Profile;

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;


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



    VoidResult assign_key(KeyView key, ValueView value) noexcept
    {
        MEMORIA_TRY_VOID(self().assign(key, value));
        return VoidResult::of();
    }

    VoidResult remove_key(KeyView key) noexcept
    {
        MEMORIA_TRY_VOID(self().remove(key));
        return VoidResult::of();
    }

    Result<CtrSharedPtr<MapIterator<Key,Value, Profile>>> iterator() const noexcept
    {
        auto iter = self().ctr_begin();
        return memoria_static_pointer_cast<MapIterator<Key,Value, Profile>>(std::move(iter));
    }

    virtual Result<CtrSharedPtr<MapIterator<Key, Value, Profile>>> find(KeyView key) const noexcept
    {
        auto iter = self().ctr_map_find(key);
        MEMORIA_RETURN_IF_ERROR(iter);

        return memoria_static_pointer_cast<MapIterator<Key, Value, Profile>>(std::move(iter));
    }

    VoidResult append(io::IOVectorProducer& producer) noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(iter, self.ctr_end());

        MEMORIA_TRY_VOID(iter.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max()));

        return VoidResult::of();
    }

    virtual VoidResult prepend(io::IOVectorProducer& producer) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(iter, self.ctr_begin());
        MEMORIA_TRY_VOID(iter.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max()));

        return VoidResult::of();
    }

    virtual VoidResult insert(KeyView before, io::IOVectorProducer& producer) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(iter, self.ctr_map_find(before));

        if (iter->is_found(before))
        {
            return VoidResult::make_error("Requested key is found. Can't insert enties this way.");
        }
        else {
            MEMORIA_TRY_VOID(iter->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max()));
            return VoidResult::of();
        }
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(map::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
