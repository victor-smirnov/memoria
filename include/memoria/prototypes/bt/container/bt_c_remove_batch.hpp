
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

    typedef TypesType                                                           Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Metadata                                             Metadata;

    using typename Base::TreePathT;


    VoidResult ctr_remove_entries(
            TreePathT& from_path,
            Position&  from_idx,
            TreePathT& to_path,
            Position&  to_idx,
            Position& sizes,
            bool merge = true
    ) noexcept;



    VoidResult ctr_remove_all_nodes(
            TreePathT& start_path,
            TreePathT& stop_path,
            Position& sums
    ) noexcept;

    VoidResult ctr_remove_nodes_from_start(
            TreePathT& stop,
            const Position& stop_idx,
            Position& sums
    ) noexcept;

    VoidResult ctr_remove_branch_nodes_from_start(
            TreePathT& stop_path,
            size_t level,
            int32_t stop_idx,
            Position& sums
    ) noexcept;


    VoidResult ctr_remove_nodes_at_end(
            TreePathT& start_path,
            const Position& start_idx,
            Position& sums
    ) noexcept;

    VoidResult ctr_remove_branch_nodes_at_end(
            TreePathT& start_path,
            size_t level,
            int32_t start_idx,
            Position& sums
    ) noexcept;

    VoidResult ctr_remove_nodes(
            TreePathT& start_path,
            const Position& start_idx,

            TreePathT& stop_path,
            Position& stop_idx,
            Position& sums
    ) noexcept;

//    VoidResult ctr_try_merge_nodes_after_remove(
//            NodeBaseG& start,
//            const Position& start_idx,

//            NodeBaseG& stop,
//            Position& stop_idx) noexcept
//    {

//    }



    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    using Base::ctr_remove_branch_nodes;

    VoidResult ctr_remove_branch_nodes(
            TreePathT& start_path,
            int32_t start_idx,
            TreePathT& stop_path,
            int32_t stop_idx,
            size_t level,
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
        TreePathT& start_path,
        Position&  start_idx,
        TreePathT& stop_path,
        Position&  stop_idx,
        Position& sizes,
        bool merge
) noexcept
{
    auto& self = this->self();

    Position stop_sizes = self.ctr_get_node_sizes(stop_path.leaf());

    bool at_end;

    if (stop_idx.ltAny(stop_sizes))
    {
        at_end = false;
    }
    else
    {
        MEMORIA_TRY(has_next_node, self.ctr_get_next_node(stop_path, 0));

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
        MEMORIA_TRY(has_prev_node, self.ctr_get_prev_node(start_path, 0));

        if (has_prev_node)
        {
            start_idx   = self.ctr_get_node_sizes(start_path.leaf());
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
        MEMORIA_TRY_VOID(ctr_remove_all_nodes(start_path, stop_path, sizes));
        start_idx = stop_idx.setAll(0);
    }
    else if (from_start && !at_end)
    {
        MEMORIA_TRY_VOID(ctr_remove_nodes_from_start(stop_path, stop_idx, sizes));

        if (merge)
        {
            auto res = self.ctr_merge_leaf_with_right_sibling(stop_path);
            MEMORIA_RETURN_IF_ERROR(res);
        }

        start_path  = stop_path;
        start_idx   = stop_idx = Position(0);
    }
    else if ((!from_start) && at_end)
    {
        MEMORIA_TRY_VOID(ctr_remove_nodes_at_end(start_path, start_idx, sizes));

        if (merge)
        {
            auto res = self.ctr_merge_leaf_with_left_sibling(start_path, [&](const Position& left_sizes)
            {
                start_idx += left_sizes;
                return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res);
        }

        stop_path   = start_path;
        stop_idx    = start_idx;
    }
    else {
        MEMORIA_TRY_VOID(ctr_remove_nodes(start_path, start_idx, stop_path, stop_idx, sizes));

        if (merge)
        {
            auto res = self.ctr_merge_leaf_with_siblings(stop_path, [&](const Position& left_sizes)
            {
                stop_idx += left_sizes;
                return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res);
        }

        start_path  = stop_path;
        start_idx   = stop_idx;
    }

    //self.addTotalSizes(-self.get_get_stream_sizes(sums));
    return VoidResult::of();
}




M_PARAMS
VoidResult M_TYPE::ctr_remove_all_nodes(TreePathT& start_path, TreePathT& stop_path, Position& sizes) noexcept
{
    auto& self = this->self();

    NodeBaseG root = start_path.root();
    start_path.clear();
    stop_path.clear();

    MEMORIA_TRY(new_root, self.ctr_create_root_node(0, true, -1));

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_node_recursively(root, sizes));

    MEMORIA_RETURN_IF_ERROR_FN(self.set_root(new_root->id()));

    start_path.add_root(new_root);

    if (stop_path.size() == 0)
    {
        stop_path.add_root(new_root);
    }

    return VoidResult::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_branch_nodes_from_start(TreePathT& stop_path, size_t level, int32_t stop_idx, Position& sizes) noexcept
{
    auto& self = this->self();

    NodeBaseG node = stop_path.leaf();

    MEMORIA_TRY_VOID(self.ctr_remove_node_content(stop_path, level, 0, stop_idx, sizes));

    while (!node->is_root())
    {
        MEMORIA_TRY(parent, self.ctr_get_node_parent_for_update(stop_path, level));

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent, node->id()));

        node = parent;

        if (parent_idx > 0)
        {
            MEMORIA_TRY_VOID(self.ctr_remove_node_content(stop_path, level, 0, parent_idx, sizes));
        }

        level++;
    }

    return VoidResult::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_nodes_from_start(TreePathT& stop_path, const Position& stop_idx, Position& sizes) noexcept
{
    auto& self = this->self();


    auto res = self.ctr_remove_leaf_content(stop_path, Position(0), stop_idx);
    MEMORIA_RETURN_IF_ERROR(res);

    sizes += res.get();

    if (!stop_path.leaf()->is_root())
    {
        Result<NodeBaseG> parent = self.ctr_get_node_parent_for_update(stop_path, 0);
        MEMORIA_RETURN_IF_ERROR(parent);

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent.get(), stop_path[0]->id()));

        MEMORIA_TRY_VOID(self.ctr_remove_branch_nodes_from_start(stop_path, 1, parent_idx, sizes));

        MEMORIA_TRY_VOID(self.ctr_remove_redundant_root(stop_path));
    }

    return VoidResult::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_branch_nodes_at_end(
        TreePathT& start_path,
        size_t level,
        int32_t start_idx,
        Position& sizes
) noexcept
{
    auto& self = this->self();

    NodeBaseG node = start_path[level];

    int32_t node_size = self.ctr_get_node_size(node, 0);
    MEMORIA_TRY_VOID(self.ctr_remove_node_content(start_path, level, start_idx, node_size, sizes));

    while (!node->is_root())
    {
        MEMORIA_TRY(parent, self.ctr_get_node_parent_for_update(start_path, level));
        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent, node->id()));

        node        = parent;
        node_size   = self.ctr_get_node_size(node, 0);

        if (parent_idx < node_size - 1)
        {
            MEMORIA_TRY_VOID(self.ctr_remove_node_content(start_path, level + 1, parent_idx + 1, node_size, sizes));
        }

        level++;
    }

    return VoidResult::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_remove_nodes_at_end(
        TreePathT& start_path,
        const Position& start_idx,
        Position& sizes
) noexcept
{
    auto& self = this->self();

    Position node_sizes = self.ctr_get_node_sizes(start_path.leaf());

    MEMORIA_TRY(leaf_sizes, self.ctr_remove_leaf_content(start_path, start_idx, node_sizes));
    sizes += leaf_sizes;

    if (start_path.size() > 1)
    {
        MEMORIA_TRY(parent, self.ctr_get_node_parent_for_update(start_path, 0));

        MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(parent, start_path.leaf()->id()));

        MEMORIA_TRY_VOID(self.ctr_remove_branch_nodes_at_end(start_path, 1, parent_idx + 1, sizes));

        return self.ctr_remove_redundant_root(start_path, 0);
    }

    return VoidResult::of();
}

M_PARAMS
VoidResult M_TYPE::ctr_remove_nodes(
        TreePathT& start_path,
        const Position& start_idx,

        TreePathT& stop_path,
        Position& stop_idx,
        Position& sizes
) noexcept {

    auto& self = this->self();

    if (self.ctr_is_the_same_sode(start_path.leaf(), stop_path.leaf()))
    {
        // The root node of removed subtree

        if ((stop_idx - start_idx).gtAny(0))
        {
            //remove some space within the node
            MEMORIA_TRY(leaf_sizes, self.ctr_remove_leaf_content(start_path, start_idx, stop_idx));
            sizes += leaf_sizes;

            stop_idx = start_idx;

            return self.ctr_remove_redundant_root(start_path, 0);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        Position start_end = self.ctr_get_node_sizes(start_path.leaf());

        MEMORIA_TRY(start_leaf_sizes, self.ctr_remove_leaf_content(start_path, start_idx, start_end));
        sizes += start_leaf_sizes;

        MEMORIA_TRY(stop_leaf_sizes, self.ctr_remove_leaf_content(stop_path, Position(0), stop_idx));
        sizes += stop_leaf_sizes;

        MEMORIA_TRY(start_parent, self.ctr_get_node_parent_for_update(start_path, 0));
        MEMORIA_TRY(stop_parent, self.ctr_get_node_parent_for_update(stop_path, 0));

        MEMORIA_TRY(start_parent_idx, self.ctr_get_child_idx(start_parent, start_path.leaf()->id()));
        MEMORIA_TRY(stop_parent_idx, self.ctr_get_child_idx(stop_parent, stop_path.leaf()->id()));

        MEMORIA_RETURN_IF_ERROR_FN(
            ctr_remove_branch_nodes(start_path, start_parent_idx + 1, stop_path, stop_parent_idx, 1, sizes)
        );

        MEMORIA_TRY(same_parent, self.ctr_is_the_same_parent(start_path, stop_path, 0));
        if (same_parent)
        {
            auto res2 = self.ctr_merge_current_leaf_nodes(start_path, stop_path);
            MEMORIA_RETURN_IF_ERROR(res2);

            if (res2.get())
            {
                stop_idx    = start_idx;
                stop_path   = start_path;
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
            TreePathT& start_path,
            int32_t start_idx,
            TreePathT& stop_path,
            int32_t stop_idx,
            size_t level,
            Position& sizes
) noexcept
{
    auto& self = this->self();

    NodeBaseG start = start_path[level];
    NodeBaseG stop  = stop_path[level];

    if (self.ctr_is_the_same_sode(start, stop))
    {
        // The root node of removed subtree

        if (stop_idx - start_idx > 0)
        {
            //remove some space within the node
            MEMORIA_TRY_VOID(self.ctr_remove_node_content(start_path, level, start_idx, stop_idx, sizes));

            MEMORIA_TRY_VOID(self.ctr_remove_redundant_root(start_path, level));
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        int32_t start_end = self.ctr_get_node_size(start, 0);

        MEMORIA_TRY_VOID(self.ctr_remove_node_content(start_path, level, start_idx, start_end, sizes));
        MEMORIA_TRY_VOID(self.ctr_remove_node_content(stop_path, level, 0, stop_idx, sizes));

        MEMORIA_TRY(start_parent, self.ctr_get_node_parent_for_update(start_path, level));
        MEMORIA_TRY(stop_parent, self.ctr_get_node_parent_for_update(stop_path, level));

        MEMORIA_TRY(start_parent_idx, self.ctr_get_child_idx(start_parent, start->id()));
        MEMORIA_TRY(stop_parent_idx, self.ctr_get_child_idx(stop_parent, stop->id()));

        auto res0 = ctr_remove_branch_nodes(start_path, start_parent_idx + 1, stop_path, stop_parent_idx, level + 1, sizes);
        MEMORIA_RETURN_IF_ERROR(res0);

        MEMORIA_TRY(same_parent, self.ctr_is_the_same_parent(start_path, stop_path, level));

        if (same_parent)
        {
            auto res1 = self.ctr_merge_current_branch_nodes(start_path, stop_path, level);
            MEMORIA_RETURN_IF_ERROR(res1);

            if (res1.get())
            {
                stop_path = start_path;
                stop_parent_idx = start_parent_idx;
            }
        }
    }

    return VoidResult::of();
}


#undef M_TYPE
#undef M_PARAMS


}
