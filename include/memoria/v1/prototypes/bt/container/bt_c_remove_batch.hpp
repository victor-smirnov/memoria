
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


    VoidResult ctr_remove_entries(
            NodeBaseG& from,
            Position&  from_idx,
            NodeBaseG& to,
            Position&  to_idx,
            Position& sizes,
            bool merge = true
    ) noexcept;



    VoidResult ctr_remove_all_nodes(NodeBaseG& start, NodeBaseG& stop, Position& sums) noexcept;

    VoidResult ctr_remove_nodes_from_start(NodeBaseG& stop, const Position& stop_idx, Position& sums) noexcept;
    VoidResult ctr_remove_branch_nodes_from_start(NodeBaseG& stop, int32_t stop_idx, Position& sums) noexcept;

    VoidResult ctr_remove_nodes_at_end(NodeBaseG& start, const Position& start_idx, Position& sums) noexcept;
    VoidResult ctr_remove_branch_nodes_at_end(NodeBaseG& start, int32_t start_idx, Position& sums) noexcept;

    VoidResult ctr_remove_nodes(
            NodeBaseG& start,
            const Position& start_idx,

            NodeBaseG& stop,
            Position& stop_idx,

            Position& sums
    ) noexcept;

    VoidResult ctr_try_merge_nodes_after_remove(
            NodeBaseG& start,
            const Position& start_idx,

            NodeBaseG& stop,
            Position& stop_idx) noexcept
    {

    }



    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    using Base::ctr_remove_branch_nodes;

    VoidResult ctr_remove_branch_nodes(
            NodeBaseG& start,
            int32_t start_idx,
            NodeBaseG& stop,
            int32_t stop_idx,

            Position& sizes
    ) noexcept;



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
VoidResult M_TYPE::ctr_remove_entries(
        NodeBaseG& start,
        Position&  start_idx,
        NodeBaseG& stop,
        Position&  stop_idx,
        Position& sizes,
        bool merge
) noexcept
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
        Result<NodeBaseG> next = self.ctr_get_next_node(stop);
        MEMORIA_RETURN_IF_ERROR(next);

        if (next.get())
        {
            stop = next.get();
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
        Result<NodeBaseG> prev = self.ctr_get_prev_node(start);
        MEMORIA_RETURN_IF_ERROR(prev);

        if (prev.get())
        {
            start       = prev.get();
            start_idx   = self.ctr_get_node_sizes(prev.get());

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
        MEMORIA_RETURN_IF_ERROR_FN(ctr_remove_all_nodes(start, stop, sizes));

        start_idx = stop_idx.setAll(0);
    }
    else if (from_start && !at_end)
    {
        MEMORIA_RETURN_IF_ERROR_FN(ctr_remove_nodes_from_start(stop, stop_idx, sizes));

        if (merge)
        {
            auto res = self.ctr_merge_leaf_with_right_sibling(stop);
            MEMORIA_RETURN_IF_ERROR(res);
        }

        start       = stop;
        start_idx   = stop_idx = Position(0);
    }
    else if ((!from_start) && at_end)
    {
        MEMORIA_RETURN_IF_ERROR_FN(ctr_remove_nodes_at_end(start, start_idx, sizes));

        if (merge)
        {
            auto res = self.ctr_merge_leaf_with_left_sibling(start, [&](const Position& left_sizes)
            {
                start_idx += left_sizes;
                return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res);
        }

        stop        = start;
        stop_idx    = start_idx;
    }
    else {
        MEMORIA_RETURN_IF_ERROR_FN(ctr_remove_nodes(start, start_idx, stop, stop_idx, sizes));

        if (merge)
        {
            auto res = self.ctr_merge_leaf_with_siblings(stop, [&](const Position& left_sizes)
            {
                stop_idx += left_sizes;
                return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res);
        }

        start       = stop;
        start_idx   = stop_idx;
    }

    //self.addTotalSizes(-self.get_get_stream_sizes(sums));
    return VoidResult::of();
}




M_PARAMS
VoidResult M_TYPE::ctr_remove_all_nodes(NodeBaseG& start, NodeBaseG& stop, Position& sizes) noexcept
{
    auto& self = this->self();

    NodeBaseG node = start;

    while (!node->is_root())
    {
        auto parent = self.ctr_get_node_parent(node);
        MEMORIA_RETURN_IF_ERROR(parent);
        node = parent.get();
    }

    Result<NodeBaseG> new_root = self.ctr_create_root_node(0, true, -1);
    MEMORIA_RETURN_IF_ERROR(new_root);

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_node_recursively(node, sizes));

    MEMORIA_RETURN_IF_ERROR_FN(self.set_root(new_root.get()->id()));

    start = stop = new_root.get();

    return VoidResult::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_branch_nodes_from_start(NodeBaseG& stop, int32_t stop_idx, Position& sizes) noexcept
{
    auto& self = this->self();

    //MEMORIA_V1_ASSERT(stop_idx, >=, 0);

    NodeBaseG node = stop;

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_leaf_content(node, 0, stop_idx, sizes));

    while (!node->is_root())
    {
        int32_t parent_idx = node->parent_idx();

        Result<NodeBaseG> parent = self.ctr_get_node_parent_for_update(node);
        MEMORIA_RETURN_IF_ERROR(parent);

        node = parent.get();

        if (parent_idx > 0)
        {
            return self.ctr_remove_leaf_content(node, 0, parent_idx, sizes);
        }
    }

    return VoidResult::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_nodes_from_start(NodeBaseG& stop, const Position& stop_idx, Position& sizes) noexcept
{
    auto& self = this->self();

    NodeBaseG node = stop;

    auto res = self.ctr_remove_leaf_content(node, Position(0), stop_idx);
    MEMORIA_RETURN_IF_ERROR(res);

    sizes += res.get();

    if (!node->is_root())
    {
        Result<NodeBaseG> parent = self.ctr_get_node_parent_for_update(node);
        MEMORIA_RETURN_IF_ERROR(parent);

        int32_t parent_idx = node->parent_idx();

        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_branch_nodes_from_start(parent.get(), parent_idx, sizes));

        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_redundant_root(node));
    }

    return VoidResult::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_branch_nodes_at_end(NodeBaseG& start, int32_t start_idx, Position& sizes) noexcept
{
    auto& self = this->self();

    NodeBaseG node = start;

    int32_t node_size = self.ctr_get_node_size(node, 0);

    auto res = self.ctr_remove_leaf_content(node, start_idx, node_size, sizes);
    MEMORIA_RETURN_IF_ERROR(res);

    while (!node->is_root())
    {
        int32_t parent_idx  = node->parent_idx();

        Result<NodeBaseG> parent = self.ctr_get_node_parent_for_update(node);
        MEMORIA_RETURN_IF_ERROR(parent);

        node            = parent.get();
        node_size       = self.ctr_get_node_size(node, 0);

        if (parent_idx < node_size - 1)
        {
            return self.ctr_remove_leaf_content(node, parent_idx + 1, node_size, sizes);
        }
    }

    return VoidResult::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_nodes_at_end(NodeBaseG& start, const Position& start_idx, Position& sizes) noexcept
{
    auto& self = this->self();

    Position node_sizes = self.ctr_get_node_sizes(start);

    auto res = self.ctr_remove_leaf_content(start, start_idx, node_sizes);
    MEMORIA_RETURN_IF_ERROR(res);

    sizes += res.get();

    if (!start->is_root())
    {
        Result<NodeBaseG> parent = self.ctr_get_node_parent_for_update(start);
        MEMORIA_RETURN_IF_ERROR(parent);

        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_branch_nodes_at_end(parent.get(), start->parent_idx() + 1, sizes));

        return self.ctr_remove_redundant_root(start);
    }

    return VoidResult::of();
}

M_PARAMS
VoidResult M_TYPE::ctr_remove_nodes(
        NodeBaseG& start,
        const Position& start_idx,

        NodeBaseG& stop,
        Position& stop_idx,

        Position& sizes
) noexcept {

    auto& self = this->self();

    if (self.ctr_is_the_same_sode(start, stop))
    {
        // The root node of removed subtree

        if ((stop_idx - start_idx).gtAny(0))
        {
            //remove some space within the node
            auto res = self.ctr_remove_leaf_content(start, start_idx, stop_idx);
            MEMORIA_RETURN_IF_ERROR(res);

            sizes += res.get();

            stop_idx = start_idx;

            return self.ctr_remove_redundant_root(start);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        Position start_end = self.ctr_get_node_sizes(start);

        auto res0 = self.ctr_remove_leaf_content(start, start_idx, start_end);
        MEMORIA_RETURN_IF_ERROR(res0);

        sizes += res0.get();

        auto res1 = self.ctr_remove_leaf_content(stop, Position(0), stop_idx);
        MEMORIA_RETURN_IF_ERROR(res1);

        sizes += res1.get();

        int32_t start_parent_idx    = start->parent_idx();
        int32_t stop_parent_idx     = stop->parent_idx();

        Result<NodeBaseG> start_parent  = self.ctr_get_node_parent_for_update(start);
        MEMORIA_RETURN_IF_ERROR(start_parent);


        Result<NodeBaseG> stop_parent   = self.ctr_get_node_parent_for_update(stop);
        MEMORIA_RETURN_IF_ERROR(stop_parent);

        MEMORIA_RETURN_IF_ERROR_FN(
            ctr_remove_branch_nodes(start_parent.get(), start_parent_idx + 1, stop_parent.get(), stop_parent_idx, sizes)
        );

        if (self.ctr_is_the_same_parent(start, stop))
        {
            auto res2 = self.ctr_merge_current_leaf_nodes(start, stop);
            MEMORIA_RETURN_IF_ERROR(res2);

            if (res2.get())
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

    return VoidResult::of();
}






M_PARAMS
VoidResult M_TYPE::ctr_remove_branch_nodes(
            NodeBaseG& start,
            int32_t start_idx,
            NodeBaseG& stop,
            int32_t stop_idx,

            Position& sizes
) noexcept
{
    auto& self = this->self();

    if (self.ctr_is_the_same_sode(start, stop))
    {
        // The root node of removed subtree

        if (stop_idx - start_idx > 0)
        {
            //remove some space within the node
            MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_leaf_content(start, start_idx, stop_idx, sizes));

            MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_redundant_root(start));
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        int32_t start_end = self.ctr_get_node_size(start, 0);

        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_leaf_content(start, start_idx, start_end, sizes));
        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_leaf_content(stop, 0, stop_idx, sizes));

        int32_t start_parent_idx    = start->parent_idx();
        int32_t stop_parent_idx     = stop->parent_idx();

        Result<NodeBaseG> start_parent = self.ctr_get_node_parent_for_update(start);
        MEMORIA_RETURN_IF_ERROR(start_parent);

        Result<NodeBaseG> stop_parent = self.ctr_get_node_parent_for_update(stop);
        MEMORIA_RETURN_IF_ERROR(stop_parent);

        auto res0 = ctr_remove_branch_nodes(start_parent.get(), start_parent_idx + 1, stop_parent.get(), stop_parent_idx, sizes);
        MEMORIA_RETURN_IF_ERROR(res0);

        if (self.ctr_is_the_same_parent(start, stop))
        {
            auto res1 = self.ctr_merge_current_branch_nodes(start, stop);
            MEMORIA_RETURN_IF_ERROR(res1);

            if (res1.get())
            {
                stop            = start;
                stop_parent_idx = start_parent_idx;
            }
        }
    }

    return VoidResult::of();
}


#undef M_TYPE
#undef M_PARAMS


}}
