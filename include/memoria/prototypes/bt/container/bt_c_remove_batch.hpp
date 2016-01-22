
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMBATCH_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMBATCH_HPP

#include <memoria/core/container/macros.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::RemoveBatchName)

    typedef TypesType                                                           Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Metadata                                             Metadata;


    void removeEntries(
            NodeBaseG& from,
            Position&  from_idx,
            NodeBaseG& to,
            Position&  to_idx,
            BranchNodeEntry& sums,
            bool merge                  = true
    );



    void removeAllNodes(NodeBaseG& start, NodeBaseG& stop, BranchNodeEntry& sums);

    void removeNodesFromStart(NodeBaseG& stop, const Position& stop_idx, BranchNodeEntry& sums);
    void removeBranchNodesFromStart(NodeBaseG& stop, Int stop_idx, BranchNodeEntry& sums);

    void removeNodesAtEnd(NodeBaseG& start, const Position& start_idx, BranchNodeEntry& sums);
    void removeBranchNodesAtEnd(NodeBaseG& start, Int start_idx, BranchNodeEntry& sums);

    void removeNodes(
            NodeBaseG& start,
            const Position& start_idx,

            NodeBaseG& stop,
            Position& stop_idx,

            BranchNodeEntry& sums
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
            Int start_idx,
            NodeBaseG& stop,
            Int stop_idx,

            BranchNodeEntry& sums
    );



MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::RemoveBatchName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



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
        BranchNodeEntry& sums,
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
        removeAllNodes(start, stop, sums);

        start_idx = stop_idx.setAll(0);
    }
    else if (from_start && !at_end)
    {
        removeNodesFromStart(stop, stop_idx, sums);

        if (merge)
        {
            self.mergeLeafWithRightSibling(stop);
        }

        start       = stop;
        start_idx   = stop_idx = Position(0);
    }
    else if ((!from_start) && at_end)
    {
        removeNodesAtEnd(start, start_idx, sums);

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
        removeNodes(start, start_idx, stop, stop_idx, sums);

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
void M_TYPE::removeAllNodes(NodeBaseG& start, NodeBaseG& stop, BranchNodeEntry& sums)
{
    auto& self = this->self();

    NodeBaseG node = start;

    while (!node->is_root()) {
        node = self.getNodeParent(node);
    }

    self.removeNode(node, sums);

    Metadata meta = self.getRootMetadata();

    NodeBaseG new_root = self.createRootNode1(0, true, meta);
    self.set_root(new_root->id());

    start = stop = new_root;
}


M_PARAMS
void M_TYPE::removeBranchNodesFromStart(NodeBaseG& stop, Int stop_idx, BranchNodeEntry& sums)
{
    auto& self = this->self();

    MEMORIA_ASSERT(stop_idx, >=, 0);

    NodeBaseG node = stop;

    self.removeNodeContent(node, 0, stop_idx, sums);

    while (!node->is_root())
    {
        Int parent_idx = node->parent_idx();

        node = self.getNodeParentForUpdate(node);

        if (parent_idx > 0)
        {
            self.removeNodeContent(node, 0, parent_idx, sums);
        }
    }
}


M_PARAMS
void M_TYPE::removeNodesFromStart(NodeBaseG& stop, const Position& stop_idx, BranchNodeEntry& sums)
{
    auto& self = this->self();

    NodeBaseG node = stop;

    VectorAdd(sums, self.removeLeafContent(node, Position(0), stop_idx));

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);

        Int parent_idx = node->parent_idx();

        self.removeBranchNodesFromStart(parent, parent_idx, sums);

        self.removeRedundantRootP(node);
    }
}


M_PARAMS
void M_TYPE::removeBranchNodesAtEnd(NodeBaseG& start, Int start_idx, BranchNodeEntry& sums)
{
    auto& self = this->self();

    NodeBaseG node = start;

    Int node_size = self.getNodeSize(node, 0);

    self.removeNodeContent(node, start_idx, node_size, sums);

    while (!node->is_root())
    {
        Int parent_idx  = node->parent_idx();

        node            = self.getNodeParentForUpdate(node);
        node_size       = self.getNodeSize(node, 0);

        if (parent_idx < node_size - 1)
        {
            self.removeNodeContent(node, parent_idx + 1, node_size, sums);
        }
    }
}


M_PARAMS
void M_TYPE::removeNodesAtEnd(NodeBaseG& start, const Position& start_idx, BranchNodeEntry& sums)
{
    auto& self = this->self();

    Position node_sizes = self.getNodeSizes(start);

    VectorAdd(sums, self.removeLeafContent(start, start_idx, node_sizes));

    if (!start->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(start);

        self.removeBranchNodesAtEnd(parent, start->parent_idx() + 1, sums);

        self.removeRedundantRootP(start);
    }
}

M_PARAMS
void M_TYPE::removeNodes(
        NodeBaseG& start,
        const Position& start_idx,

        NodeBaseG& stop,
        Position& stop_idx,

        BranchNodeEntry& sums
) {

    auto& self = this->self();

    if (self.isTheSameNode(start, stop))
    {
        // The root node of removed subtree

        if ((stop_idx - start_idx).gtAny(0))
        {
            //remove some space within the node
            VectorAdd(sums, self.removeLeafContent(start, start_idx, stop_idx));

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

        VectorAdd(sums, self.removeLeafContent(start, start_idx, start_end));

        VectorAdd(sums, self.removeLeafContent(stop, Position(0), stop_idx));

        Int start_parent_idx    = start->parent_idx();
        Int stop_parent_idx     = stop->parent_idx();

        NodeBaseG start_parent  = self.getNodeParentForUpdate(start);
        NodeBaseG stop_parent   = self.getNodeParentForUpdate(stop);

        removeBranchNodes(start_parent, start_parent_idx + 1, stop_parent, stop_parent_idx, sums);

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
            Int start_idx,
            NodeBaseG& stop,
            Int stop_idx,

            BranchNodeEntry& sums
)
{
    auto& self = this->self();

    if (self.isTheSameNode(start, stop))
    {
        // The root node of removed subtree

        if (stop_idx - start_idx > 0)
        {
            //remove some space within the node
            self.removeNodeContent(start, start_idx, stop_idx, sums);

            self.removeRedundantRootP(start);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        Int start_end = self.getNodeSize(start, 0);

        self.removeNodeContent(start, start_idx, start_end, sums);
        self.removeNodeContent(stop, 0, stop_idx, sums);

        Int start_parent_idx    = start->parent_idx();
        Int stop_parent_idx     = stop->parent_idx();

        NodeBaseG start_parent  = self.getNodeParentForUpdate(start);
        NodeBaseG stop_parent   = self.getNodeParentForUpdate(stop);

        removeBranchNodes(start_parent, start_parent_idx + 1, stop_parent, stop_parent_idx, sums);

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


}

#endif
