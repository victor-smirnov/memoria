
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

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::RemoveBatchName)

    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::TreePathT;




    void ctr_remove_entries(
            TreePathT& from_path,
            Position&  from_idx,
            TreePathT& to_path,
            Position&  to_idx,
            bool merge = true
    );





    void ctr_remove_nodes_from_start(
            TreePathT& stop,
            const Position& stop_idx
    );

    void ctr_remove_branch_nodes_from_start(
            TreePathT& stop_path,
            size_t level,
            int32_t stop_idx
    );


    void ctr_remove_nodes_at_end(
            TreePathT& start_path,
            const Position& start_idx
    );

    void ctr_remove_branch_nodes_at_end(
            TreePathT& start_path,
            size_t level,
            int32_t start_idx
    );

    void ctr_remove_nodes(
            TreePathT& start_path,
            const Position& start_idx,

            TreePathT& stop_path,
            Position& stop_idx
    );


    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    using Base::ctr_remove_branch_nodes;

    void ctr_remove_branch_nodes(
            TreePathT& start_path,
            int32_t start_idx,
            TreePathT& stop_path,
            int32_t stop_idx,
            size_t level
    );



MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::RemoveBatchName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



/**
 * \brief Removes all entries from the tree starting with *from* iterator and ending with *to* one, but not include it.
 *
 * \param from
 * \param to
 * \param keys
 * \param merge if *true* then try to merge btree leafs if necessary.
 *
 * \return number of removed entries
 *
 * \see removeAllPages, removeBlocksFromStart, removeBlocksAtEnd, removeBlocks
 * \see commit_merge_withRightSibling, addTotalKeyCount
 */

M_PARAMS
void M_TYPE::ctr_remove_entries(
        TreePathT& start_path,
        Position&  start_idx,
        TreePathT& stop_path,
        Position&  stop_idx,
        bool merge
)
{
    auto& self = this->self();

    auto stop_sizes = self.ctr_get_node_sizes(stop_path.leaf());

    bool at_end;

    if (stop_idx.ltAny(stop_sizes))
    {
        at_end = false;
    }
    else
    {
        auto has_next_node = self.ctr_get_next_node(stop_path, 0);

        if (has_next_node)
        {
            //stop_path = next.get();
            stop_idx = Position(0);

            at_end = false;
        }
        else {
            at_end = true;
        }
    }


    bool from_start;

    if (start_idx.eqAll(0))
    {
        auto has_prev_node = self.ctr_get_prev_node(start_path, 0);

        if (has_prev_node)
        {
            auto sizes = self.ctr_get_node_sizes(start_path.leaf());
            start_idx   = sizes;
            from_start  = false;
        }
        else {
            from_start = true;
        }
    }
    else {
        from_start = false;
    }


    if (from_start && at_end)
    {
        self.ctr_remove_all_nodes(start_path, stop_path);
        start_idx = stop_idx.setAll(0);
    }
    else if (from_start && !at_end)
    {
        ctr_remove_nodes_from_start(stop_path, stop_idx);

        if (merge)
        {
            self.ctr_merge_leaf_with_right_sibling(stop_path);
        }

        start_path  = stop_path;
        start_idx   = stop_idx = Position(0);
    }
    else if ((!from_start) && at_end)
    {
        ctr_remove_nodes_at_end(start_path, start_idx);

        if (merge)
        {
            auto merge_status = self.ctr_merge_leaf_with_left_sibling(start_path);
            if(merge_status.merged) {
                start_idx += merge_status.original_left_sizes;
            }
        }

        stop_path   = start_path;
        stop_idx    = start_idx;
    }
    else {
        ctr_remove_nodes(start_path, start_idx, stop_path, stop_idx);
        start_path  = stop_path;
        start_idx   = stop_idx;
    }
}







M_PARAMS
void M_TYPE::ctr_remove_branch_nodes_from_start(TreePathT& stop_path, size_t level, int32_t stop_idx)
{
    auto& self = this->self();

    self.ctr_remove_branch_content(stop_path, level, 0, stop_idx);

    while (!stop_path[level]->is_root())
    {
        auto parent_idx = self.ctr_get_parent_idx(stop_path, level);
        self.ctr_remove_branch_content(stop_path, level + 1, 0, parent_idx);

        level++;
    }
}


M_PARAMS
void M_TYPE::ctr_remove_nodes_from_start(TreePathT& stop_path, const Position& stop_idx)
{
    auto& self = this->self();

    // FIXME: Must split the node here if remove fails
    assert_success(self.ctr_remove_leaf_content(stop_path, Position(0), stop_idx));

    if (!stop_path.leaf()->is_root())
    {
        auto parent_idx = self.ctr_get_parent_idx(stop_path, 0);

        self.ctr_remove_branch_nodes_from_start(stop_path, 1, parent_idx);
        self.ctr_remove_redundant_root(stop_path, 0);
    }
}


M_PARAMS
void M_TYPE::ctr_remove_branch_nodes_at_end(
        TreePathT& start_path,
        size_t level,
        int32_t start_idx
)
{
    auto& self = this->self();

    auto node_size = self.ctr_get_node_size(start_path[level], 0);
    self.ctr_remove_branch_content(start_path, level, start_idx, node_size);

    while (!start_path[level]->is_root())
    {
        auto parent_idx = self.ctr_get_parent_idx(start_path, level);

        auto node_size2 = self.ctr_get_node_size(start_path[level + 1], 0);
        self.ctr_remove_branch_content(start_path, level + 1, parent_idx + 1, node_size2);

        level++;
    }
}


M_PARAMS
void M_TYPE::ctr_remove_nodes_at_end(
        TreePathT& start_path,
        const Position& start_idx
)
{
    auto& self = this->self();

    auto node_sizes = self.ctr_get_node_sizes(start_path.leaf());

    // FIXME: Must split the node here if remove fails
    assert_success(self.ctr_remove_leaf_content(start_path, start_idx, node_sizes));

    if (start_path.size() > 1)
    {
        auto parent = self.ctr_get_node_parent_for_update(start_path, 0);

        auto parent_idx = self.ctr_get_child_idx(parent.as_immutable(), start_path.leaf()->id());

        self.ctr_remove_branch_nodes_at_end(start_path, 1, parent_idx + 1);

        return self.ctr_remove_redundant_root(start_path, 0);
    }
}

M_PARAMS
void M_TYPE::ctr_remove_nodes(
        TreePathT& start_path,
        const Position& start_idx,

        TreePathT& stop_path,
        Position& stop_idx
) {

    auto& self = this->self();

    if (self.ctr_is_the_same_sode(start_path.leaf(), stop_path.leaf()))
    {
        // The root node of removed subtree
        if ((stop_idx - start_idx).gtAny(0))
        {
            // FIXME: Must split the node here if remove fails
            assert_success(self.ctr_remove_leaf_content(start_path, start_idx, stop_idx));

            stop_idx = start_idx;
            return self.ctr_remove_redundant_root(start_path, 0);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        auto start_parent_idx = self.ctr_get_parent_idx(start_path, 0);
        auto stop_parent_idx = self.ctr_get_parent_idx(stop_path, 0);

        ctr_remove_branch_nodes(start_path, start_parent_idx + 1, stop_path, stop_parent_idx, 1);

        auto start_end = self.ctr_get_node_sizes(start_path.leaf());

        // FIXME: Must split the node here if remove fails
        assert_success(self.ctr_remove_leaf_content(start_path, start_idx, start_end));

        stop_path = start_path;
        self.ctr_expect_next_node(stop_path, 0);

        // FIXME: Must split the node here if remove fails
        assert_success(self.ctr_remove_leaf_content(stop_path, Position(0), stop_idx));

        start_path = stop_path;
        self.ctr_expect_prev_node(start_path, 0);

        auto merged = self.ctr_merge_leaf_nodes(start_path, stop_path);
        if (merged)
        {
            stop_path = start_path;
            stop_idx = start_idx;
        }
        else {
            stop_idx = Position{};
        }
    }
}






M_PARAMS
void M_TYPE::ctr_remove_branch_nodes(
            TreePathT& start_path,
            int32_t start_idx,
            TreePathT& stop_path,
            int32_t stop_idx,
            size_t level
)
{
    auto& self = this->self();

    if (self.ctr_is_the_same_sode(start_path[level], stop_path[level]))
    {
        // The root node of the removed subtree

        if (stop_idx - start_idx > 0)
        {
            //remove some space within the node
            self.ctr_remove_branch_content(start_path, level, start_idx, stop_idx);
            self.ctr_remove_redundant_root(start_path, level);

            self.ctr_assign_path_nodes(start_path, stop_path, level);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        auto start_parent_idx = self.ctr_get_parent_idx(start_path, level);
        auto stop_parent_idx = self.ctr_get_parent_idx(stop_path, level);

        ctr_remove_branch_nodes(start_path, start_parent_idx + 1, stop_path, stop_parent_idx, level + 1);

        auto start_end = self.ctr_get_node_size(start_path[level], 0);

        self.ctr_remove_branch_content(start_path, level, start_idx, start_end);
        self.ctr_assign_path_nodes(start_path, stop_path, level);
        self.ctr_expect_next_node(stop_path, level);

        self.ctr_remove_branch_content(stop_path, level, 0, stop_idx);
        self.ctr_assign_path_nodes(stop_path, start_path, level);
        self.ctr_expect_prev_node(start_path, level);

        auto merged = self.ctr_merge_branch_nodes(start_path, stop_path, level);
        if (merged)
        {
            self.ctr_assign_path_nodes(start_path, stop_path, level);
        }
    }
}


#undef M_TYPE
#undef M_PARAMS


}
