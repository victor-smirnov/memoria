
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
    using typename Base::NodeBaseG;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
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
            SplitFn split_fn
    ) noexcept
    {
        MEMORIA_TRY_VOID(ctr_split_node_raw(path, level, split_fn));
        return self().ctr_expect_prev_node(path, level);
    }

    VoidResult ctr_split_node_raw(
            TreePathT& path,
            size_t level,
            SplitFn split_fn
    ) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN(UpdateNodeFn, updateUp);


    BoolResult ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry) noexcept;

    BoolResult ctr_update_branch_nodes(TreePathT& path, size_t level, int32_t& idx, const BranchNodeEntry& entry) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN(TryMergeNodesFn, mergeWith);
    BoolResult ctr_try_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level) noexcept;
    BoolResult ctr_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level, bool only_if_same_parent = false) noexcept;

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
    using ResultT = VoidResult;
    auto& self = this->self();

    NodeBaseG node = path[level];
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    MEMORIA_TRY_VOID(self.branch_dispatcher().dispatch(node, InsertFn(), idx, sums, id));

    if (!node->is_root())
    {
        MEMORIA_TRY_VOID(self.ctr_update_path(path, level));
    }

    return ResultT::of();
}




M_PARAMS
VoidResult M_TYPE::ctr_split_node_raw(
        TreePathT& path,
        size_t level,
        SplitFn split_fn
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

    MEMORIA_TRY(right_max, self.ctr_get_node_max_keys(right_node));

    MEMORIA_TRY_VOID(self.ctr_update_path(path, level));

    MEMORIA_TRY(new_parent_idx, self.ctr_get_child_idx(path[level + 1], path[level]->id()));

    BlockUpdateMgr mgr(self);
    MEMORIA_TRY_VOID(mgr.add(path[level + 1]));

    VoidResult insertion_status = self.ctr_insert_to_branch_node(path, level + 1, new_parent_idx + 1, right_max, right_node->id());

    if (insertion_status.is_error())
    {
        if(insertion_status.is_packed_error())
        {
            mgr.rollback();

            MEMORIA_TRY(parent_size, self.ctr_get_node_size(path[level + 1], 0));
            int32_t parent_split_idx = parent_size / 2;

            MEMORIA_TRY_VOID(ctr_split_path_raw(path, level + 1, parent_split_idx));

            if (new_parent_idx < parent_split_idx)
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
                new_parent_idx -= parent_split_idx;
            }

            MEMORIA_TRY_VOID(mgr.add(path[level + 1]));

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
                    return MEMORIA_MAKE_GENERIC_ERROR("Can't insert node into the right path");
                }
                else {
                    return MEMORIA_PROPAGATE_ERROR(right_path_insertion_status);
                }
            }
        }
        else {
            return MEMORIA_PROPAGATE_ERROR(insertion_status);
        }
    }

    path[level] = right_node;

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
BoolResult M_TYPE::ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry) noexcept
{
    auto& self = this->self();

    BlockUpdateMgr mgr(self);
    MEMORIA_TRY_VOID(mgr.add(node));

    VoidResult res = self.branch_dispatcher().dispatch(node, UpdateNodeFn(), idx, entry);

    if (res.is_error()) {
        if (res.is_packed_error())
        {
            mgr.rollback();
            return BoolResult::of(false);
        }
        else {
            return MEMORIA_PROPAGATE_ERROR(res);
        }
    }

    return BoolResult::of(true);
}





M_PARAMS
BoolResult M_TYPE::ctr_update_branch_nodes(TreePathT& path, size_t level, int32_t& idx, const BranchNodeEntry& entry) noexcept
{
    auto& self = this->self();

    bool updated_next_node = false;

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(path[level]));

    MEMORIA_TRY(success, self.ctr_update_branch_node(path[level], idx, entry));
    if (!success)
    {
        MEMORIA_TRY(size, self.ctr_get_node_size(path[level], 0));
        int32_t split_idx = size / 2;

        MEMORIA_TRY_VOID(self.ctr_split_path_raw(path, level, split_idx));

        NodeBaseG node;

        if (idx < split_idx)
        {
            MEMORIA_TRY_VOID(self.ctr_expect_prev_node(path, level));
        }
        else {
            idx -= split_idx;
            updated_next_node = true;
        }

        node = path[level];

        MEMORIA_TRY(success2, self.ctr_update_branch_node(node, idx, entry));
        if (!success2)
        {
            return MEMORIA_MAKE_GENERIC_ERROR("Updating entry is too large");
        }
    }

    MEMORIA_TRY_VOID(self.ctr_update_path(path, level));

    return BoolResult::of(updated_next_node);
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
BoolResult M_TYPE::ctr_try_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_check_same_paths(tgt_path, src_path, level + 1));

    NodeBaseG src = src_path[level];
    NodeBaseG tgt = tgt_path[level];

    BlockUpdateMgr mgr(self);

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(tgt));

    MEMORIA_TRY_VOID(mgr.add(tgt));

    MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(src_path, level));

    auto res = self.branch_dispatcher().dispatch_1st_const(src, tgt, TryMergeNodesFn());
    if (res.is_error()) {
        if (res.is_packed_error())
        {
            mgr.rollback();
            return BoolResult::of(false);
        }
        else {
            return MEMORIA_PROPAGATE_ERROR(res);
        }
    }

    auto status = self.ctr_remove_non_leaf_node_entry(tgt_path, level + 1, parent_idx);

    if (status.is_error()) {
        if (status.is_packed_error())
        {
            mgr.rollback();
            return BoolResult::of(false);
        }
        else {
            return MEMORIA_PROPAGATE_ERROR(status);
        }
    }

    MEMORIA_TRY_VOID(self.ctr_update_path(tgt_path, level));
    MEMORIA_TRY_VOID(self.store().removeBlock(src->id()));

    return BoolResult::of(true);
}


M_PARAMS
BoolResult M_TYPE::ctr_merge_branch_nodes(
        TreePathT& tgt_path,
        TreePathT& src_path,
        size_t level,
        bool only_if_same_parent
) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY(is_same_parent, self.ctr_is_the_same_parent(tgt_path, src_path, level));
    if (is_same_parent)
    {
        MEMORIA_TRY(merged, self.ctr_try_merge_branch_nodes(tgt_path, src_path, level));
        if (!merged)
        {
            MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(tgt_path, src_path, level));
            MEMORIA_TRY_VOID(self.ctr_expect_next_node(src_path, level));
        }

        return merged_result;
    }
    else if (!only_if_same_parent)
    {
        MEMORIA_TRY(merged, ctr_merge_branch_nodes(tgt_path, src_path, level + 1));
        if (merged)
        {
            MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(tgt_path, src_path, level));
            MEMORIA_TRY_VOID(self.ctr_expect_next_node(src_path, level));

            return self.ctr_try_merge_branch_nodes(tgt_path, src_path, level);
        }
        else
        {
            MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(tgt_path, src_path, level));
            MEMORIA_TRY_VOID(self.ctr_expect_next_node(src_path, level));
        }
    }

    return BoolResult::of(false);
}





#undef M_TYPE
#undef M_PARAMS

}
