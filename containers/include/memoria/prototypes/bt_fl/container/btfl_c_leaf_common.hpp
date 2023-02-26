
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


#include <memoria/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt_fl/btfl_tools.hpp>


#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btfl::LeafCommonName)

    using typename Base::TreeNodeConstPtr;
    using typename Base::Position;
    using typename Base::BlockIteratorStatePtr;

    bool ctr_is_at_the_end(const TreeNodeConstPtr& leaf, const Position& pos) const
    {
        auto sizes = self().ctr_get_node_sizes(leaf);
        return pos.sum() >= sizes.sum();
    }

    BlockIteratorStatePtr ctr_remove_range(BlockIteratorStatePtr&& from, BlockIteratorStatePtr&& to)
    {
        auto& self = this->self();
        auto start = from->iter_leafrank();
        auto end   = to->iter_leafrank();

        self.ctr_remove_entries(from->path(), start, to->path(), end, true);
        to->iter_finish_update(end);

        return std::move(to);
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btfl::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
