
// Copyright 2013 Victor Smirnov
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


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchVariableName)

    using typename Base::BlockID;
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::BranchNodeEntry;
    using typename Base::TreePathT;

public:
    void ctr_update_path(TreePathT& path, size_t level)
    {
        auto& self = the_self();

        if (!path[level]->is_root())
        {
            auto max_entry = self.ctr_get_node_max_keys(path[level]);
            return ctr_update_path(path, level, max_entry);
        }
    }

    void ctr_update_path(TreePathT& path, size_t level, const BranchNodeEntry& entry);

public:


    MEMORIA_V1_DECLARE_NODE_FN(TryRemoveNonLeafNodeEntryFn, try_remove_entries);
    void ctr_remove_branch_node_entry(TreePathT& path, size_t level, size_t idx) ;

    MEMORIA_V1_DECLARE_NODE_FN(UpdateNodeFn, update);

    PkdUpdateStatus ctr_update_branch_node(const TreeNodeConstPtr& node, size_t idx, const BranchNodeEntry& entry);

    bool ctr_update_branch_nodes(TreePathT& path, size_t level, size_t& idx, const BranchNodeEntry& entry);


MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::ctr_remove_branch_node_entry(TreePathT& path, size_t level, size_t start)
{
    auto& self = this->self();

    self.ctr_cow_clone_path(path, level);

    TreeNodeConstPtr node = path[level];
    self.ctr_update_block_guard(node);

    PkdUpdateStatus status = self.branch_dispatcher().dispatch(node.as_mutable(), TryRemoveNonLeafNodeEntryFn(), start, start + 1);
    assert_success(status);

    self.ctr_update_path(path, level);
}


M_PARAMS
PkdUpdateStatus M_TYPE::ctr_update_branch_node(const TreeNodeConstPtr& node, size_t idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    // FIXME Must update node here!!!
    return self.branch_dispatcher().dispatch(node.as_mutable(), UpdateNodeFn(), idx, entry);
}



M_PARAMS
bool M_TYPE::ctr_update_branch_nodes(TreePathT& path, size_t level, size_t& idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    bool updated_next_node = false;

    self.ctr_cow_clone_path(path, level);
    self.ctr_update_block_guard(path[level]);

    auto status1 = self.ctr_update_branch_node(path[level], idx, entry);

    if (!is_success(status1))
    {
        auto size = self.ctr_get_node_size(path[level], 0);
        size_t split_idx = size / 2;

        self.ctr_split_path_raw(path, level, split_idx);

        TreeNodeConstPtr node;

        if (idx < split_idx)
        {
            self.ctr_expect_prev_node(path, level);
        }
        else {
            idx -= split_idx;
            updated_next_node = true;
        }

        node = path[level];

        auto status2 = self.ctr_update_branch_node(node, idx, entry);
        if (!is_success(status2))
        {
            MEMORIA_MAKE_GENERIC_ERROR("Updating entry is too large").do_throw();
        }
    }

    self.ctr_update_path(path, level);

    return updated_next_node;
}



M_PARAMS
void M_TYPE::ctr_update_path(TreePathT& path, size_t level, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    auto parent_idx = self.ctr_get_parent_idx(path, level);

    auto using_next_node = self.ctr_update_branch_nodes(path, level + 1, parent_idx, entry);

    // Locating current child ID in the parent path[level+1].
    auto child_idx = self.ctr_find_child_idx(path[level + 1], path[level]->id());
    if (child_idx < 0)
    {
        // If no child is found in the parent, then we are looking eigher in the right,
        // or in the left parent's siblings.
        if (using_next_node)
        {
            self.ctr_expect_prev_node(path, level + 1);
        }
        else {
            self.ctr_expect_next_node(path, level + 1);
        }

        // This is just a check
        auto child_idx = self.ctr_find_child_idx(path[level + 1], path[level]->id());
        if (child_idx < 0)
        {
            MEMORIA_MAKE_GENERIC_ERROR("ctr_update_path() internal error").do_throw();
        }
    }
}



#undef M_TYPE
#undef M_PARAMS

}
