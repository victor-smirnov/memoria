
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
    using typename Base::BlockUpdateMgr;
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
    MEMORIA_V1_DECLARE_NODE_FN(InsertFn, insert);
    VoidResult ctr_insert_to_branch_node(
            TreePathT& path,
            size_t level,
            int32_t idx,
            const BranchNodeEntry& keys,
            const BlockID& id
    ) noexcept;

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
        ctr_split_node_raw(path, level, split_fn);
        return self().ctr_expect_prev_node(path, level);
    }

    void ctr_split_node_raw(
            TreePathT& path,
            size_t level,
            const SplitFn& split_fn
    );

    MEMORIA_V1_DECLARE_NODE_FN(UpdateNodeFn, updateUp);

    bool ctr_update_branch_node(const TreeNodePtr& node, int32_t idx, const BranchNodeEntry& entry);
    bool ctr_update_branch_node(const TreeNodeConstPtr& node, int32_t idx, const BranchNodeEntry& entry);

    bool ctr_update_branch_nodes(TreePathT& path, size_t level, int32_t& idx, const BranchNodeEntry& entry);

    MEMORIA_V1_DECLARE_NODE_FN(TryMergeNodesFn, mergeWith);
    bool ctr_try_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level);
    bool ctr_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level, bool only_if_same_parent = false);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
VoidResult M_TYPE::ctr_insert_to_branch_node(
        TreePathT& path,
        size_t level,
        int32_t idx,
        const BranchNodeEntry& sums,
        const BlockID& id
) noexcept
{
    return wrap_throwing([&]() -> VoidResult {
        auto& self = this->self();

        self.ctr_cow_clone_path(path, level);

        self.ctr_update_block_guard(path[level]);
        TreeNodeConstPtr node = path[level];

        VoidResult res = self.branch_dispatcher().dispatch(node.as_mutable(), InsertFn(), idx, sums, id);
        MEMORIA_RETURN_IF_ERROR(res);

        if (!node->is_root())
        {
            self.ctr_update_path(path, level);
        }

        return VoidResult::of();
    });
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
    // FIXME: must update the node here!!!

    auto right_node = self.ctr_create_node(level, false, left_node->is_leaf(), left_node->header().memory_block_size());

    split_fn(left_node.as_mutable(), right_node);

    auto right_max = self.ctr_get_node_max_keys(right_node.as_immutable());

    self.ctr_update_path(path, level);

    auto new_parent_idx = self.ctr_get_child_idx(path[level + 1], path[level]->id());

    BlockUpdateMgr mgr(self);
    mgr.add(path[level + 1].as_mutable());

    VoidResult insertion_status = self.ctr_insert_to_branch_node(path, level + 1, new_parent_idx + 1, right_max, right_node->id());

    if (insertion_status.is_error())
    {
        if(insertion_status.is_packed_error())
        {
            mgr.rollback();

            auto parent_size = self.ctr_get_node_size(path[level + 1], 0);
            int32_t parent_split_idx = parent_size / 2;

            ctr_split_path_raw(path, level + 1, parent_split_idx);

            if (new_parent_idx < parent_split_idx)
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
                new_parent_idx -= parent_split_idx;
            }

            mgr.add(path[level + 1].as_mutable());

            VoidResult right_path_insertion_status =
                        self.ctr_insert_to_branch_node(
                            path,
                            level + 1,
                            new_parent_idx + 1,
                            right_max,
                            right_node->id()
                        );


            if (right_path_insertion_status.is_error())
            {
                if(right_path_insertion_status.is_packed_error())
                {
                    mgr.rollback();
                    MEMORIA_MAKE_GENERIC_ERROR("Can't insert node into the right path").do_throw();
                }
                else {
                    MEMORIA_PROPAGATE_ERROR(right_path_insertion_status).do_throw();
                }
            }
        }
        else {
            MEMORIA_PROPAGATE_ERROR(insertion_status).do_throw();
        }
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
bool M_TYPE::ctr_update_branch_node(const TreeNodePtr& node, int32_t idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    BlockUpdateMgr mgr(self);
    mgr.add(node);

    VoidResult res = self.branch_dispatcher().dispatch(node, UpdateNodeFn(), idx, entry);

    if (res.is_error()) {
        if (res.is_packed_error())
        {
            mgr.rollback();
            return false;
        }
        else {
            MEMORIA_PROPAGATE_ERROR(res).do_throw();
        }
    }

    return true;
}


M_PARAMS
bool M_TYPE::ctr_update_branch_node(const TreeNodeConstPtr& node, int32_t idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    // FIXME Must update node here!!!

    BlockUpdateMgr mgr(self);
    mgr.add(node.as_mutable());

    VoidResult res = self.branch_dispatcher().dispatch(node.as_mutable(), UpdateNodeFn(), idx, entry);

    if (res.is_error()) {
        if (res.is_packed_error())
        {
            mgr.rollback();
            return false;
        }
        else {
            MEMORIA_PROPAGATE_ERROR(res).do_throw();
        }
    }

    return true;
}



M_PARAMS
bool M_TYPE::ctr_update_branch_nodes(TreePathT& path, size_t level, int32_t& idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    bool updated_next_node = false;

    self.ctr_cow_clone_path(path, level);
    self.ctr_update_block_guard(path[level]);

    auto success = self.ctr_update_branch_node(path[level], idx, entry);
    if (!success)
    {
        auto size = self.ctr_get_node_size(path[level], 0);
        int32_t split_idx = size / 2;

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

        auto success2 = self.ctr_update_branch_node(node, idx, entry);
        if (!success2)
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
bool M_TYPE::ctr_try_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level)
{
    auto& self = this->self();

    self.ctr_check_same_paths(tgt_path, src_path, level + 1);

    auto parent_idx = self.ctr_get_parent_idx(src_path, level);
    self.ctr_cow_clone_path(tgt_path, level);

    TreeNodeConstPtr src = src_path[level];
    TreeNodeConstPtr tgt = tgt_path[level];

    BlockUpdateMgr mgr(self);

    self.ctr_update_block_guard(tgt);

    mgr.add(tgt.as_mutable());


    auto res = self.branch_dispatcher().dispatch_1st_const(src, tgt.as_mutable(), TryMergeNodesFn());
    if (res.is_error()) {
        if (res.is_packed_error())
        {
            mgr.rollback();
            return false;
        }
        else {
            MEMORIA_PROPAGATE_ERROR(res).do_throw();
        }
    }

    // FIXME: in case of 'packed error' (that shouldn't happen),
    // the tree is in inconsistent state. So we need to backup the parent node too.
    auto status = self.ctr_remove_non_leaf_node_entry(tgt_path, level + 1, parent_idx);

    if (status.is_error()) {
        if (status.is_packed_error())
        {
            mgr.rollback();
            return false;
        }
        else {
            MEMORIA_PROPAGATE_ERROR(status).do_throw();
        }
    }

    // FIXME. Here src is actually immutable.
    // So ctr_cow_ref_children_after_merge should take const block ptr.

    self.ctr_cow_ref_children_after_merge(src.as_mutable());

    self.ctr_update_path(tgt_path, level);
    self.ctr_unref_block(src->id());

    return true;
}


M_PARAMS
bool M_TYPE::ctr_merge_branch_nodes(
        TreePathT& tgt_path,
        TreePathT& src_path,
        size_t level,
        bool only_if_same_parent
)
{
    auto& self = this->self();

    auto is_same_parent = self.ctr_is_the_same_parent(tgt_path, src_path, level);
    if (is_same_parent)
    {
        auto merged = self.ctr_try_merge_branch_nodes(tgt_path, src_path, level);
        if (!merged)
        {
            self.ctr_assign_path_nodes(tgt_path, src_path, level);
            self.ctr_expect_next_node(src_path, level);
        }

        return merged;
    }
    else if (!only_if_same_parent)
    {
        auto merged = ctr_merge_branch_nodes(tgt_path, src_path, level + 1);
        if (merged)
        {
            self.ctr_assign_path_nodes(tgt_path, src_path, level);
            self.ctr_expect_next_node(src_path, level);

            return self.ctr_try_merge_branch_nodes(tgt_path, src_path, level);
        }
        else
        {
            self.ctr_assign_path_nodes(tgt_path, src_path, level);
            self.ctr_expect_next_node(src_path, level);
        }
    }

    return false;
}

#undef M_TYPE
#undef M_PARAMS

}
