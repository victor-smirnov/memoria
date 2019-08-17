
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


#include <memoria/v1/containers/set/set_names.hpp>
#include <memoria/v1/containers/set/set_tools.hpp>
#include <memoria/v1/containers/set/set_api_impl.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(set::CtrApiName)

public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;

    using Key = typename Types::Key;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using KeyV      = typename DataTypeTraits<Key>::ValueType;

    using Profile   = typename Types::Profile;

public:

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    CtrSharedPtr<BTSSIterator<Profile>> find_element_raw(KeyView key)
    {
        return self().find(key);
    }

    bool contains_element(KeyView key) {
        return false;
    }

    bool insert_element(KeyView key) {
        return false;
    }

    bool remove_element(KeyView key) {
        return self().remove(key);
    }



    CtrSharedPtr<SetIterator<Key>> iterator()
    {
        auto iter = self().begin();
        return ctr_make_shared<SetIteratorImpl<Key, IteratorPtr>>(iter);
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

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}}