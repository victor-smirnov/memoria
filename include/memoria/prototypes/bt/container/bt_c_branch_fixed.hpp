
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
    MEMORIA_V1_DECLARE_NODE_FN(InsertFn, commit_insert);
    void ctr_insert_to_branch_node(
            TreePathT& path,
            size_t level,
            int32_t idx,
            const BranchNodeEntry& keys,
            const BlockID& id
    );

    void ctr_split_path(
            TreePathT& path,
            size_t level,
            int32_t split_at
    );

    void ctr_split_path_raw(
            TreePathT& path,
            size_t level,
            int32_t split_at
    );

    void ctr_split_node(
            TreePathT& path,
            size_t level,
            const SplitFn& split_fn
    )
    {
        self().ctr_split_node_raw(path, level, split_fn);
        return self().ctr_expect_prev_node(path, level);
    }

    void ctr_split_node_raw(
            TreePathT& path,
            size_t level,
            const SplitFn& split_fn
    );

    MEMORIA_V1_DECLARE_NODE_FN(UpdateNodeFn,  commit_update);
    bool ctr_update_branch_node(
            const TreeNodePtr& node,
            int32_t idx,
            const BranchNodeEntry& entry
    );

    bool ctr_update_branch_node(
            const TreeNodeConstPtr& node,
            int32_t idx,
            const BranchNodeEntry& entry
    );

    bool ctr_update_branch_nodes(
            TreePathT& path,
            size_t level,
            int32_t& idx,
            const BranchNodeEntry& entry
    );





    MEMORIA_V1_DECLARE_NODE2_FN(CanMergeFn, prepare_merge_with);
    bool ctr_can_merge_nodes(const TreeNodeConstPtr& tgt, const TreeNodeConstPtr& src)
    {
        auto update_state = self().make_branch_update_state();
        return true;//isSuccess(self().node_dispatcher().dispatch(src, tgt, CanMergeFn(), update_state));
    }


    MEMORIA_V1_DECLARE_NODE_FN(MergeNodesFn, commit_merge_with);
    void ctr_do_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level);
    bool ctr_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level, bool only_if_same_parent = false);



MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::ctr_insert_to_branch_node(
        TreePathT& path,
        size_t level,
        int32_t idx,
        const BranchNodeEntry& keys,
        const BlockID& id
)
{
    auto& self = this->self();

    self.ctr_cow_clone_path(path, level);

    TreeNodeConstPtr node = path[level];
    self.ctr_update_block_guard(node);

    auto update_state = self.make_branch_update_state();
    self.branch_dispatcher().dispatch(node.as_mutable(), InsertFn(), idx, keys, id, update_state);

    if (!node->is_root())
    {
        auto parent = self.ctr_get_node_parent_for_update(path, level);

        auto max = self.ctr_get_node_max_keys(node);
        auto parent_idx = self.ctr_get_child_idx(parent.as_immutable(), node->id());

        self.ctr_update_branch_nodes(path, level + 1, parent_idx, max);
    }
}

M_PARAMS
void M_TYPE::ctr_split_node_raw(
        TreePathT& path,
        size_t level,
        const SplitFn& split_fn
)
{
    auto& self = this->self();

    if (level + 1 == path.size())
    {
        self.ctr_create_new_root_block(path);
    }

    TreeNodeConstPtr left_node = path[level];

    auto right_node = self.ctr_create_node(level, false, left_node->is_leaf(), left_node->header().memory_block_size());

    split_fn(left_node.as_mutable(), right_node);

    auto left_max = self.ctr_get_node_max_keys(left_node);
    auto right_max = self.ctr_get_node_max_keys(right_node.as_immutable());

    auto parent_idx = self.ctr_get_child_idx(path[level + 1], left_node->id());

    self.ctr_update_branch_nodes(path, level + 1, parent_idx, left_max);

    auto capacity = self.ctr_get_branch_node_capacity(path[level + 1], -1ull);

    if (capacity > 0)
    {
        self.ctr_insert_to_branch_node(path, level + 1, parent_idx + 1, right_max, right_node->id());
    }
    else {
        auto parent_size = self.ctr_get_node_size(path[level + 1], 0);
        int32_t parent_split_idx = parent_size / 2;

        ctr_split_path_raw(path, level + 1, parent_split_idx);

        if (parent_idx < parent_split_idx)
        {
            TreePathT left_path(path, level + 1);
            self.ctr_expect_prev_node(left_path, level + 1);


            self.ctr_assign_path_nodes(
                            left_path,
                            path,
                            level + 1
            );

        }
        else {
            parent_idx -= parent_split_idx;
        }


        self.ctr_insert_to_branch_node(
                    path,
                    level + 1,
                    parent_idx + 1,
                    right_max,
                    right_node->id()
        );
    }

    path[level] = right_node.as_immutable();

    self.ctr_ref_block(right_node->id());
}


M_PARAMS
void M_TYPE::ctr_split_path(
        TreePathT& path,
        size_t level,
        int32_t split_at
)
{
    auto& self = this->self();

    return ctr_split_node(
                path,
                level,
                [&self, split_at](const TreeNodePtr& left, const TreeNodePtr& right)
    {
        self.ctr_split_branch_node(left, right, split_at);
    });
}

M_PARAMS
void M_TYPE::ctr_split_path_raw(
        TreePathT& path,
        size_t level,
        int32_t split_at
)
{
    auto& self = this->self();

    return ctr_split_node_raw(
                path,
                level,
                [&self, split_at](const TreeNodePtr& left, const TreeNodePtr& right)
    {
        self.ctr_split_branch_node(left, right, split_at);
    });
}


M_PARAMS
bool M_TYPE::ctr_update_branch_node(const TreeNodePtr& node, int32_t idx, const BranchNodeEntry& keys)
{
    self().ctr_update_block_guard(node);

    auto update_state = self().make_branch_update_state();
    self().branch_dispatcher().dispatch(node, UpdateNodeFn(), idx, keys, update_state);

    return true;
}


M_PARAMS
bool M_TYPE::ctr_update_branch_node(const TreeNodeConstPtr& node, int32_t idx, const BranchNodeEntry& keys)
{
    self().ctr_update_block_guard(node);

    auto update_state = self().make_branch_update_state();
    self().branch_dispatcher().dispatch(node.as_mutable(), UpdateNodeFn(), idx, keys, update_state);

    return true;
}




M_PARAMS
bool M_TYPE::ctr_update_branch_nodes(
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

    return false;
}


M_PARAMS
void M_TYPE::ctr_update_path(TreePathT& path, size_t level, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    self.ctr_check_path(path, level);
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






M_PARAMS
void M_TYPE::ctr_do_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level)
{
    auto& self = this->self();

    self.ctr_check_same_paths(tgt_path, src_path, level + 1);

    self.ctr_cow_clone_path(tgt_path, level);

    TreeNodeConstPtr src = src_path[level];
    TreeNodeConstPtr tgt = tgt_path[level];

    self.ctr_update_block_guard(tgt);

    auto update_state = self.make_branch_update_state();
    self.branch_dispatcher().dispatch_1st_const(src, tgt.as_mutable(), MergeNodesFn(), update_state);

    auto parent_idx = self.ctr_get_parent_idx(src_path, level);

    auto max = self.ctr_get_node_max_keys(tgt);

    self.ctr_remove_non_leaf_node_entry(tgt_path, level + 1, parent_idx).get_or_throw();

    self.ctr_update_path(tgt_path, level, max);

    self.ctr_cow_ref_children_after_merge(src.as_mutable());

    return self.ctr_unref_block(src->id());
}



M_PARAMS
bool M_TYPE::ctr_merge_branch_nodes(TreePathT& tgt, TreePathT& src, size_t level, bool only_if_same_parent)
{
    auto& self = this->self();

    auto can_be_merged = self.ctr_can_merge_nodes(tgt[level], src[level]);
    if (can_be_merged)
    {
        auto same_parent = self.ctr_is_the_same_parent(tgt, src, level);
        if (same_parent)
        {
            self.ctr_do_merge_branch_nodes(tgt, src, level);
            return true;
        }
        else if (!only_if_same_parent)
        {
            auto parents_merged = ctr_merge_branch_nodes(tgt, src, level + 1, only_if_same_parent);

            self.ctr_assign_path_nodes(tgt, src, level);
            self.ctr_expect_next_node(src, level);

            if (parents_merged)
            {
                self.ctr_do_merge_branch_nodes(tgt, src, level);
                return true;
            }
        }
    }
    else
    {
        self.ctr_assign_path_nodes(tgt, src, level);
        self.ctr_expect_next_node(src, level);
    }

    return false;
}


#undef M_TYPE
#undef M_PARAMS

}
