
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
    using typename Base::TreeNodePtr;
    using typename Base::Iterator;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::BranchNodeEntry;

protected:
    MEMORIA_V1_DECLARE_NODE_FN(RemoveSpaceFn, removeSpace);

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


    MEMORIA_V1_DECLARE_NODE_FN(ShouldBeMergedNodeFn, shouldBeMergedWithSiblings);
    BoolResult ctr_should_merge_node(const TreeNodePtr& node) const noexcept
    {
        return self().node_dispatcher().dispatch(node, ShouldBeMergedNodeFn());
    }




    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------
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
VoidResult M_TYPE::ctr_remove_node_content(TreePathT& path, size_t level, int32_t start, int32_t end) noexcept
{
    using ResultT = VoidResult;
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, level));
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(path[level]));

    auto res = self.ctr_for_all_ids(path[level], start, end, [&](const BlockID& id) noexcept -> VoidResult {
        return self.ctr_unref_block(id);
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

    MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, level));

    TreeNodePtr node = path[level];

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

    TreeNodePtr node = path.leaf();
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

    TreeNodePtr node = path.leaf();
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

    MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch(node, RemoveSpaceFn(), stream, start, end));
    MEMORIA_TRY_VOID(self.ctr_update_path(path, 0));

    return ResultT::of(end - start);
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
