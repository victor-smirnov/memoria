
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


#include <memoria/containers/set/set_names.hpp>
#include <memoria/containers/set/set_tools.hpp>
#include <memoria/containers/set/set_api_impl.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(set::CtrApiName)

public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;

    using Key = typename Types::Key;

    using CtrSizeT = typename Types::CtrSizeT;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;

    using Profile   = typename Types::Profile;

public:

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

    Result<CtrSharedPtr<SetIterator<Key, Profile>>> find(KeyView key) const noexcept
    {
        return memoria_static_pointer_cast<SetIterator<Key, Profile>>(self().ctr_set_find(key));
    }

//    bool contains_element(KeyView key) {
//        return false;
//    }

//    bool insert_element(KeyView key) {
//        return false;
//    }

//    bool remove_element(KeyView key) {
//        return self().remove(key);
//    }



    Result<CtrSharedPtr<SetIterator<Key, Profile>>> iterator() const noexcept
    {
        auto iter = self().ctr_begin();
        return memoria_static_pointer_cast<SetIterator<Key, Profile>>(std::move(iter));
    }

    VoidResult append(io::IOVectorProducer& producer) noexcept
    {
        auto& self = this->self();
        auto iter = self.ctr_end();
        MEMORIA_RETURN_IF_ERROR(iter);

        auto res = iter.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
        MEMORIA_RETURN_IF_ERROR(res);

        return VoidResult::of();
    }

    virtual VoidResult prepend(io::IOVectorProducer& producer) noexcept
    {
        auto& self = this->self();
        auto iter = self.ctr_begin();
        MEMORIA_RETURN_IF_ERROR(iter);

        auto res = iter.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
        MEMORIA_RETURN_IF_ERROR(res);

        return VoidResult::of();
    }

    virtual VoidResult insert(KeyView before, io::IOVectorProducer& producer) noexcept
    {
        auto& self = this->self();

        auto iter = self.ctr_set_find(before);
        MEMORIA_RETURN_IF_ERROR(iter);

        if (iter.get()->is_found(before))
        {
            return VoidResult::make_error("Requested key is found. Can't insert enties this way.");
        }
        else {
            auto res = iter.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
            MEMORIA_RETURN_IF_ERROR(res);

            return VoidResult::of();
        }
    }

    /**
     * Returns true if set is already containing the element
     */
    BoolResult insert(KeyView k) noexcept
    {
        auto iter = self().ctr_set_find(k);
        MEMORIA_RETURN_IF_ERROR(iter);

        if (iter.get()->is_found(k))
        {
            return BoolResult::of(true);
        }
        else {
            auto res = iter.get()->insert(k);
            MEMORIA_RETURN_IF_ERROR(res);

            return BoolResult::of(false);
        }
    }

    /**
     * Returns true if the set contained the element
     */
    BoolResult remove(KeyView key) noexcept
    {
        auto iter = self().ctr_set_find(key);
        MEMORIA_RETURN_IF_ERROR(iter);

        if ((!iter.get()->iter_is_end()) && iter.get()->key() == key)
        {
            MEMORIA_RETURN_IF_ERROR_FN(iter.get()->remove());
            return BoolResult::of(true);
        }

        return BoolResult::of(false);
    }

    /**
     * Returns true if the set contains the element
     */
    BoolResult contains(KeyView k) noexcept
    {
        auto iter = self().ctr_set_find(k);
        MEMORIA_RETURN_IF_ERROR(iter);

        return BoolResult::of(iter.get()->is_found(k));
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
