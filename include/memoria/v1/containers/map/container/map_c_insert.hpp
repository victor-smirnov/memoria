
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
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(map::CtrInsertName)

    using Types = typename Base::Types;

    using typename Base::NodeBaseG;
    using typename Base::IteratorPtr;

    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;

    using Key = typename Types::Key;
    using Value = typename Types::Value;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

//    using CtrSizeT = typename Types::CtrSizeT;

//    CtrSizeT size() const {
//      return self().sizes()[0];
//    }

    template <typename T>
    IteratorPtr find(T&& k)
    {
        return self().template find_ge<IntList<0, 0, 1>>(0, k);
    }

    template <typename K, typename V>
    IteratorPtr assign(K&& key, V&& value)
    {
        auto iter = self().find(key);

        if (iter->is_found(key))
        {
            iter->assign(value);
        }
        else {
            iter->insert_(key, value);
        }

        return iter;
    }

    template <typename T>
    bool remove(T&& k)
    {
        auto iter = find(k);

        if (iter->key() == k)
        {
            iter->remove();
            return true;
        }
        else {
            return false;
        }
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(map::CtrInsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}}
