
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

#include <memoria/core/container/macros.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>


#include <functional>



namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::RemoveToolsName)

    using typename Base::CtrID;
    using typename Base::BlockID;
    using typename Base::NodeBaseG;
    using typename Base::Iterator;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::BranchNodeEntry;


public:
    VoidResult drop() noexcept
    {
        auto& self = this->self();

        if (self.store().isActive())
        {
            auto res0 = self.for_each_ctr_reference([&](auto prop_name, auto ctr_id) noexcept -> VoidResult {
                MEMORIA_TRY_VOID(self.store().drop_ctr(ctr_id));
                return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res0);

            MEMORIA_TRY(root, self.ctr_get_root_node());
            MEMORIA_TRY_VOID(self.ctr_remove_root_node(root));

            MEMORIA_TRY_VOID(self.set_root(BlockID{}));

            this->do_unregister_on_dtr_ = false;
            return self.store().unregisterCtr(self.name(), this);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Transaction must be in active state to drop containers");
        }
    }


    VoidResult cleanup() noexcept
    {
        auto& self = this->self();
        auto metadata = self.ctr_get_root_metadata();

        NodeBaseG new_root = self.ctr_create_node(0, true, true, metadata.memory_block_size());

        self.drop();
        return self.set_root(new_root->id());
    }

protected:
    MEMORIA_V1_DECLARE_NODE_FN(RemoveSpaceFn, removeSpace);

    VoidResult ctr_remove_node_recursively(NodeBaseG& node) noexcept;
    VoidResult ctr_remove_node(NodeBaseG& node) noexcept;
    VoidResult ctr_remove_root_node(NodeBaseG& node) noexcept;

    VoidResult ctr_remove_node_content(TreePathT& path, size_t level, int32_t start, int32_t end) noexcept;
    Result<Position> ctr_remove_leaf_content(TreePathT& path, const Position& start, const Position& end) noexcept;
    Result<Position> ctr_remove_leaf_content(TreePathT& path, int32_t stream, int32_t start, int32_t end) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN(RemoveNonLeafNodeEntryFn, removeSpaceAcc);
    VoidResult ctr_remove_non_leaf_node_entry(TreePathT& path, size_t level, int32_t idx) noexcept;

    struct LeftMergeResult {
        bool merged;
        Position original_left_sizes;
    };

    Result<LeftMergeResult> ctr_merge_leaf_with_left_sibling(TreePathT& path) noexcept;
    BoolResult ctr_merge_leaf_with_right_sibling(TreePathT& path) noexcept;
    //Result<MergeType> ctr_merge_leaf_with_siblings(TreePathT& path) noexcept;


    MEMORIA_V1_DECLARE_NODE_FN(ShouldBeMergedNodeFn, shouldBeMergedWithSiblings);
    BoolResult ctr_should_merge_node(const NodeBaseG& node) const noexcept
    {
        return self().node_dispatcher().dispatch(node, ShouldBeMergedNodeFn());
    }




    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------



    VoidResult ctr_remove_redundant_root(TreePathT& path, size_t level = 0) noexcept;


    BoolResult ctr_is_the_same_parent(const TreePathT& left, const TreePathT& right, size_t level) noexcept
    {
        if (level + 1 < left.size())
        {
            return BoolResult::of(left[level + 1]->id() == right[level + 1]->id());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Invalid tree path. Level = {}, height = {}", level, left.size());
        }
    }




MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::RemoveToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
VoidResult M_TYPE::ctr_remove_node_recursively(NodeBaseG& node) noexcept
{
    auto& self = this->self();

    if (!node->is_leaf())
    {
        //MEMORIA_TRY(size, self.ctr_get_node_size(node, 0));

        auto res = self.ctr_for_all_ids(node, /*0, size,*/ [&, this](const BlockID& id) noexcept -> VoidResult
        {
            auto& self = this->self();
            MEMORIA_TRY(child, self.ctr_get_block(id));
            return self.ctr_remove_node_recursively(child);
        });

        MEMORIA_RETURN_IF_ERROR(res);
    }

    return self.store().removeBlock(node->id());
}

M_PARAMS
VoidResult M_TYPE::ctr_remove_node(NodeBaseG& node) noexcept
{
    auto& self = this->self();

    BranchNodeEntry sums;
    Position sizes;

    if (!node->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent_for_update(node);

        self.ctr_remove_non_leaf_node_entry(parent, node->parent_idx());

        self.ctr_remove_node(node, sums, sizes);
    }
    else {
        MMA_THROW(Exception()) << WhatCInfo("Empty root node should not be deleted with this method.");
    }
}

M_PARAMS
VoidResult M_TYPE::ctr_remove_root_node(NodeBaseG& node) noexcept
{
    auto& self = this->self();
    MEMORIA_V1_ASSERT_TRUE_RTN(node->is_root());
    return self.ctr_remove_node_recursively(node);
}



M_PARAMS
VoidResult M_TYPE::ctr_remove_node_content(TreePathT& path, size_t level, int32_t start, int32_t end) noexcept
{
    using ResultT = VoidResult;
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(path[level]));

    auto res = self.ctr_for_all_ids(path[level], start, end, [&](const BlockID& id) noexcept -> VoidResult {
        MEMORIA_TRY(child, self.ctr_get_block(id));
        return self.ctr_remove_node_recursively(child);
    });
    MEMORIA_RETURN_IF_ERROR(res);

    MEMORIA_TRY_VOID(self.branch_dispatcher().dispatch(path[level], RemoveSpaceFn(), start, end));
    MEMORIA_TRY_VOID(self.ctr_update_path(path, level));

    return ResultT::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_non_leaf_node_entry(TreePathT& path, size_t level, int32_t start) noexcept
{
    auto& self = this->self();

    NodeBaseG node = path[level];

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    MEMORIA_TRY_VOID(self.branch_dispatcher().dispatch(node, RemoveNonLeafNodeEntryFn(), start, start + 1));

    MEMORIA_TRY_VOID(self.ctr_update_path(path, level));

    return VoidResult::of();
}



M_PARAMS
Result<typename M_TYPE::Position> M_TYPE::ctr_remove_leaf_content(TreePathT& path, const Position& start, const Position& end) noexcept
{
    using ResultT = Result<Position>;
    auto& self = this->self();

    NodeBaseG node = path.leaf();
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch(node, RemoveSpaceFn(), start, end));
    MEMORIA_TRY_VOID(self.ctr_update_path(path, 0));

    return ResultT::of(end - start);
}

M_PARAMS
Result<typename M_TYPE::Position> M_TYPE::ctr_remove_leaf_content(
        TreePathT& path,
        int32_t stream,
        int32_t start,
        int32_t end
) noexcept
{
    using ResultT = Result<Position>;
    auto& self = this->self();

    NodeBaseG node = path.leaf();
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch(node, RemoveSpaceFn(), stream, start, end));
    MEMORIA_TRY_VOID(self.ctr_update_path(path, 0));

    return ResultT::of(end - start);
}



M_PARAMS
VoidResult M_TYPE::ctr_remove_redundant_root(TreePathT& path, size_t level) noexcept
{
    auto& self = this->self();

    if (level + 1 < path.size())
    {
        MEMORIA_TRY(parent, self.ctr_get_node_parent(path, level));

        if (!parent->is_root())
        {
            MEMORIA_TRY_VOID(ctr_remove_redundant_root(path, level + 1));
        }

        if (parent->is_root())
        {
            MEMORIA_TRY(size, self.ctr_get_node_size(parent, 0));
            if (size == 1)
            {
                MEMORIA_TRY(root_metadata, self.ctr_get_root_metadata());

                NodeBaseG node = path[level];

                // FIXME redesign it to use tryConvertToRoot(node) instead
                if (self.ctr_can_convert_to_root(node, parent->root_metadata_size()))
                {
                    MEMORIA_TRY_VOID(self.ctr_node_to_root(node, root_metadata));
                    MEMORIA_TRY_VOID(self.store().removeBlock(parent->id()));
                    MEMORIA_TRY_VOID(self.set_root(node->id()));

                    path.remove_root();
                }
            }
        }
    }

    return VoidResult::of();
}


M_PARAMS
Result<typename M_TYPE::LeftMergeResult> M_TYPE::ctr_merge_leaf_with_left_sibling(TreePathT& path) noexcept
{
    using ResultT = Result<typename M_TYPE::LeftMergeResult>;
    auto& self = this->self();

    LeftMergeResult status{false};

    MEMORIA_TRY(should_merge, self.ctr_should_merge_node(path.leaf()));

    if (should_merge)
    {
        TreePathT prev = path;
        MEMORIA_TRY(has_prev, self.ctr_get_prev_node(prev, 0));

        if (has_prev)
        {
            MEMORIA_TRY(left_sizes, self.ctr_get_leaf_sizes(prev.leaf()));
            MEMORIA_TRY(merged, self.ctr_merge_leaf_nodes(prev, path, false));
            status.merged = merged;

            if (merged)
            {
                status.original_left_sizes = left_sizes;
                path = prev;
            }
        }
    }

    return ResultT::of(status);
}



M_PARAMS
BoolResult M_TYPE::ctr_merge_leaf_with_right_sibling(TreePathT& path) noexcept
{
    bool merged = false;

    auto& self = this->self();

    MEMORIA_TRY(should_merge, self.ctr_should_merge_node(path.leaf()));
    if (should_merge)
    {
        TreePathT next_path = path;
        MEMORIA_TRY(has_next, self.ctr_get_next_node(next_path, 0));

        if (has_next)
        {
            MEMORIA_TRY(has_merge, self.ctr_merge_leaf_nodes(path, next_path));
            merged = has_merge;
        }
    }

    return BoolResult::of(merged);
}







#undef M_TYPE
#undef M_PARAMS


}
