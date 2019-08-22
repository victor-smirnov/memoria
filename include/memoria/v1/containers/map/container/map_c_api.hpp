
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


#include <memoria/v1/containers/map/map_names.hpp>
#include <memoria/v1/containers/map/map_tools.hpp>
#include <memoria/v1/containers/map/map_api_impl.hpp>

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(map::CtrApiName)

    using Types = typename Base::Types;

    using typename Base::NodeBaseG;
    using typename Base::IteratorPtr;

    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;


    int64_t map_size() const {
        return self().size();
    }

    void assign_key(KeyView key, ValueView value) {
        self().assign(key, value);
    }

    void remove_key(KeyView key) {
        self().remove(key);
    }

    CtrSharedPtr<MapIterator<Key,Value>> iterator()
    {
        auto iter = self().begin();
        return ctr_make_shared<MapIteratorImpl<Key,Value, IteratorPtr>>(iter);
    }

    void append_entries(io::IOVectorProducer& producer)
    {
        auto& self = this->self();
        auto iter = self.end();

        iter->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

    virtual void prepend_entries(io::IOVectorProducer& producer)
    {
        auto& self = this->self();
        auto iter = self.begin();

        iter->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

    virtual void insert_entries(KeyView before, io::IOVectorProducer& producer)
    {
        auto& self = this->self();

        auto iter = self.find(before);

        if (iter->is_found(before))
        {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Requested key is found. Can't insert enties this way.");
        }
        else {
            iter->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
        }
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(map::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}}
