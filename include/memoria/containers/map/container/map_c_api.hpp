
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

#include <memoria/core/tools/optional.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(map::CtrApiName)

    using Types = typename Base::Types;

    using typename Base::TreeNodePtr;
    using typename Base::IteratorPtr;

    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::Profile;
    using typename Base::ApiProfileT;

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



    void assign_key(KeyView key, ValueView value)
    {
        self().assign(key, value);
    }

    void remove_key(KeyView key)
    {
        self().remove(key);
    }

    IterSharedPtr<MapIterator<Key,Value, ApiProfileT>> iterator() const
    {
        auto iter = self().ctr_begin();
        //return memoria_static_pointer_cast<MapIterator<Key,Value, ApiProfileT>>(std::move(iter));
        return iter;
    }

    virtual IterSharedPtr<MapIterator<Key, Value, ApiProfileT>> find(KeyView key) const
    {
        auto iter = self().ctr_map_find(key);
        //return memoria_static_pointer_cast<MapIterator<Key, Value, ApiProfileT>>(std::move(iter));
        return iter;
    }

    void append(io::IOVectorProducer& producer)
    {
        auto& self = this->self();
        auto iter = self.ctr_end();

        iter.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

    virtual void prepend(io::IOVectorProducer& producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_begin();
        iter.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

    virtual void insert(KeyView before, io::IOVectorProducer& producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_map_find(before);

        if (iter->is_found(before))
        {
            MEMORIA_MAKE_GENERIC_ERROR("Requested key is found. Can't insert enties this way.").do_throw();
        }
        else {
            iter->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
        }
    }

    Optional<Datum<Value>> remove_and_return(KeyView key)
    {
        auto& self = this->self();

        auto iter = self.ctr_map_find(key);

        if (iter->is_found(key))
        {
            auto val = iter->value();
            iter->remove();
            return std::move(val);
        }
        else {
            return Optional<Datum<Value>>{};
        }
    }

    Optional<Datum<Value>> replace_and_return(KeyView key, ValueView value)
    {
        auto& self = this->self();

        auto iter = self.ctr_map_find(key);

        if (iter->is_found(key))
        {
            auto prev = iter->value();
            iter->assign(value);
            return std::move(prev);
        }
        else {
            iter->insert(key, value);
            return Optional<Datum<Value>>{};
        }
    }


    virtual void with_value(
            KeyView key,
            std::function<Optional<Datum<Value>> (Optional<Datum<Value>>)> value_fn
    )
    {
        using OptionalValue = Optional<Datum<Value>>;
        auto& self = this->self();

        auto iter = self.ctr_map_find(key);
        if (iter->is_found(key))
        {
            auto new_value = value_fn(iter->value());
            if (new_value) {
                iter->assign(new_value.get());
            }
            else {
                iter->remove();
            }
        }
        else {
            auto value = value_fn(OptionalValue{});
            if (value) {
                iter->insert(key, value.get());
            }
        }
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(map::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
