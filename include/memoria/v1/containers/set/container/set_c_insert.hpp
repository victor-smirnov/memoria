
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
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(set::CtrInsertName)

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

public:


    IteratorPtr find(const Key& k)
    {
        return self().template find_max_ge<IntList<0, 1>>(0, k);
    }


    bool remove(const Key& k)
    {
        auto iter = find(k);

        if (iter->is_found(k))
        {
            iter->remove();
            return true;
        }
        else {
            return false;
        }
    }

    /**
     * Returns true if the set contains the element
     */

    bool contains(const Key& k)
    {
        auto iter = find(k);
        return iter->is_found(k);
    }


    /**
     * Returns true if set is already containing the element
     */
    bool insert_key(const Key& k)
    {
    	auto iter = find(k);

        if (iter->is_found(k))
        {
        	return true;
        }
        else {
            iter->insert(k);
            return false;
        }
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrRemoveName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}}
