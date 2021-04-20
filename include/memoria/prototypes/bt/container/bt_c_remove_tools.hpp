
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
    using typename Base::TreeNodeConstPtr;
    using typename Base::Iterator;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::BranchNodeEntry;

protected:
    MEMORIA_V1_DECLARE_NODE_FN(RemoveSpaceFn, removeSpace);

    void ctr_remove_node_content(TreePathT& path, size_t level, int32_t start, int32_t end);
    Position ctr_remove_leaf_content(TreePathT& path, const Position& start, const Position& end);
    Position ctr_remove_leaf_content(TreePathT& path, int32_t stream, int32_t start, int32_t end);

    MEMORIA_V1_DECLARE_NODE_FN(RemoveNonLeafNodeEntryFn, removeSpaceAcc);
    VoidResult ctr_remove_non_leaf_node_entry(TreePathT& path, size_t level, int32_t idx) noexcept;

    struct LeftMergeResult {
        bool merged;
        Position original_left_sizes;
    };

    LeftMergeResult ctr_merge_leaf_with_left_sibling(TreePathT& path);
    bool ctr_merge_leaf_with_right_sibling(TreePathT& path);


    MEMORIA_V1_DECLARE_NODE_FN(ShouldBeMergedNodeFn, shouldBeMergedWithSiblings);
    bool ctr_should_merge_node(const TreeNodeConstPtr& node) const
    {
        return self().node_dispatcher().dispatch(node, ShouldBeMergedNodeFn()).get_or_throw();
    }




    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------
    bool ctr_is_the_same_parent(const TreePathT& left, const TreePathT& right, size_t level)
    {
        if (level + 1 < left.size())
        {
            return left[level + 1]->id() == right[level + 1]->id();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid tree path. Level = {}, height = {}", level, left.size()).do_throw();
        }
    }




MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::RemoveToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS











M_PARAMS
void M_TYPE::ctr_remove_node_content(TreePathT& path, size_t level, int32_t start, int32_t end)
{    
    auto& self = this->self();

    self.ctr_cow_clone_path(path, level);
    self.ctr_update_block_guard(path[level]);

    self.ctr_for_all_ids(path[level], start, end, [&](const BlockID& id) {
        return self.ctr_unref_block(id);
    });


    self.branch_dispatcher().dispatch(path[level].as_mutable(), RemoveSpaceFn(), start, end).get_or_throw();
    self.ctr_update_path(path, level);
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_non_leaf_node_entry(TreePathT& path, size_t level, int32_t start) noexcept
{
    return wrap_throwing([&]() -> VoidResult {
        auto& self = this->self();

        self.ctr_cow_clone_path(path, level);

        TreeNodeConstPtr node = path[level];

        self.ctr_update_block_guard(node);

        VoidResult res = self.branch_dispatcher().dispatch(node.as_mutable(), RemoveNonLeafNodeEntryFn(), start, start + 1);
        MEMORIA_RETURN_IF_ERROR(res);

        self.ctr_update_path(path, level);

        return VoidResult::of();
    });
}



M_PARAMS
typename M_TYPE::Position M_TYPE::ctr_remove_leaf_content(TreePathT& path, const Position& start, const Position& end)
{
    auto& self = this->self();

    TreeNodeConstPtr node = path.leaf();
    self.ctr_update_block_guard(node);

    self.leaf_dispatcher().dispatch(node.as_mutable(), RemoveSpaceFn(), start, end).get_or_throw();
    self.ctr_update_path(path, 0);

    return end - start;
}

M_PARAMS
typename M_TYPE::Position M_TYPE::ctr_remove_leaf_content(
        TreePathT& path,
        int32_t stream,
        int32_t start,
        int32_t end
)
{
    auto& self = this->self();

    TreeNodeConstPtr node = path.leaf();
    self.ctr_update_block_guard(node);

    self.leaf_dispatcher().dispatch(node.as_mutable(), RemoveSpaceFn(), stream, start, end).get_or_throw();
    self.ctr_update_path(path, 0);

    return end - start;
}





M_PARAMS
typename M_TYPE::LeftMergeResult M_TYPE::ctr_merge_leaf_with_left_sibling(TreePathT& path)
{
    auto& self = this->self();

    LeftMergeResult status{false};

    auto should_merge = self.ctr_should_merge_node(path.leaf());

    if (should_merge)
    {
        TreePathT prev = path;
        auto has_prev = self.ctr_get_prev_node(prev, 0);

        if (has_prev)
        {
            auto left_sizes = self.ctr_get_leaf_sizes(prev.leaf());
            auto merged = self.ctr_merge_leaf_nodes(prev, path, false);
            status.merged = merged;

            if (merged)
            {
                status.original_left_sizes = left_sizes;
                path = prev;
            }
        }
    }

    return status;
}



M_PARAMS
bool M_TYPE::ctr_merge_leaf_with_right_sibling(TreePathT& path)
{
    bool merged = false;

    auto& self = this->self();

    auto should_merge = self.ctr_should_merge_node(path.leaf());
    if (should_merge)
    {
        TreePathT next_path = path;
        auto has_next = self.ctr_get_next_node(next_path, 0);

        if (has_next)
        {
            auto has_merge = self.ctr_merge_leaf_nodes(path, next_path);
            merged = has_merge;
        }
    }

    return merged;
}


#undef M_TYPE
#undef M_PARAMS


}
