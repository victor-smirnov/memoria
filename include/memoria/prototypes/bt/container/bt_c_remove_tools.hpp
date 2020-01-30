
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

    typedef TypesType                                                           Types;
    typedef typename Base::Allocator                                            Allocator;

    using typename Base::BlockID;
    using typename Base::CtrID;

    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Metadata                                             Metadata;

    typedef std::function<VoidResult (const Position&)>                         MergeFn;

    using typename Base::TreePathT;
    using Base::CONTAINER_HASH;


public:
    VoidResult drop() noexcept
    {
        using ResultT = Result<void>;
        auto& self = this->self();

        if (self.store().isActive())
        {
            auto res0 = self.for_each_ctr_reference([&](auto prop_name, auto ctr_id) noexcept -> VoidResult {
                auto r0 = self.store().drop_ctr(ctr_id);
                MEMORIA_RETURN_IF_ERROR(r0);
                return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res0);

            Result<NodeBaseG> root = self.ctr_get_root_node();
            MEMORIA_RETURN_IF_ERROR(root);

            MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_root_node(root.get()));

            auto res1 = self.set_root(BlockID{});
            MEMORIA_RETURN_IF_ERROR(res1);

            this->do_unregister_on_dtr_ = false;
            return self.store().unregisterCtr(self.name(), this);
        }
        else {
            return ResultT::make_error("Transaction must be in active state to drop containers");
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
    MEMORIA_V1_DECLARE_NODE_FN_RTN(RemoveSpaceFn, removeSpace, OpStatus);

    VoidResult ctr_remove_node_recursively(NodeBaseG& node, Position& accum) noexcept;
    VoidResult ctr_remove_rode(NodeBaseG& node) noexcept;
    VoidResult ctr_remove_root_node(NodeBaseG& node) noexcept;

    VoidResult ctr_remove_node_content(TreePathT& path, size_t level, int32_t start, int32_t end, Position& sums) noexcept;
    Result<Position> ctr_remove_leaf_content(TreePathT& path, const Position& start, const Position& end) noexcept;
    Result<Position> ctr_remove_leaf_content(TreePathT& path, int32_t stream, int32_t start, int32_t end) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(RemoveNonLeafNodeEntryFn, removeSpaceAcc, OpStatus);
    Result<OpStatus> ctr_remove_non_leaf_node_entry(TreePathT& path, size_t level, int32_t idx) noexcept;

    BoolResult ctr_merge_leaf_with_left_sibling(TreePathT& path, MergeFn fn = [](const Position&, int32_t){}) noexcept;
    BoolResult ctr_merge_leaf_with_right_sibling(TreePathT& path) noexcept;
    Result<MergeType> ctr_merge_leaf_with_siblings(TreePathT& path, MergeFn fn = [](const Position&, int32_t){}) noexcept;


    MEMORIA_V1_DECLARE_NODE_FN_RTN(ShouldBeMergedNodeFn, shouldBeMergedWithSiblings, bool);
    bool ctr_should_merge_node(const NodeBaseG& node) const noexcept
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
            return BoolResult::make_error("Invalid tree path. Level = {}, height = {}", level, left.size());
        }
    }




MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::RemoveToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
VoidResult M_TYPE::ctr_remove_node_recursively(NodeBaseG& node, Position& sizes) noexcept
{
    auto& self = this->self();

    if (!node->is_leaf())
    {
        int32_t size = self.ctr_get_node_size(node, 0);
        auto res = self.ctr_for_all_ids(node, 0, size, [&, this](const BlockID& id) noexcept -> VoidResult
        {
            auto& self = this->self();
            MEMORIA_TRY(child, self.ctr_get_block(id));
            return self.ctr_remove_node_recursively(child, sizes);
        });

        MEMORIA_RETURN_IF_ERROR(res);
    }
    else {
        sizes += self.ctr_leaf_sizes(node);
    }

    return self.store().removeBlock(node->id());
}

M_PARAMS
VoidResult M_TYPE::ctr_remove_rode(NodeBaseG& node) noexcept
{
    auto& self = this->self();

    BranchNodeEntry sums;
    Position sizes;

    if (!node->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent_for_update(node);

        self.ctr_remove_non_leaf_node_entry(parent, node->parent_idx());

        self.ctr_remove_rode(node, sums, sizes);
    }
    else {
        MMA_THROW(Exception()) << WhatCInfo("Empty root node should not be deleted with this method.");
    }
}

M_PARAMS
VoidResult M_TYPE::ctr_remove_root_node(NodeBaseG& node) noexcept
{
    auto& self = this->self();

    //MEMORIA_V1_ASSERT_TRUE(node->is_root());

    Position sizes{};

    return self.ctr_remove_node_recursively(node, sizes);
}



M_PARAMS
VoidResult M_TYPE::ctr_remove_node_content(TreePathT& path, size_t level, int32_t start, int32_t end, Position& sizes) noexcept
{
    using ResultT = VoidResult;
    auto& self = this->self();

    auto res = self.ctr_for_all_ids(path[level], start, end, [&, this](const BlockID& id) noexcept -> VoidResult {
        auto& self = this->self();
        MEMORIA_TRY(child, self.ctr_get_block(id));
        return self.ctr_remove_node_recursively(child, sizes);
    });
    MEMORIA_RETURN_IF_ERROR(res);

    OpStatus status = self.branch_dispatcher().dispatch(path[level], RemoveSpaceFn(), start, end);
    if (isFail(status)) {
        return VoidResult::make_error("PackedOOMException");
    }

    MEMORIA_TRY_VOID(self.ctr_update_path(path, level));

    return ResultT::of();
}


M_PARAMS
Result<OpStatus> M_TYPE::ctr_remove_non_leaf_node_entry(TreePathT& path, size_t level, int32_t start) noexcept
{
    auto& self = this->self();

    NodeBaseG node = path[level];

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    if (isFail(self.branch_dispatcher().dispatch(node, RemoveNonLeafNodeEntryFn(), start, start + 1))) {
        return OpStatus::FAIL;
    }

    MEMORIA_TRY_VOID(self.ctr_update_path(path, level));

    return OpStatus::OK;
}



M_PARAMS
Result<typename M_TYPE::Position> M_TYPE::ctr_remove_leaf_content(TreePathT& path, const Position& start, const Position& end) noexcept
{
    using ResultT = Result<Position>;
    auto& self = this->self();

    NodeBaseG node = path.leaf();
    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_block_guard(node));

    OpStatus status = self.leaf_dispatcher().dispatch(node, RemoveSpaceFn(), start, end);

    if (isFail(status)) {
        return ResultT::make_error("PackedOOMException");
    }

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_path(path, 0));

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

    OpStatus status = self.leaf_dispatcher().dispatch(node, RemoveSpaceFn(), stream, start, end);

    if (isFail(status)) {
        return ResultT::make_error("PackedOOMException");
    }

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
            int32_t size = self.ctr_get_node_size(parent, 0);
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




/**
 * \brief Merge node with its siblings (if present).
 *
 * First try to merge with right sibling, then with left sibling.
 *
 * \param path path to the node
 * \param level level at the tree of the node
 * \param key_idx some key index in the merging node. After merge the value will be incremented with the size of
 * the merged sibling.
 * \return true if the node have been merged
 *
 * \see mergeWithRightSibling, mergeWithLeftSibling
 */

M_PARAMS
Result<MergeType> M_TYPE::ctr_merge_leaf_with_siblings(TreePathT& path, MergeFn fn) noexcept
{
    using ResultT = Result<MergeType>;
    auto& self = this->self();

    auto res0 = self.ctr_merge_leaf_with_right_sibling(path);
    MEMORIA_RETURN_IF_ERROR(res0);

    if (res0.get())
    {
        return ResultT::of(MergeType::RIGHT);
    }
    else {
        auto res1 = self.ctr_merge_leaf_with_left_sibling(path, fn);
        MEMORIA_RETURN_IF_ERROR(res1);

        if (res1.get())
        {
            return ResultT::of(MergeType::LEFT);
        }
        else {
            return ResultT::of(MergeType::NONE);
        }
    }
}


/**
 * \brief Try to merge node with its left sibling (if present).
 *
 * Calls \ref ctr_should_merge_node to check if requested node should be merged with its left sibling, then merge if true.
 *
 * \param path path to the node
 * \param level level at the tree of the node
 * \param key_idx some key index in the merging node. After merge the value will be incremented with the
 * size of the merged sibling.
 *
 * \return true if node has been merged
 *
 * \see mergeWithRightSibling, ctr_should_merge_node for details
 */


M_PARAMS
BoolResult M_TYPE::ctr_merge_leaf_with_left_sibling(TreePathT& path, MergeFn fn) noexcept
{
    auto& self = this->self();

    bool merged = false;

    if (self.ctr_should_merge_node(path.leaf()))
    {
        TreePathT prev = path;
        MEMORIA_TRY(has_prev, self.ctr_get_prev_node(prev, 0));

        if (has_prev)
        {
            auto merge_res = self.ctr_merge_leaf_nodes(prev, path, fn);
            MEMORIA_RETURN_IF_ERROR(merge_res);

            merged = merge_res.get();

            if (merged)
            {
                path = prev;
            }
        }
        else {
            merged = false;
        }
    }

    return BoolResult::of(merged);
}

/**
 * \brief Merge node with its right sibling (if present)
 *
 * Calls \ref ctr_should_merge_node to check if requested node should be merged with its right sibling, then merge if true.
 *
 * \param path path to the node
 * \param level level of the node in the tree

 * \return true if node has been merged
 *
 * \see mergeWithLeftSibling, ctr_should_merge_node for details
 */

M_PARAMS
BoolResult M_TYPE::ctr_merge_leaf_with_right_sibling(TreePathT& path) noexcept
{
    bool merged = false;

    auto& self = this->self();

    if (self.ctr_should_merge_node(path.leaf()))
    {
        TreePath next_path = path;
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
