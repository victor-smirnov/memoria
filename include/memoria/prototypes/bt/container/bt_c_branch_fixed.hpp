
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
    using typename Base::NodeBaseG;
    using typename Base::BranchNodeEntry;
    using typename Base::TreePathT;

    using SplitFn = std::function<VoidResult (NodeBaseG&, NodeBaseG&)>;



public:

    VoidResult ctr_update_path(TreePathT& path, size_t level) noexcept
    {
        auto& self = this->self();

        if (!path[level]->is_root())
        {
            MEMORIA_TRY(max_entry, self.ctr_get_node_max_keys(path[level]));
            return ctr_update_path(path, level, max_entry);
        }
        else {
            return VoidResult::of();
        }
    }

    VoidResult ctr_update_path(TreePathT& path, size_t level, const BranchNodeEntry& entry) noexcept;


public:
    MEMORIA_V1_DECLARE_NODE_FN(InsertFn, insert);
    VoidResult ctr_insert_to_branch_node(
            TreePathT& path,
            size_t level,
            int32_t idx,
            const BranchNodeEntry& keys,
            const BlockID& id
    ) noexcept;

    VoidResult ctr_split_path(
            TreePathT& path,
            size_t level,
            int32_t split_at
    ) noexcept;

    VoidResult ctr_split_path_raw(
            TreePathT& path,
            size_t level,
            int32_t split_at
    ) noexcept;

    VoidResult ctr_split_node(
            TreePathT& path,
            size_t level,
            const SplitFn& split_fn
    ) noexcept
    {
        MEMORIA_TRY_VOID(self().ctr_split_node_raw(path, level, split_fn));
        return self().ctr_expect_prev_node(path, level);
    }

    VoidResult ctr_split_node_raw(
            TreePathT& path,
            size_t level,
            const SplitFn& split_fn
    ) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN(UpdateNodeFn, updateUp);
    BoolResult ctr_update_branch_node(
            NodeBaseG& node,
            int32_t idx,
            const BranchNodeEntry& entry
    ) noexcept;

    BoolResult ctr_update_branch_nodes(
            TreePathT& path,
            size_t level,
            int32_t& idx,
            const BranchNodeEntry& entry
    ) noexcept;





    MEMORIA_V1_DECLARE_NODE2_FN(CanMergeFn, canBeMergedWith);
    BoolResult ctr_can_merge_nodes(const NodeBaseG& tgt, const NodeBaseG& src) noexcept
    {
        return self().node_dispatcher().dispatch(src, tgt, CanMergeFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(MergeNodesFn, mergeWith);
    VoidResult ctr_do_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level) noexcept;
    BoolResult ctr_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level, bool only_if_same_parent = false) noexcept;



MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
VoidResult M_TYPE::ctr_insert_to_branch_node(
        TreePathT& path,
        size_t level,
        int32_t idx,
        const BranchNodeEntry& keys,
        const BlockID& id
) noexcept
{
    using ResultT = VoidResult;

    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, level));

    NodeBaseG node = path[level];
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    MEMORIA_TRY_VOID(self.branch_dispatcher().dispatch(node, InsertFn(), idx, keys, id));

    if (!node->is_root())
    {
        MEMORIA_TRY(parent, self.ctr_get_node_parent_for_update(path, level));

        MEMORIA_TRY(max, self.ctr_get_node_max_keys(node));
        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent, node->id()));

        MEMORIA_TRY_VOID(self.ctr_update_branch_nodes(path, level + 1, parent_idx, max));
    }

    return ResultT::of();
}

M_PARAMS
VoidResult M_TYPE::ctr_split_node_raw(
        TreePathT& path,
        size_t level,
        const SplitFn& split_fn
) noexcept
{
    using ResultT = VoidResult;
    auto& self = this->self();

    if (level + 1 == path.size())
    {
        MEMORIA_TRY_VOID(self.ctr_create_new_root_block(path));
    }

    NodeBaseG left_node = path[level];

    MEMORIA_TRY(right_node, self.ctr_create_node(level, false, left_node->is_leaf(), left_node->header().memory_block_size()));

    MEMORIA_TRY_VOID(split_fn(left_node, right_node));

    MEMORIA_TRY(left_max, self.ctr_get_node_max_keys(left_node));
    MEMORIA_TRY(right_max, self.ctr_get_node_max_keys(right_node));

    MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(path[level + 1], left_node->id()));

    MEMORIA_TRY_VOID(self.ctr_update_branch_nodes(path, level + 1, parent_idx, left_max));

    MEMORIA_TRY(capacity, self.ctr_get_branch_node_capacity(path[level + 1], -1ull));

    if (capacity > 0)
    {
        MEMORIA_TRY_VOID(self.ctr_insert_to_branch_node(path, level + 1, parent_idx + 1, right_max, right_node->id()));
    }
    else {
        MEMORIA_TRY(parent_size, self.ctr_get_node_size(path[level + 1], 0));
        int32_t parent_split_idx = parent_size / 2;

        MEMORIA_TRY_VOID(ctr_split_path_raw(path, level + 1, parent_split_idx));

        if (parent_idx < parent_split_idx)
        {
            TreePathT left_path(path, level + 1);
            MEMORIA_TRY_VOID(self.ctr_expect_prev_node(left_path, level + 1));

            MEMORIA_TRY_VOID(
                self.ctr_assign_path_nodes(
                            left_path,
                            path,
                            level + 1
                )
            );
        }
        else {
            parent_idx -= parent_split_idx;
        }

        MEMORIA_TRY_VOID(
                    self.ctr_insert_to_branch_node(
                        path,
                        level + 1,
                        parent_idx + 1,
                        right_max,
                        right_node->id()
                    )
        );
    }

    path[level] = right_node;

    MEMORIA_TRY_VOID(self.ctr_ref_block(right_node->id()));

    return ResultT::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_split_path(
        TreePathT& path,
        size_t level,
        int32_t split_at
) noexcept
{
    auto& self = this->self();

    return ctr_split_node(
                path,
                level,
                [&self, split_at](NodeBaseG& left, NodeBaseG& right) noexcept -> VoidResult
    {
        MEMORIA_TRY_VOID(self.ctr_split_branch_node(left, right, split_at));
        return VoidResult::of();
    });
}

M_PARAMS
VoidResult M_TYPE::ctr_split_path_raw(
        TreePathT& path,
        size_t level,
        int32_t split_at
) noexcept
{
    auto& self = this->self();

    return ctr_split_node_raw(
                path,
                level,
                [&self, split_at](NodeBaseG& left, NodeBaseG& right) noexcept -> VoidResult
    {
        MEMORIA_TRY_VOID(self.ctr_split_branch_node(left, right, split_at));
        return VoidResult::of();
    });
}


M_PARAMS
BoolResult M_TYPE::ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys) noexcept
{
    MEMORIA_TRY_VOID(self().ctr_update_block_guard(node));

    MEMORIA_TRY_VOID(self().branch_dispatcher().dispatch(node, UpdateNodeFn(), idx, keys));

    return BoolResult::of(true);
}




M_PARAMS
BoolResult M_TYPE::ctr_update_branch_nodes(
        TreePathT& path,
        size_t level,
        int32_t& idx,
        const BranchNodeEntry& entry
) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, level));

    NodeBaseG tmp = path[level];

    MEMORIA_TRY_VOID(self.ctr_update_branch_node(tmp, idx, entry));

    while(!tmp->is_root())
    {
        MEMORIA_TRY(max, self.ctr_get_node_max_keys(tmp));

        MEMORIA_TRY(parent, self.ctr_get_node_parent_for_update(path, level));

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent, tmp->id()));

        tmp = parent;

        MEMORIA_TRY_VOID(self.ctr_update_branch_node(tmp, parent_idx, max));

        level++;
    }

    return BoolResult::of(false);
}


M_PARAMS
VoidResult M_TYPE::ctr_update_path(TreePathT& path, size_t level, const BranchNodeEntry& entry) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_check_path(path, level));
    MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(path, level));
    MEMORIA_TRY(using_next_node, self.ctr_update_branch_nodes(path, level + 1, parent_idx, entry));

    // Locating current child ID in the parent path[level+1].
    MEMORIA_TRY(child_idx, self.ctr_find_child_idx(path[level + 1], path[level]->id()));
    if (child_idx < 0)
    {
        // If no child is found in the parent, then we are looking eigher in the right,
        // or in the left parent's siblings.
        if (using_next_node)
        {
            MEMORIA_TRY_VOID(self.ctr_expect_prev_node(path, level + 1));
        }
        else {
            MEMORIA_TRY_VOID(self.ctr_expect_next_node(path, level + 1));
        }

        // This is just a check
        MEMORIA_TRY(child_idx, self.ctr_find_child_idx(path[level + 1], path[level]->id()));
        if (child_idx < 0)
        {
            return MEMORIA_MAKE_GENERIC_ERROR("ctr_update_path() internal error");
        }
    }

    return VoidResult::of();
}






M_PARAMS
VoidResult M_TYPE::ctr_do_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_check_same_paths(tgt_path, src_path, level + 1));

    MEMORIA_TRY_VOID(self.ctr_cow_clone_path(tgt_path, level));

    NodeBaseG src = src_path[level];
    NodeBaseG tgt = tgt_path[level];

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(tgt));

    MEMORIA_TRY_VOID(self.branch_dispatcher().dispatch_1st_const(src, tgt, MergeNodesFn()));

    MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(src_path, level));

    MEMORIA_TRY(max, self.ctr_get_node_max_keys(tgt));

    MEMORIA_TRY_VOID(self.ctr_remove_non_leaf_node_entry(tgt_path, level + 1, parent_idx));

    MEMORIA_TRY_VOID(self.ctr_update_path(tgt_path, level, max));

    MEMORIA_TRY_VOID(self.ctr_cow_ref_children_after_merge(src));

    return self.ctr_unref_block(src->id());
}



M_PARAMS
BoolResult M_TYPE::ctr_merge_branch_nodes(TreePathT& tgt, TreePathT& src, size_t level, bool only_if_same_parent) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY(can_be_merged, self.ctr_can_merge_nodes(tgt[level], src[level]));
    if (can_be_merged)
    {
        MEMORIA_TRY(same_parent, self.ctr_is_the_same_parent(tgt, src, level));
        if (same_parent)
        {
            MEMORIA_TRY_VOID(self.ctr_do_merge_branch_nodes(tgt, src, level));
            return BoolResult::of(true);
        }
        else if (!only_if_same_parent)
        {
            MEMORIA_TRY(parents_merged, ctr_merge_branch_nodes(tgt, src, level + 1, only_if_same_parent));

            MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(tgt, src, level));
            MEMORIA_TRY_VOID(self.ctr_expect_next_node(src, level));

            if (parents_merged)
            {
                MEMORIA_TRY_VOID(self.ctr_do_merge_branch_nodes(tgt, src, level));
                return BoolResult::of(true);
            }
        }
    }
    else
    {
        MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(tgt, src, level));
        MEMORIA_TRY_VOID(self.ctr_expect_next_node(src, level));
    }

    return BoolResult::of(false);
}


#undef M_TYPE
#undef M_PARAMS

}
