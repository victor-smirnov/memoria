
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


#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(map::CtrInsertMaxName)

public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;

    using Key = typename Types::Key;
    using Value = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    static const int32_t Streams = Types::Streams;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

public:

    Result<IteratorPtr> ctr_map_find(const KeyView& k) const noexcept
    {
        return self().template ctr_find_max_ge<IntList<0, 1>>(0, k);
    }


    Result<bool> remove(const KeyView& k) noexcept
    {
        MEMORIA_TRY(iter, self().ctr_map_find(k));

        if (iter->is_found(k))
        {
            MEMORIA_TRY_VOID(iter->remove());
            return BoolResult::of(true);
        }
        else {
            return BoolResult::of(false);
        }
    }

    Result<IteratorPtr> assign(const KeyView& key, const ValueView& value)
    {
        MEMORIA_TRY(iter, self().ctr_map_find(key));

        if (iter->is_found(key))
        {
            MEMORIA_TRY_VOID(iter.get()->assign(value));
        }
        else {
            MEMORIA_TRY_VOID(iter.get()->insert(key, value));
        }

        return iter_result;
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(map::CtrInsertMaxName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
