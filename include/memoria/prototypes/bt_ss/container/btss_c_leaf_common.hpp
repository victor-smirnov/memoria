
// Copyright 2015-2021 Victor Smirnov
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/prototypes/bt_ss/btss_names.hpp>
#include <memoria/prototypes/bt_ss/btss_batch_input.hpp>


#include <memoria/core/container/macros.hpp>
#include <memoria/api/common/ctr_api_btss.hpp>


#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btss::LeafCommonName)

    using typename Base::TreeNodeConstPtr;
    using typename Base::Position;
    using typename Base::Profile;
    using typename Base::TreePathT;

    using typename Base::CtrSizeT;
    using typename Base::LeafNode;

    using SplitResultT = bt::SplitResult<CtrSizeT>;


    void dump_leafs(CtrSizeT leafs)
    {

    }




    bool ctr_is_at_the_end(const TreeNodeConstPtr& leaf, const Position& pos)
    {
        auto size = self().template ctr_get_leaf_stream_size<0>(leaf);
        return pos[0] >= size;
    }




    SplitResultT split_leaf_in_a_half(TreePathT& path, CtrSizeT target_idx)
    {
        auto& self = this->self();

        auto leaf_sizes = self.ctr_get_leaf_sizes(path.leaf());
        CtrSizeT split_idx = div_2(leaf_sizes[0]);

        self.ctr_split_leaf(path, Position::create(0, split_idx));

        if (target_idx > split_idx)
        {
            self.ctr_expect_next_node(path, 0);
            return SplitResultT(bt::SplitStatus::RIGHT, target_idx - split_idx);
        }
        else {
            return SplitResultT(bt::SplitStatus::LEFT, target_idx);
        }
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
