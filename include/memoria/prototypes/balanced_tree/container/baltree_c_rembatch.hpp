
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMBATCH_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMBATCH_HPP

#include <memoria/core/container/macros.hpp>
#include <memoria/prototypes/balanced_tree/baltree_types.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::RemoveBatchName)

	typedef TypesType															Types;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::ID                                                   ID;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                         	Position;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int  Indexes                                                   = Base::Indexes;

    











    Position removeEntries(
    		TreePath& from,
    		Position& from_idx,
    		TreePath& to,
    		Position& to_idx,
    		Accumulator& accum,
    		bool merge 					= true
    );



    void removeAllPages(TreePath& start, TreePath& stop, Accumulator& accum, Position& removed_key_count);

    void removePagesFromStart(TreePath& stop, Position& stop_idx, Accumulator& accum, Position& removed_key_count);

    void removePagesAtEnd(TreePath& start, Position& start_idx, Accumulator& accum, Position& removed_key_count);

    void removePages(
            TreePath& start,
            Position& start_idx,
            TreePath& stop,
            Position& stop_idx,
            Int level,
            Accumulator& accum,
            Position& removed_key_count
    );




    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    void removePagesInternal(
            TreePath& start,
            Position& start_idx,
            TreePath& stop,
            Position& stop_idx,
            Int level,
            Accumulator& accum,
            Position& removed_key_count
    );



MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::RemoveBatchName)
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
		TreePath& start,
		Position& start_idx,
		TreePath& stop,
		Position& stop_idx,
		Accumulator& keys,
		bool merge
)
{
	auto& self = this->self();

    Position removed_key_count;

    Position stop_sizes = self.getNodeSizes(stop.leaf());

    bool at_end;

    if (stop_idx.ltAny(stop_sizes))
    {
    	at_end = false;
    }
    else
    {
    	auto stop_tmp = stop;
    	at_end = !self.getNextNode(stop_tmp, 0);
    }


    bool from_start;

    if (start_idx.eqAll(0))
    {
        if (self.getPrevNode(start, 0))
        {
            start_idx = self.getNodeSizes(start.leaf());

            from_start = false;
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
        removeAllPages(start, stop, keys, removed_key_count);

        start_idx       = stop_idx.setAll(0);
    }
    else if (from_start && !at_end)
    {
        removePagesFromStart(stop, stop_idx, keys, removed_key_count);

        if (merge)
        {
        	self.mergeWithRightSibling(stop, 0);
        }

        start       = stop;
        start_idx   = stop_idx;
    }
    else if ((!from_start) && at_end)
    {
        removePagesAtEnd(start, start_idx, keys, removed_key_count);

        if (merge)
        {
        	self.mergeWithLeftSibling(start, 0, [&start_idx, &self](const TreePath& left, const TreePath& right, Int level)
        	{
        		if (level == 0) {
        			start_idx += self.getNodeSizes(left.leaf());
        		}
            });
        }

        stop        = start;
        stop_idx    = start_idx;
    }
    else {
        removePages(start, start_idx, stop, stop_idx, 0, keys, removed_key_count);

        if (merge)
        {
        	self.mergeWithSiblings(stop, 0, [&stop_idx, &self](const TreePath& left, const TreePath& right, Int level)
            {
        		if (level == 0)
        		{
        			stop_idx += self.getNodeSizes(left.leaf());
        		}
            });
        }

        start       = stop;
        start_idx   = stop_idx;
    }

    self.addTotalKeyCount(stop, -removed_key_count);

    return removed_key_count;
}




M_PARAMS
void M_TYPE::removeAllPages(TreePath& start, TreePath& stop, Accumulator& accum, Position& removed_key_count)
{
	auto& self = this->self();

    Int level = start.getSize() - 1;
    Position count = self.getNodeSizes(start[level].node());

    removed_key_count += self.removeRoom(start, level, Position(0), count, accum);


    self.removeNode(start[level].node());

    NodeBaseG node = self.createRootNode1(0, true, me()->getRootMetadata());

    self.set_root(node->id());

    start.clear();
    stop.clear();

    start.append(TreePathItem(node, 0));
    stop. append(TreePathItem(node, 0));
}


M_PARAMS
void M_TYPE::removePagesFromStart(TreePath& stop, Position& stop_idx, Accumulator& accum, Position& removed_key_count)
{
    auto& self = this->self();

	Position idx = stop_idx;

    for (Int c = 0; c < stop.getSize(); c++)
    {
        removed_key_count += self.removeRoom(stop, c, Position(0), idx, accum);
        idx = Position(stop[c].parent_idx());
    }

    stop_idx = Position(0);

    self.removeRedundantRoot(stop, 0);
}


M_PARAMS
void M_TYPE::removePagesAtEnd(TreePath& start, Position& start_idx, Accumulator& accum, Position& removed_key_count)
{
	auto& self = this->self();

	if (start_idx.eqAll(0))
    {
    	self.getPrevNode(start);
        start_idx = self.getNodeSizes(start.leaf());
    }

    Position idx = start_idx;

    for (Int c = 0; c < start.getSize(); c++)
    {
        if (idx.gtAny(0))
        {
            removed_key_count += self.removeRoom(start, c, idx, self.getNodeSizes(start[c].node()) - idx, accum);
            idx = Position(start[c].parent_idx() + 1);
        }
        else {
            idx = Position(start[c].parent_idx());
        }
    }

    self.removeRedundantRoot(start, 0);
}


M_PARAMS
void M_TYPE::removePages(
                TreePath& start,
                Position& start_idx,
                TreePath& stop,
                Position& stop_idx,
                Int level,
                Accumulator& accum,
                Position& removed_key_count
)
{
    if (start_idx.eqAll(0))
    {
    	if (self().getPrevNode(start))
    	{
    		start_idx = self().getNodeSizes(start.leaf());
    	}
    }

    removePagesInternal(start, start_idx, stop, stop_idx, level, accum, removed_key_count);
}


M_PARAMS
void M_TYPE::removePagesInternal(
                TreePath& start,
                Position& start_idx,
                TreePath& stop,
                Position& stop_idx,
                Int level,
                Accumulator& accum,
                Position& removed_key_count)
{
	auto& self = this->self();

    if (self.isTheSameNode(start, stop, level))
    {
        // The root node of removed subtree

        if ((stop_idx - start_idx).gtAny(0))
        {
            //remove some space within the node
            Position count = stop_idx - start_idx;
            removed_key_count += self.removeRoom(start, level, start_idx, count, accum);

            if (level > 0)
            {
            	stop.moveLeft(level - 1, 0, count.get());
            }

            if (!start[level]->is_root())
            {
                self.removeRedundantRoot(start, stop, level);
            }

            stop_idx = start_idx;
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        removed_key_count 		+= self.removeRoom(
        									start,
        									level,
        									start_idx,
        									self.getNodeSizes(start[level].node()) - start_idx,
        									accum
        								);

        removed_key_count       += self.removeRoom(stop,  level, Position(0), stop_idx, accum);

        stop_idx.setAll(0);

        Position start_parent_idx = Position(start[level].parent_idx() + 1);

        // FIXME: stop[level].parent_idx() - can be updated elsewhere in makeRoom() - check it

        Position stop_parent_idx = Position(stop[level].parent_idx());

        removePages(start, start_parent_idx, stop, stop_parent_idx, level + 1, accum, removed_key_count);

        stop[level].parent_idx() = stop_parent_idx.get();


        if (self.isTheSameParent(start, stop, level))
        {
            if (self.canMerge(start, stop, level))
            {
                self.mergeNodes(start, stop, level);

                stop_idx = start_idx;

                self.removeRedundantRoot(start, stop, level);
            }
        }
    }
}







#undef M_TYPE
#undef M_PARAMS


}

#endif
