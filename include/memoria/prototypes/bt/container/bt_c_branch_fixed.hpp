
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

#include <vector>

namespace memoria {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchFixedName)

public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    using typename Base::BlockID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;

    typedef std::function<VoidResult (NodeBaseG&, NodeBaseG&)>                  SplitFn;

    using typename Base::TreePathT;

    static const int32_t Streams = Types::Streams;

public:

    VoidResult ctr_update_path(TreePathT& path, size_t level) noexcept
    {
        auto& self = this->self();

        if (!path[level]->is_root())
        {
            auto max_entry = self.ctr_get_node_max_keys(path[level]);
            return ctr_update_path(path, level, max_entry);
        }
        else {
            return VoidResult::of();
        }
    }

    VoidResult ctr_update_path(TreePathT& path, size_t level, const BranchNodeEntry& entry) noexcept;


public:
    MEMORIA_V1_DECLARE_NODE_FN_RTN(InsertFn, insert, OpStatus);
    Result<OpStatus> ctr_insert_to_branch_node(
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
        MEMORIA_TRY_VOID(self().ctr_split_node_raw(path, level, split_fn));
        return self().ctr_expect_prev_node(path, level);
    }

    VoidResult ctr_split_node_raw(
            TreePathT& path,
            size_t level,
            SplitFn split_fn
    ) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(UpdateNodeFn, updateUp, OpStatus);
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





    MEMORIA_V1_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);
    bool ctr_can_merge_nodes(const NodeBaseG& tgt, const NodeBaseG& src) noexcept
    {
        return self().node_dispatcher().dispatch(src, tgt, CanMergeFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(MergeNodesFn, mergeWith, OpStatus);
    VoidResult ctr_do_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level) noexcept;
    BoolResult ctr_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level, bool only_if_same_parent = false) noexcept;
    BoolResult ctr_merge_current_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level) noexcept;


MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
Result<OpStatus> M_TYPE::ctr_insert_to_branch_node(
        TreePathT& path,
        size_t level,
        int32_t idx,
        const BranchNodeEntry& keys,
        const BlockID& id
) noexcept
{
    using ResultT = Result<OpStatus>;

    auto& self = this->self();

    NodeBaseG node = path[level];
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    if (isFail(self.branch_dispatcher().dispatch(node, InsertFn(), idx, keys, id))) {
        return ResultT::of(OpStatus::FAIL);
    }

    if (!node->is_root())
    {
        MEMORIA_TRY(parent, self.ctr_get_node_parent_for_update(path, level));

        auto max = self.ctr_get_node_max_keys(node);
        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent, node->id()));

        auto res = self.ctr_update_branch_nodes(path, level + 1, parent_idx, max);
        MEMORIA_RETURN_IF_ERROR(res);
    }

    return ResultT::of(OpStatus::OK);
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

    auto left_max  = self.ctr_get_node_max_keys(left_node);
    auto right_max = self.ctr_get_node_max_keys(right_node);

    MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(path[level + 1], left_node->id()));

    MEMORIA_TRY_VOID(self.ctr_update_branch_nodes(path, level + 1, parent_idx, left_max));

    if (self.ctr_get_branch_node_capacity(path[level + 1], -1) > 0)
    {
        MEMORIA_TRY(insertion_status, self.ctr_insert_to_branch_node(path, level + 1, parent_idx + 1, right_max, right_node->id()));
        if (isFail(insertion_status))
        {
            return ResultT::make_error("PackedOOMException");
        }
    }
    else {
        int32_t parent_size = self.ctr_get_node_size(path[level + 1], 0);
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

        MEMORIA_TRY(
                    right_path_insertion_status,
                    self.ctr_insert_to_branch_node(
                        path,
                        level + 1,
                        parent_idx + 1,
                        right_max,
                        right_node->id()
                    )
        );

        if(isFail(right_path_insertion_status))
        {
            return ResultT::make_error("Can't insert node into the right path");
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
BoolResult M_TYPE::ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys) noexcept
{
    MEMORIA_TRY_VOID(self().ctr_update_block_guard(node));

    auto res = self().branch_dispatcher().dispatch(node, UpdateNodeFn(), idx, keys);
    if (!isOk(res))
    {
        return BoolResult::make_error("PackedOOMException");
    }

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

    NodeBaseG tmp = path[level];

    MEMORIA_TRY_VOID(self.ctr_update_branch_node(tmp, idx, entry));

    while(!tmp->is_root())
    {
        auto max = self.ctr_get_node_max_keys(tmp);

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
    int32_t child_idx = self.ctr_find_child_idx(path[level + 1], path[level]->id());
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
        int32_t child_idx = self.ctr_find_child_idx(path[level + 1], path[level]->id());
        if (child_idx < 0)
        {
            return VoidResult::make_error("ctr_update_path() internal error");
        }
    }

    return VoidResult::of();
}





/**
 * \brief Merge *src* path to the *tgt* path unconditionally.
 *
 * Perform merging of two paths, *src* to *dst* at the specified *level*. Both nodes (at boths paths) must
 * have the same parent.
 *
 * \param tgt path to node to be merged with
 * \param src path to node to be merged
 * \param level level of the node in the tree
 * \return true if paths have been merged
 *
 * \see mergeWithSiblings - this is the basic method
 */

M_PARAMS
VoidResult M_TYPE::ctr_do_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_check_same_paths(tgt_path, src_path, level + 1));

    NodeBaseG src = src_path[level];
    NodeBaseG tgt = tgt_path[level];

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(tgt));
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(src));

    OpStatus status0 = self.branch_dispatcher().dispatch(src, tgt, MergeNodesFn());
    if (isFail(status0)) {
        return VoidResult::make_error("PackedOOMException");
    }

    MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(src_path, level));

    auto max = self.ctr_get_node_max_keys(tgt);

    MEMORIA_TRY(status1, self.ctr_remove_non_leaf_node_entry(tgt_path, level + 1, parent_idx));
    if (isFail(status1)) {
        return VoidResult::make_error("PackedOOMException");
    }

    MEMORIA_TRY_VOID(self.ctr_update_path(tgt_path, level, max));

    return self.store().removeBlock(src->id());
}

/**
 * \brief Merge *src* path to the *tgt* path.
 *
 * Merge two tree paths, *src* to *dst* upward starting from nodes specified with *level*. If both these
 * nodes have different parents, then recursively merge parents first. Calls \ref ctr_can_merge_nodes to check if nodes can be merged.
 * This call will try to merge parents only if current nodes can be merged.
 *
 * If after nodes have been merged the resulting path is redundant, that means it consists from a single node chain,
 * then this path is truncated from the tree root down to the specified *level*.
 *
 * Unlike this call, \ref mergePaths tries to merge paths starting from the root down to the specified *level*.
 *
 * \param tgt path to node to be merged with
 * \param src path to node to be merged
 * \param level level of the node in the tree
 * \return true if paths have been merged
 *
 * \see mergeWithSiblings - this is the basic method
 * \see ctr_can_merge_nodes, removeRedundantRoot, mergeNodes, ctr_is_the_same_parent
 */

M_PARAMS
BoolResult M_TYPE::ctr_merge_branch_nodes(TreePathT& tgt, TreePathT& src, size_t level, bool only_if_same_parent) noexcept
{
    auto& self = this->self();

    if (self.ctr_can_merge_nodes(tgt[level], src[level]))
    {
        MEMORIA_TRY(same_parent, self.ctr_is_the_same_parent(tgt, src, level));
        if (same_parent)
        {
            MEMORIA_TRY_VOID(self.ctr_do_merge_branch_nodes(tgt, src, level));
            MEMORIA_TRY_VOID(self.ctr_remove_redundant_root(tgt, level));

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
                MEMORIA_TRY_VOID(self.ctr_remove_redundant_root(tgt));

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



M_PARAMS
BoolResult M_TYPE::ctr_merge_current_branch_nodes(
        TreePathT& tgt_path,
        TreePathT& src_path,
        size_t level
) noexcept
{
    auto& self = this->self();

    if (self.ctr_can_merge_nodes(tgt_path[level], src_path[level]))
    {
        MEMORIA_TRY_VOID(self.ctr_do_merge_branch_nodes(tgt_path, src_path, level));
        MEMORIA_TRY_VOID(self.ctr_remove_redundant_root(tgt_path, level));

        return BoolResult::of(true);
    }
    else
    {
        return BoolResult::of(false);
    }
}




#undef M_TYPE
#undef M_PARAMS

}
