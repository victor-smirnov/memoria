
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

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bt::RemoveBatchName)

    typedef TypesType                                                           Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Metadata                                             Metadata;


    void removeEntries(
            NodeBaseG& from,
            Position&  from_idx,
            NodeBaseG& to,
            Position&  to_idx,
            Position& sizes,
            bool merge = true
    );



    void removeAllNodes(NodeBaseG& start, NodeBaseG& stop, Position& sums);

    void removeNodesFromStart(NodeBaseG& stop, const Position& stop_idx, Position& sums);
    void removeBranchNodesFromStart(NodeBaseG& stop, int32_t stop_idx, Position& sums);

    void removeNodesAtEnd(NodeBaseG& start, const Position& start_idx, Position& sums);
    void removeBranchNodesAtEnd(NodeBaseG& start, int32_t start_idx, Position& sums);

    void removeNodes(
            NodeBaseG& start,
            const Position& start_idx,

            NodeBaseG& stop,
            Position& stop_idx,

            Position& sums
    );

    void tryMergeNodesAfterRemove(
            NodeBaseG& start,
            const Position& start_idx,

            NodeBaseG& stop,
            Position& stop_idx)
    {

    }



    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    void removeBranchNodes(
            NodeBaseG& start,
            int32_t start_idx,
            NodeBaseG& stop,
            int32_t stop_idx,

            Position& sizes
    );



MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bt::RemoveBatchName)
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
 * \see removeAllPages, removePagesFromStart, removePagesAtEnd, removePages
 * \see mergeWithRightSibling, addTotalKeyCount
 */

M_PARAMS
void M_TYPE::removeEntries(
        NodeBaseG& start,
        Position&  start_idx,
        NodeBaseG& stop,
        Position&  stop_idx,
        Position& sizes,
        bool merge
)
{
    auto& self = this->self();

    Position stop_sizes = self.getNodeSizes(stop);

    bool at_end;

    if (stop_idx.ltAny(stop_sizes))
    {
        at_end = false;
    }
    else
    {
        auto next = self.getNextNodeP(stop);

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
        auto prev = self.getPrevNodeP(start);

        if (prev)
        {
            start       = prev;
            start_idx   = self.getNodeSizes(prev);

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
        removeAllNodes(start, stop, sizes);

        start_idx = stop_idx.setAll(0);
    }
    else if (from_start && !at_end)
    {
        removeNodesFromStart(stop, stop_idx, sizes);

        if (merge)
        {
            self.mergeLeafWithRightSibling(stop);
        }

        start       = stop;
        start_idx   = stop_idx = Position(0);
    }
    else if ((!from_start) && at_end)
    {
        removeNodesAtEnd(start, start_idx, sizes);

        if (merge)
        {
            self.mergeLeafWithLeftSibling(start, [&](const Position& left_sizes)
            {
                start_idx += left_sizes;
            });
        }

        stop        = start;
        stop_idx    = start_idx;
    }
    else {
        removeNodes(start, start_idx, stop, stop_idx, sizes);

        if (merge)
        {
            self.mergeLeafWithSiblings(stop, [&](const Position& left_sizes)
            {
                stop_idx += left_sizes;
            });
        }

        start       = stop;
        start_idx   = stop_idx;
    }

    //self.addTotalSizes(-self.getStreamSizes(sums));
}




M_PARAMS
void M_TYPE::removeAllNodes(NodeBaseG& start, NodeBaseG& stop, Position& sizes)
{
    auto& self = this->self();

    NodeBaseG node = start;

    while (!node->is_root()) {
        node = self.getNodeParent(node);
    }

    self.removeNodeRecursively(node, sizes);

    Metadata meta = self.getRootMetadata();

    NodeBaseG new_root = self.createRootNode(0, true, meta);
    self.set_root(new_root->id());

    start = stop = new_root;
}


M_PARAMS
void M_TYPE::removeBranchNodesFromStart(NodeBaseG& stop, int32_t stop_idx, Position& sizes)
{
    auto& self = this->self();

    MEMORIA_V1_ASSERT(stop_idx, >=, 0);

    NodeBaseG node = stop;

    self.removeNodeContent(node, 0, stop_idx, sizes);

    while (!node->is_root())
    {
        int32_t parent_idx = node->parent_idx();

        node = self.getNodeParentForUpdate(node);

        if (parent_idx > 0)
        {
            self.removeNodeContent(node, 0, parent_idx, sizes);
        }
    }
}


M_PARAMS
void M_TYPE::removeNodesFromStart(NodeBaseG& stop, const Position& stop_idx, Position& sizes)
{
    auto& self = this->self();

    NodeBaseG node = stop;

    sizes += self.removeLeafContent(node, Position(0), stop_idx);

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);

        int32_t parent_idx = node->parent_idx();

        self.removeBranchNodesFromStart(parent, parent_idx, sizes);

        self.removeRedundantRootP(node);
    }
}


M_PARAMS
void M_TYPE::removeBranchNodesAtEnd(NodeBaseG& start, int32_t start_idx, Position& sizes)
{
    auto& self = this->self();

    NodeBaseG node = start;

    int32_t node_size = self.getNodeSize(node, 0);

    self.removeNodeContent(node, start_idx, node_size, sizes);

    while (!node->is_root())
    {
        int32_t parent_idx  = node->parent_idx();

        node            = self.getNodeParentForUpdate(node);
        node_size       = self.getNodeSize(node, 0);

        if (parent_idx < node_size - 1)
        {
            self.removeNodeContent(node, parent_idx + 1, node_size, sizes);
        }
    }
}


M_PARAMS
void M_TYPE::removeNodesAtEnd(NodeBaseG& start, const Position& start_idx, Position& sizes)
{
    auto& self = this->self();

    Position node_sizes = self.getNodeSizes(start);

    sizes += self.removeLeafContent(start, start_idx, node_sizes);

    if (!start->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(start);

        self.removeBranchNodesAtEnd(parent, start->parent_idx() + 1, sizes);

        self.removeRedundantRootP(start);
    }
}

M_PARAMS
void M_TYPE::removeNodes(
        NodeBaseG& start,
        const Position& start_idx,

        NodeBaseG& stop,
        Position& stop_idx,

        Position& sizes
) {

    auto& self = this->self();

    if (self.isTheSameNode(start, stop))
    {
        // The root node of removed subtree

        if ((stop_idx - start_idx).gtAny(0))
        {
            //remove some space within the node
            sizes += self.removeLeafContent(start, start_idx, stop_idx);

            stop_idx = start_idx;

            self.removeRedundantRootP(start);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        Position start_end = self.getNodeSizes(start);

        sizes += self.removeLeafContent(start, start_idx, start_end);

        sizes += self.removeLeafContent(stop, Position(0), stop_idx);

        int32_t start_parent_idx    = start->parent_idx();
        int32_t stop_parent_idx     = stop->parent_idx();

        NodeBaseG start_parent  = self.getNodeParentForUpdate(start);
        NodeBaseG stop_parent   = self.getNodeParentForUpdate(stop);

        removeBranchNodes(start_parent, start_parent_idx + 1, stop_parent, stop_parent_idx, sizes);

        if (self.isTheSameParent(start, stop))
        {
            if (self.mergeCurrentLeafNodes(start, stop))
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
void M_TYPE::removeBranchNodes(
            NodeBaseG& start,
            int32_t start_idx,
            NodeBaseG& stop,
            int32_t stop_idx,

            Position& sizes
)
{
    auto& self = this->self();

    if (self.isTheSameNode(start, stop))
    {
        // The root node of removed subtree

        if (stop_idx - start_idx > 0)
        {
            //remove some space within the node
            self.removeNodeContent(start, start_idx, stop_idx, sizes);

            self.removeRedundantRootP(start);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        int32_t start_end = self.getNodeSize(start, 0);

        self.removeNodeContent(start, start_idx, start_end, sizes);
        self.removeNodeContent(stop, 0, stop_idx, sizes);

        int32_t start_parent_idx    = start->parent_idx();
        int32_t stop_parent_idx     = stop->parent_idx();

        NodeBaseG start_parent  = self.getNodeParentForUpdate(start);
        NodeBaseG stop_parent   = self.getNodeParentForUpdate(stop);

        removeBranchNodes(start_parent, start_parent_idx + 1, stop_parent, stop_parent_idx, sizes);

        if (self.isTheSameParent(start, stop))
        {
            if (self.mergeCurrentBranchNodes(start, stop))
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