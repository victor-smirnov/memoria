
// Copyright Victor Smirnov 2011-2013.
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

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Metadata                                             Metadata;


    Position removeEntries(
            NodeBaseG& from,
            Position&  from_idx,
            NodeBaseG& to,
            Position&  to_idx,
            Accumulator& sums,
            bool merge                  = true
    );



    void removeAllNodes(NodeBaseG& start, NodeBaseG& stop, Accumulator& sums, Position& sizes);

    void removeNodesFromStart(NodeBaseG& stop, const Position& stop_idx, Accumulator& sums, Position& sizes);
    void removeNonLeafNodesFromStart(NodeBaseG& stop, Int stop_idx, Accumulator& sums, Position& sizes);

    void removeNodesAtEnd(NodeBaseG& start, const Position& start_idx, Accumulator& sums, Position& sizes);
    void removeNonLeafNodesAtEnd(NodeBaseG& start, Int start_idx, Accumulator& sums, Position& sizes);

    void removeNodes(
            NodeBaseG& start,
            const Position& start_idx,

            NodeBaseG& stop,
            Position& stop_idx,

            Accumulator& sums,
            Position& sizes
    );



    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    void removeNonLeafNodes(
            NodeBaseG& start,
            Int start_idx,
            NodeBaseG& stop,
            Int stop_idx,

            Accumulator& sums,
            Position& sizes
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
typename M_TYPE::Position M_TYPE::removeEntries(
        NodeBaseG& start,
        Position&  start_idx,
        NodeBaseG& stop,
        Position&  stop_idx,
        Accumulator& sums,
        bool merge
)
{
    auto& self = this->self();

    Position sizes;

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
        removeAllNodes(start, stop, sums, sizes);

        start_idx = stop_idx.setAll(0);
    }
    else if (from_start && !at_end)
    {
        removeNodesFromStart(stop, stop_idx, sums, sizes);

        if (merge)
        {
            self.mergeWithRightSibling(stop);
        }

        start       = stop;
        start_idx   = stop_idx = Position(0);
    }
    else if ((!from_start) && at_end)
    {
        removeNodesAtEnd(start, start_idx, sums, sizes);

        if (merge)
        {
            self.mergeWithLeftSibling(start, [&](const Position& left_sizes, Int level)
            {
                if (level == 0)
                {
                    start_idx += left_sizes;
                }
            });
        }

        stop        = start;
        stop_idx    = start_idx;
    }
    else {
        removeNodes(start, start_idx, stop, stop_idx, sums, sizes);

        if (merge)
        {
            self.mergeWithSiblings(stop, [&](const Position& left_sizes, Int level)
            {
                if (level == 0)
                {
                    stop_idx += left_sizes;
                }
            });
        }

        start       = stop;
        start_idx   = stop_idx;
    }

    self.addTotalKeyCount(-sizes);

    return sizes;
}




M_PARAMS
void M_TYPE::removeAllNodes(NodeBaseG& start, NodeBaseG& stop, Accumulator& sums, Position& sizes)
{
    auto& self = this->self();

    NodeBaseG node = start;

    while (!node->is_root()) {
        node = self.getNodeParent(node);
    }

    self.removeNode(node, sums, sizes);

    Metadata meta = self.getRootMetadata();

    NodeBaseG new_root = self.createRootNode1(0, true, meta);
    self.set_root(new_root->id());

    start = stop = new_root;
}


M_PARAMS
void M_TYPE::removeNonLeafNodesFromStart(NodeBaseG& stop, Int stop_idx, Accumulator& sums, Position& sizes)
{
    auto& self = this->self();

    MEMORIA_ASSERT(stop_idx, >=, 0);

    NodeBaseG node = stop;

    self.removeNodeContent(node, 0, stop_idx, sums, sizes);

    while (!node->is_root())
    {
        Int parent_idx = node->parent_idx();

        if (parent_idx > 0)
        {
            node = self.getNodeParentForUpdate(node);
            self.removeNodeContent(node, 0, parent_idx, sums, sizes);
        }
        else {
            break;
        }
    }
}


M_PARAMS
void M_TYPE::removeNodesFromStart(NodeBaseG& stop, const Position& stop_idx, Accumulator& sums, Position& sizes)
{
    auto& self = this->self();

    NodeBaseG node = stop;

    sizes += stop_idx;
    VectorAdd(sums, self.removeLeafContent(node, Position(0), stop_idx));

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);

        Int parent_idx = node->parent_idx();

        self.removeNonLeafNodesFromStart(parent, parent_idx, sums, sizes);

        self.removeRedundantRootP(node);
    }
}


M_PARAMS
void M_TYPE::removeNonLeafNodesAtEnd(NodeBaseG& start, Int start_idx, Accumulator& sums, Position& sizes)
{
    auto& self = this->self();

    NodeBaseG node = start;

    Int node_size = self.getNodeSize(node, 0);

    self.removeNodeContent(node, start_idx, node_size, sums, sizes);

    while (!node->is_root())
    {
        Int parent_idx  = node->parent_idx();

        node            = self.getNodeParentForUpdate(node);
        node_size       = self.getNodeSize(node, 0);

        if (parent_idx < node_size - 1)
        {
            self.removeNodeContent(node, parent_idx + 1, node_size, sums, sizes);
        }
        else {
            break;
        }
    }
}


M_PARAMS
void M_TYPE::removeNodesAtEnd(NodeBaseG& start, const Position& start_idx, Accumulator& sums, Position& sizes)
{
    auto& self = this->self();

    Position node_sizes = self.getNodeSizes(start);

    sizes += node_sizes - start_idx;
    VectorAdd(sums, self.removeLeafContent(start, start_idx, node_sizes));

    if (!start->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(start);

        self.removeNonLeafNodesAtEnd(parent, start->parent_idx() + 1, sums, sizes);

        self.removeRedundantRootP(start);
    }
}

M_PARAMS
void M_TYPE::removeNodes(
        NodeBaseG& start,
        const Position& start_idx,

        NodeBaseG& stop,
        Position& stop_idx,

        Accumulator& sums,
        Position& sizes
) {

    auto& self = this->self();

    if (self.isTheSameNode(start, stop))
    {
        // The root node of removed subtree

        if ((stop_idx - start_idx).gtAny(0))
        {
            //remove some space within the node
            sizes += stop_idx - start_idx;
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

        sizes += start_end - start_idx;
        VectorAdd(sums, self.removeLeafContent(start, start_idx, start_end));

        sizes += stop_idx;
        VectorAdd(sums, self.removeLeafContent(stop, Position(0), stop_idx));

        Int start_parent_idx    = start->parent_idx();
        Int stop_parent_idx     = stop->parent_idx();

        NodeBaseG start_parent  = self.getNodeParentForUpdate(start);
        NodeBaseG stop_parent   = self.getNodeParentForUpdate(stop);

        removeNonLeafNodes(start_parent, start_parent_idx + 1, stop_parent, stop_parent_idx, sums, sizes);

        if (self.isTheSameParent(start, stop))
        {
            if (self.canMerge(start, stop))
            {
                self.mergeNodes(start, stop);

                stop_idx    = start_idx;
                stop        = start;

                self.removeRedundantRootP(start);
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
void M_TYPE::removeNonLeafNodes(
            NodeBaseG& start,
            Int start_idx,
            NodeBaseG& stop,
            Int stop_idx,

            Accumulator& sums,
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
            self.removeNodeContent(start, start_idx, stop_idx, sums, sizes);

            self.removeRedundantRootP(start);
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        Int start_end = self.getNodeSize(start, 0);

        self.removeNodeContent(start, start_idx, start_end, sums, sizes);
        self.removeNodeContent(stop, 0, stop_idx, sums, sizes);

        Int start_parent_idx    = start->parent_idx();
        Int stop_parent_idx     = stop->parent_idx();

        NodeBaseG start_parent  = self.getNodeParentForUpdate(start);
        NodeBaseG stop_parent   = self.getNodeParentForUpdate(stop);

        removeNonLeafNodes(start_parent, start_parent_idx + 1, stop_parent, stop_parent_idx, sums, sizes);

        if (self.isTheSameParent(start, stop))
        {
            if (self.canMerge(start, stop))
            {
                self.mergeNodes(start, stop);

                stop            = start;
                stop_parent_idx = start_parent_idx;

                self.removeRedundantRootP(start);
            }
        }
    }
}







#undef M_TYPE
#undef M_PARAMS


}

#endif
