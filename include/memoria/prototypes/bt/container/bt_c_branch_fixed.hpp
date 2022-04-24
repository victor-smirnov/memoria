
// Copyright 2011 Victor Smirnov
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
#include <memoria/core/tools/result.hpp>

#include <vector>

namespace memoria {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchFixedName)

public:
    using typename Base::BlockID;
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::BranchNodeEntry;
    using typename Base::TreePathT;

    using SplitFn = std::function<void (const TreeNodePtr&, const TreeNodePtr&)>;

public:

    void ctr_update_path(TreePathT& path, size_t level)
    {
        auto& self = this->self();

        if (!path[level]->is_root())
        {
            auto max_entry = self.ctr_get_node_max_keys(path[level]);
            return ctr_update_path(path, level, max_entry);
        }
    }

    void ctr_update_path(TreePathT& path, size_t level, const BranchNodeEntry& entry);


public:

    MEMORIA_V1_DECLARE_NODE_FN(RemoveNonLeafNodeEntryFn, remove_entries);
    void ctr_remove_branch_node_entry(TreePathT& path, size_t level, int32_t idx) ;

    MEMORIA_V1_DECLARE_NODE_FN(UpdateNodeFn,  commit_update);
    void ctr_update_branch_node(
            const TreeNodePtr& node,
            int32_t idx,
            const BranchNodeEntry& entry
    );

    void ctr_update_branch_node(
            const TreeNodeConstPtr& node,
            int32_t idx,
            const BranchNodeEntry& entry
    );

    void ctr_update_branch_nodes(
            TreePathT& path,
            size_t level,
            int32_t& idx,
            const BranchNodeEntry& entry
    );

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::ctr_update_branch_node(const TreeNodePtr& node, int32_t idx, const BranchNodeEntry& keys)
{
    self().ctr_update_block_guard(node);

    auto update_state = self().ctr_make_branch_update_state();
    self().branch_dispatcher().dispatch(node, UpdateNodeFn(), idx, keys, update_state);
}


M_PARAMS
void M_TYPE::ctr_update_branch_node(const TreeNodeConstPtr& node, int32_t idx, const BranchNodeEntry& keys)
{
    self().ctr_update_block_guard(node);

    auto update_state = self().ctr_make_branch_update_state(node);
    self().branch_dispatcher().dispatch(node.as_mutable(), UpdateNodeFn(), idx, keys, update_state);
}




M_PARAMS
void M_TYPE::ctr_update_branch_nodes(
        TreePathT& path,
        size_t level,
        int32_t& idx,
        const BranchNodeEntry& entry
)
{
    auto& self = this->self();

    self.ctr_cow_clone_path(path, level);

    TreeNodeConstPtr tmp = path[level];

    self.ctr_update_branch_node(tmp, idx, entry);

    while(!tmp->is_root())
    {
        auto max = self.ctr_get_node_max_keys(tmp);

        auto parent = self.ctr_get_node_parent_for_update(path, level);

        auto parent_idx = self.ctr_get_child_idx(parent.as_immutable(), tmp->id());

        tmp = parent.as_immutable();

        self.ctr_update_branch_node(tmp, parent_idx, max);

        level++;
    }
}


M_PARAMS
void M_TYPE::ctr_update_path(TreePathT& path, size_t level, const BranchNodeEntry& entry)
{
    auto& self = this->self();
    auto parent_idx = self.ctr_get_parent_idx(path, level);
    self.ctr_update_branch_nodes(path, level + 1, parent_idx, entry);
}


M_PARAMS
void M_TYPE::ctr_remove_branch_node_entry(TreePathT& path, size_t level, int32_t start)
{
    auto& self = this->self();

    self.ctr_cow_clone_path(path, level);

    TreeNodeConstPtr node = path[level];
    self.ctr_update_block_guard(node);

    self.branch_dispatcher().dispatch(node.as_mutable(), RemoveNonLeafNodeEntryFn(), start, start + 1);
    self.ctr_update_path(path, level);
}



#undef M_TYPE
#undef M_PARAMS

}
