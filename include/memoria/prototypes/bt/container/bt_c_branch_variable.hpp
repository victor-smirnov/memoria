
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

#include <vector>

namespace memoria {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchVariableName)

public:
    using Types = typename Base::Types;

protected:
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
    VoidResult ctr_update_path(TreePathT& path, size_t level) noexcept;

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
            TreePathT& left_path,
            TreePathT& right_path,
            size_t level,
            int32_t split_at
    ) noexcept;

    VoidResult ctr_split_node(
            TreePathT& left_path,
            TreePathT& right_path,
            size_t level,
            SplitFn split_fn
    ) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(UpdateNodeFn, updateUp, OpStatus);


    BoolResult ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry) noexcept;

    VoidResult ctr_update_branch_nodes(TreePathT& path, size_t level, int32_t& idx, const BranchNodeEntry& entry) noexcept;

    VoidResult ctr_update_branch_nodes_no_backup(TreePathT& path, size_t level, int32_t idx, const BranchNodeEntry& entry) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(TryMergeNodesFn, mergeWith, OpStatus);
    BoolResult ctr_try_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level) noexcept;
    BoolResult ctr_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level) noexcept;
    BoolResult ctr_merge_current_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level) noexcept;


MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
Result<OpStatus> M_TYPE::ctr_insert_to_branch_node(
        TreePathT& path,
        size_t level,
        int32_t idx,
        const BranchNodeEntry& sums,
        const BlockID& id
) noexcept
{
    using ResultT = Result<OpStatus>;
    auto& self = this->self();

    NodeBaseG node = path[level];
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    if(isFail(self.branch_dispatcher().dispatch(node, InsertFn(), idx, sums, id))) {
        return ResultT::of(OpStatus::FAIL);
    }

    if (!node->is_root())
    {
        MEMORIA_TRY(parent, self.ctr_get_node_parent_for_update(path, level));
        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent, node->id()));

        auto max = self.ctr_get_node_max_keys(node);
        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_branch_nodes(path, level + 1, parent_idx, max));
    }

    return ResultT::of(OpStatus::OK);
}




M_PARAMS
VoidResult M_TYPE::ctr_split_node(
        TreePathT& left_path,
        TreePathT& right_path,
        size_t level,
        SplitFn split_fn
) noexcept
{
    using ResultT = VoidResult;
    auto& self = this->self();

    if (level + 1 == left_path.size())
    {
        // We are at the root of the tree
        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_create_new_root_block(left_path, right_path));
    }

    NodeBaseG left_node = left_path[level];

    Result<NodeBaseG> left_parent_res = self.ctr_get_node_parent(left_path, level);
    MEMORIA_RETURN_IF_ERROR(left_parent_res);
    NodeBaseG left_parent = left_parent_res.get();

    Result<NodeBaseG> right_node_res = self.ctr_create_node(level, false, left_node->is_leaf(), left_node->header().memory_block_size());
    MEMORIA_RETURN_IF_ERROR(right_node_res);
    NodeBaseG right_node = right_node_res.get();

    MEMORIA_RETURN_IF_ERROR_FN(split_fn(left_node, right_node));

    auto left_max  = self.ctr_get_node_max_keys(left_node);
    auto right_max = self.ctr_get_node_max_keys(right_node);

    MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(left_parent, left_node->id()));

    MEMORIA_TRY_VOID(self.ctr_update_branch_nodes(left_path, level + 1, parent_idx, left_max));

    BlockUpdateMgr mgr(self);
    MEMORIA_TRY_VOID(mgr.add(left_parent));

    Result<OpStatus> ins0_res = self.ctr_insert_to_branch_node(left_path, level + 1, parent_idx + 1, right_max, right_node->id());
    MEMORIA_RETURN_IF_ERROR(ins0_res);

    if(isFail(ins0_res.get()))
    {
        mgr.rollback();

        MEMORIA_TRY_VOID(ctr_split_path(left_path, right_path, level + 1, parent_idx + 1));
        MEMORIA_TRY_VOID(mgr.add(right_path[level + 1]));

        Result<OpStatus> ins1_res = self.ctr_insert_to_branch_node(right_path, level + 1, 0, right_max, right_node->id());
        MEMORIA_RETURN_IF_ERROR(ins1_res);

        if(isFail(ins1_res.get()))
        {
            mgr.rollback();

            int32_t right_parent_size = self.ctr_get_node_size(right_path[level + 1], 0);

            auto split_res = ctr_split_path(left_path, right_path, level + 1, right_parent_size / 2);
            MEMORIA_RETURN_IF_ERROR(split_res);

            Result<OpStatus> ins2_res = self.ctr_insert_to_branch_node(right_path, level + 1, 0, right_max, right_node->id());
            MEMORIA_RETURN_IF_ERROR(ins2_res);

            if(isFail(ins2_res.get()))
            {
                return ResultT::make_error("PackedOOMException");
            }
        }
    }

    return ResultT::of();
}

M_PARAMS
VoidResult M_TYPE::ctr_split_path(
        TreePathT& left_path,
        TreePathT& right_path,
        size_t level,
        int32_t split_at
) noexcept
{
    auto& self = this->self();

    return ctr_split_node(
                left_path,
                right_path,
                level,
                [&self, split_at](NodeBaseG& left, NodeBaseG& right) noexcept -> VoidResult {
        auto split_res = self.ctr_split_branch_node(left, right, split_at);
        MEMORIA_RETURN_IF_ERROR(split_res);
        return VoidResult::of();
    });
}



M_PARAMS
BoolResult M_TYPE::ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry) noexcept
{
    auto& self = this->self();

    BlockUpdateMgr mgr(self);
    MEMORIA_TRY_VOID(mgr.add(node));

    if (isFail(self.branch_dispatcher().dispatch(node, UpdateNodeFn(), idx, entry))) {
        mgr.rollback();
        return BoolResult::of(false);
    }

    return BoolResult::of(true);
}





M_PARAMS
VoidResult M_TYPE::ctr_update_branch_nodes(TreePathT& path, size_t level, int32_t& idx, const BranchNodeEntry& entry) noexcept
{
    auto& self = this->self();

    NodeBaseG node = path[level];
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    auto res = self.ctr_update_branch_node(node, idx, entry);
    MEMORIA_RETURN_IF_ERROR(res);

    if (!res.get())
    {
        int32_t size        = self.ctr_get_node_size(node, 0);
        int32_t split_idx   = size / 2;

        // FIXME: paths
        TreePathT right_path;

        MEMORIA_TRY_VOID(self.ctr_split_path(path, right_path, level, split_idx));

        if (idx >= split_idx)
        {
            idx -= split_idx;
            path = right_path;
            node = path[level];
        }

        BoolResult upd_result = self.ctr_update_branch_node(node, idx, entry);
        MEMORIA_RETURN_IF_ERROR(upd_result);

        if (!upd_result.get())
        {
            // TODO: error handling
            // upd_result must be true here
            return VoidResult::make_error("SomethingWrongException");
        }
    }

    if(!node->is_root())
    {
        Result<NodeBaseG> parent = self.ctr_get_node_parent_for_update(path, level);
        MEMORIA_RETURN_IF_ERROR(parent);

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent.get(), node->id()));

        auto max = self.ctr_get_node_max_keys(node);
        return self.ctr_update_branch_nodes(path, level + 1, parent_idx, max);
    }

    return VoidResult::of();
}






M_PARAMS
VoidResult M_TYPE::ctr_update_path(TreePathT& path, size_t level) noexcept
{
    auto& self = this->self();

    NodeBaseG node = path[level];

    if (!node->is_root())
    {
        auto entry = self.ctr_get_node_max_keys(node);

        Result<NodeBaseG> parent = self.ctr_get_node_parent_for_update(path, level);
        MEMORIA_RETURN_IF_ERROR(parent);

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent.get(), node->id()));
        return self.ctr_update_branch_nodes(path, level + 1, parent_idx, entry);
    }

    return VoidResult::of();
}



M_PARAMS
VoidResult M_TYPE::ctr_update_branch_nodes_no_backup(
        TreePathT& path,
        size_t level,
        int32_t idx,
        const BranchNodeEntry& entry
) noexcept
{
    auto& self = this->self();

    NodeBaseG node = path[level];

    self.ctr_update_branch_node(node, idx, entry);

    if(!node->is_root())
    {
        Result<NodeBaseG> parent = self.ctr_get_node_parent_for_update(path, level);
        MEMORIA_RETURN_IF_ERROR_FN(parent);

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent.get(), node->id()));

        self.ctr_update_branch_nodes_no_backup(path, level + 1, parent_idx, entry);
    }

    return VoidResult::of();
}


M_PARAMS
BoolResult M_TYPE::ctr_try_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level) noexcept
{
    auto& self = this->self();

    NodeBaseG src = src_path[level];
    NodeBaseG tgt = tgt_path[level];

    BlockUpdateMgr mgr(self);

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(src));
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(tgt));

    MEMORIA_TRY_VOID(mgr.add(src));
    MEMORIA_TRY_VOID(mgr.add(tgt));


    //int32_t tgt_size = self.ctr_get_node_size(tgt, 0);

    MEMORIA_TRY(src_parent, self.ctr_get_node_parent(src_path, level));
    MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(src_parent, src->id()));


    if (isFail(self.branch_dispatcher().dispatch(src, tgt, TryMergeNodesFn())))
    {
        mgr.rollback();
        return BoolResult::of(false);
    }

    auto max = self.ctr_get_node_max_keys(tgt);

    Result<OpStatus> status = self.ctr_remove_non_leaf_node_entry(src_path, level + 1, parent_idx);
    MEMORIA_RETURN_IF_ERROR(status);

    if (isFail(status.get())) {
        mgr.rollback();
        return BoolResult::of(false);
    }

    int32_t idx = parent_idx - 1;

    MEMORIA_TRY_VOID(self.ctr_update_branch_nodes(src_path, level + 1, idx, max));
    MEMORIA_TRY_VOID(self.store().removeBlock(src->id()));

    return BoolResult::of(true);
}


M_PARAMS
BoolResult M_TYPE::ctr_merge_branch_nodes(
        TreePathT& tgt_path,
        TreePathT& src_path,
        size_t level
) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY(is_same_parent, self.ctr_is_the_same_parent(tgt_path, src_path, level));
    if (is_same_parent)
    {
        return self.ctr_merge_current_branch_nodes(tgt_path, src_path, level);
    }
    else
    {
        auto res = ctr_merge_branch_nodes(tgt_path, src_path, 1);
        if (res.get())
        {
            return self.ctr_merge_current_branch_nodes(tgt_path, src_path, 1);
        }
        else
        {
            return BoolResult::of(false);
        }
    }
}




M_PARAMS
BoolResult M_TYPE::ctr_merge_current_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level) noexcept
{
    auto& self = this->self();

    auto res0 = self.ctr_try_merge_branch_nodes(tgt_path, src_path, level);
    MEMORIA_RETURN_IF_ERROR(res0);
    if (res0.get())
    {
        auto res1 = self.ctr_remove_redundant_root(tgt_path, level);
        MEMORIA_RETURN_IF_ERROR(res1);

        return BoolResult::of(true);
    }
    else {
        return BoolResult::of(false);
    }
}


#undef M_TYPE
#undef M_PARAMS

}
