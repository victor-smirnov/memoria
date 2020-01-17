
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

#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/prototypes/bt/bt_names.hpp>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::RemoveBatchName)

    typedef TypesType                                                           Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Metadata                                             Metadata;


    void ctr_remove_entries(
            NodeBaseG& from,
            Position&  from_idx,
            NodeBaseG& to,
            Position&  to_idx,
            Position& sizes,
            bool merge = true
    );



    void ctr_remove_all_nodes(NodeBaseG& start, NodeBaseG& stop, Position& sums);

    void ctr_remove_nodes_from_start(NodeBaseG& stop, const Position& stop_idx, Position& sums);
    void ctr_remove_branch_nodes_from_start(NodeBaseG& stop, int32_t stop_idx, Position& sums);

    void ctr_remove_nodes_at_end(NodeBaseG& start, const Position& start_idx, Position& sums);
    void ctr_remove_branch_nodes_at_end(NodeBaseG& start, int32_t start_idx, Position& sums);

    void ctr_remove_nodes(
            NodeBaseG& start,
            const Position& start_idx,

            NodeBaseG& stop,
            Position& stop_idx,

            Position& sums
    );

    void ctr_try_merge_nodes_after_remove(
            NodeBaseG& start,
            const Position& start_idx,

            NodeBaseG& stop,
            Position& stop_idx)
    {

    }



    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    using Base::ctr_remove_branch_nodes;

    void ctr_remove_branch_nodes(
            NodeBaseG& start,
            int32_t start_idx,
            NodeBaseG& stop,
            int32_t stop_idx,

            Position& sizes
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
 * \see mergeWithRightSibling, addTotalKeyCount
 */

M_PARAMS
void M_TYPE::ctr_remove_entries(
        NodeBaseG& start,
        Position&  start_idx,
        NodeBaseG& stop,
        Position&  stop_idx,
        Position& sizes,
        bool merge
)
{
    auto& self = this->self();

    Position stop_sizes = self.ctr_get_node_sizes(stop);

    bool at_end;

    if (stop_idx.ltAny(stop_sizes))
    {
        at_end = false;
    }
    else
    {
        auto next = self.ctr_get_next_node(stop);

        if (next)
        {
            stop = next;
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
        auto prev = self.ctr_get_prev_node(start);

        if (prev)
        {
            start       = prev;
            start_idx   = self.ctr_get_node_sizes(prev);

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
        ctr_remove_all_nodes(start, stop, sizes);

        start_idx = stop_idx.setAll(0);
    }
    else if (from_start && !at_end)
    {
        ctr_remove_nodes_from_start(stop, stop_idx, sizes);

        if (merge)
        {
            self.ctr_merge_leaf_with_right_sibling(stop);
        }

        start       = stop;
        start_idx   = stop_idx = Position(0);
    }
    else if ((!from_start) && at_end)
    {
        ctr_remove_nodes_at_end(start, start_idx, sizes);

        if (merge)
        {
            self.ctr_merge_leaf_with_left_sibling(start, [&](const Position& left_sizes)
            {
                start_idx += left_sizes;
            });
        }

        stop        = start;
        stop_idx    = start_idx;
    }
    else {
        ctr_remove_nodes(start, start_idx, stop, stop_idx, sizes);

        if (merge)
        {
            self.ctr_merge_leaf_with_siblings(stop, [&](const Position& left_sizes)
            {
                stop_idx += left_sizes;
            });
        }

        start       = stop;
        start_idx   = stop_idx;
    }

    //self.addTotalSizes(-self.get_get_stream_sizes(sums));
}




M_PARAMS
void M_TYPE::ctr_remove_all_nodes(NodeBaseG& start, NodeBaseG& stop, Position& sizes)
{
    auto& self = this->self();

    NodeBaseG node = start;

    while (!node->is_root()) {
        node = self.ctr_get_node_parent(node);
    }

    NodeBaseG new_root = self.ctr_create_root_node(0, true, -1);

    self.ctr_remove_node_recursively(node, sizes);

    self.set_root(new_root->id()).terminate_if_error();

    start = stop = new_root;
}


M_PARAMS
void M_TYPE::ctr_remove_branch_nodes_from_start(NodeBaseG& stop, int32_t stop_idx, Position& sizes)
{
    auto& self = this->self();

    MEMORIA_V1_ASSERT(stop_idx, >=, 0);

    NodeBaseG node = stop;

    self.ctr_remove_leaf_content(node, 0, stop_idx, sizes);

    while (!node->is_root())
    {
        int32_t parent_idx = node->parent_idx();

        node = self.ctr_get_node_parent_for_update(node);

        if (parent_idx > 0)
        {
            self.ctr_remove_leaf_content(node, 0, parent_idx, sizes);
        }
    }
}


M_PARAMS
void M_TYPE::ctr_remove_nodes_from_start(NodeBaseG& stop, const Position& stop_idx, Position& sizes)
{
    auto& self = this->self();

    NodeBaseG node = stop;

    sizes += self.ctr_remove_leaf_content(node, Position(0), stop_idx);

    if (!node->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent_for_update(node);

        int32_t parent_idx = node->parent_idx();

        self.ctr_remove_branch_nodes_from_start(parent, parent_idx, sizes);

        self.ctr_remove_redundant_root(node);
    }
}


M_PARAMS
void M_TYPE::ctr_remove_branch_nodes_at_end(NodeBaseG& start, int32_t start_idx, Position& sizes)
{
    auto& self = this->self();

    NodeBaseG node = start;

    int32_t node_size = self.ctr_get_node_size(node, 0);

    self.ctr_remove_leaf_content(node, start_idx, node_size, sizes);

    while (!node->is_root())
    {
        int32_t parent_idx  = node->parent_idx();

        node            = self.ctr_get_node_parent_for_update(node);
        node_size       = self.ctr_get_node_size(node, 0);

        if (parent_idx < node_size - 1)
        {
            self.ctr_remove_leaf_content(node, parent_idx + 1, node_size, sizes);
        }
    }
}


M_PARAMS
void M_TYPE::ctr_remove_nodes_at_end(NodeBaseG& start, const Position& start_idx, Position& sizes)
{
    auto& self = this->self();

    Position node_sizes = self.ctr_get_node_sizes(start);

    sizes += self.ctr_remove_leaf_content(start, start_idx, node_sizes);

    if (!start->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent_for_update(start);

        self.ctr_remove_branch_nodes_at_end(parent, start->parent_idx() + 1, sizes);

        self.ctr_remove_redundant_root(start);
    }
}

M_PARAMS
void M_TYPE::ctr_remove_nodes(
        NodeBaseG& start,
        const Position& start_idx,

        NodeBaseG& stop,
        Position& stop_idx,

        Position& sizes
) {

    auto& self = this->self();

    if (self.ctr_is_the_same_sode(start, stop))
    {
        // The root node of removed subtree

        if ((stop_idx - start_idx).gtAny(0))
        {
            //remove some space within the node
            sizes += self.ctr_remove_leaf_content(start, start_idx, stop_idx);

            stop_idx = start_idx;

            self.ctr_remove_redundant_root(start);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        Position start_end = self.ctr_get_node_sizes(start);

        sizes += self.ctr_remove_leaf_content(start, start_idx, start_end);

        sizes += self.ctr_remove_leaf_content(stop, Position(0), stop_idx);

        int32_t start_parent_idx    = start->parent_idx();
        int32_t stop_parent_idx     = stop->parent_idx();

        NodeBaseG start_parent  = self.ctr_get_node_parent_for_update(start);
        NodeBaseG stop_parent   = self.ctr_get_node_parent_for_update(stop);

        ctr_remove_branch_nodes(start_parent, start_parent_idx + 1, stop_parent, stop_parent_idx, sizes);

        if (self.ctr_is_the_same_parent(start, stop))
        {
            if (self.ctr_merge_current_leaf_nodes(start, stop))
            {
                stop_idx    = start_idx;
                stop        = start;
            }
            else {
                stop_idx = Position(0);
            }
        }
        else {
            stop_idx = Position(0);
        }
    }
}






M_PARAMS
void M_TYPE::ctr_remove_branch_nodes(
            NodeBaseG& start,
            int32_t start_idx,
            NodeBaseG& stop,
            int32_t stop_idx,

            Position& sizes
)
{
    auto& self = this->self();

    if (self.ctr_is_the_same_sode(start, stop))
    {
        // The root node of removed subtree

        if (stop_idx - start_idx > 0)
        {
            //remove some space within the node
            self.ctr_remove_leaf_content(start, start_idx, stop_idx, sizes);

            self.ctr_remove_redundant_root(start);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        int32_t start_end = self.ctr_get_node_size(start, 0);

        self.ctr_remove_leaf_content(start, start_idx, start_end, sizes);
        self.ctr_remove_leaf_content(stop, 0, stop_idx, sizes);

        int32_t start_parent_idx    = start->parent_idx();
        int32_t stop_parent_idx     = stop->parent_idx();

        NodeBaseG start_parent  = self.ctr_get_node_parent_for_update(start);
        NodeBaseG stop_parent   = self.ctr_get_node_parent_for_update(stop);

        ctr_remove_branch_nodes(start_parent, start_parent_idx + 1, stop_parent, stop_parent_idx, sizes);

        if (self.ctr_is_the_same_parent(start, stop))
        {
            if (self.ctr_merge_current_branch_nodes(start, stop))
            {
                stop            = start;
                stop_parent_idx = start_parent_idx;
            }
        }
    }
}







#undef M_TYPE
#undef M_PARAMS


}}
