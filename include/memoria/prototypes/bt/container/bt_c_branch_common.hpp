
// Copyright 2015 Victor Smirnov
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
#include <memoria/core/container/macros.hpp>
#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchCommonName)
public:
    using typename Base::TreeNodePtr;
    using typename Base::TreePathT;

public:

    VoidResult ctr_create_new_root_block(TreePathT& path) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN(GetNonLeafCapacityFn, capacity);
    Int32Result ctr_get_branch_node_capacity(const TreeNodePtr& node, uint64_t active_streams) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, GetNonLeafCapacityFn(), active_streams);
    }


    MEMORIA_V1_DECLARE_NODE_FN(SplitNodeFn, splitTo);
    VoidResult ctr_split_branch_node(TreeNodePtr& src, TreeNodePtr& tgt, int32_t split_at) noexcept;

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
VoidResult M_TYPE::ctr_split_branch_node(TreeNodePtr& src, TreeNodePtr& tgt, int32_t split_at) noexcept
{
    auto& self = this->self();
    return self.branch_dispatcher().dispatch(src, tgt, SplitNodeFn(), split_at);
}


M_PARAMS
VoidResult M_TYPE::ctr_create_new_root_block(TreePathT& path) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));

    TreeNodePtr root = path.root();

    MEMORIA_TRY(new_root, self.ctr_create_node(root->level() + 1, true, false, root->header().memory_block_size()));

    MEMORIA_TRY(root_active_streams, self.ctr_get_active_streams(root));
    MEMORIA_TRY_VOID(self.ctr_layout_branch_node(new_root, root_active_streams));
    MEMORIA_TRY_VOID(self.ctr_copy_root_metadata(root, new_root));
    MEMORIA_TRY_VOID(self.ctr_root_to_node(root));

    MEMORIA_TRY(max, self.ctr_get_node_max_keys(root));

    path.add_root(new_root);

    MEMORIA_TRY_VOID(self.ctr_insert_to_branch_node(path, new_root->level(), 0, max, root->id()));

    MEMORIA_TRY_VOID(self.ctr_ref_block(root->id()));

    return self.set_root(new_root->id());
}

#undef M_TYPE
#undef M_PARAMS

}
