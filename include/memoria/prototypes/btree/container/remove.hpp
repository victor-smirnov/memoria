
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_REMOVE_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_REMOVE_HPP


#include <memoria/prototypes/btree/pages/tools.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::RemoveName)


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::ID                                                   ID;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::Accumulator											Accumulator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int  Indexes                                                   = Base::Indexes;

    

    struct DataRemoveHandlerFn {

    	Int idx_, count_;
    	MyType& me_;

    	DataRemoveHandlerFn(Int idx, Int count, MyType& me): idx_(idx), count_(count), me_(me) {}

    	template <typename Node>
    	void operator()(Node* node) {}
    };

    struct UpdateType {
    	enum Enum {NONE, PARENT_ONLY, FULL};
    };






    bool mergeWithSiblings(TreePath& path, Int level)
    {
    	Int idx = 0;
    	return me()->mergeWithSiblings(path, level, idx);
    }

    bool mergeWithLeftSibling(TreePath& path, Int level, Int& key_idx);
    bool mergeWithRightSibling(TreePath& path, Int level);
    bool mergeWithSiblings(TreePath& path, Int level, Int& key_idx);

    bool mergePaths(TreePath& tgt, TreePath& src, Int level = 0);

    /**
     * remove key and data pointed by iterator 'iter' form the map.
     *
     */
    bool removeEntry(Iterator& iter, Accumulator& keys);

    void removeEntry(TreePath& path, Int& idx, Accumulator& keys, bool merge = true);

    BigInt removeEntries(Iterator& from, Iterator& to, Accumulator& accum, bool merge = true);

    void drop();

// PROTECTED API:

    void removeAllPages(TreePath& start, TreePath& stop, Accumulator& accum, BigInt& removed_key_count);
    void removePagesFromStart(TreePath& stop, Int& stop_idx, Accumulator& accum, BigInt& removed_key_count);
    void removePagesAtEnd(TreePath& start, Int& start_idx, Accumulator& accum, BigInt& removed_key_count);
    void removePages(TreePath& start, Int& start_idx, TreePath& stop, Int& stop_idx, Int level, Accumulator& accum, BigInt& removed_key_count);

    /**
         * remove 'count' elements from tree node starting from 'from' element.
         *
         * For all _counters_ (page_count, key_count, ...)
         * Sum child._counter_ of all removed children and decrement current
         * node._counter_
         *
         * remove children if 'remove_children' is TRUE
         *
         * Collapse elemnt's window in the node by shifting rest of elements
         * (keys & data) to the 'from' position.
         *
         * For all shifted children update child.parent_id
         */

        // FIXME: remove data pages for dynarray
    BigInt removeRoom(TreePath& path, Int level, Int from, Int count, Accumulator& accumulator, bool remove_children = true);


private:
    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    void removePagesInternal(TreePath& start, Int& start_idx, TreePath& stop, Int& stop_idx, Int level, Accumulator& accum, BigInt& removed_key_count);

    void removeRedundantRoot(TreePath& path, Int level);
    void removeRedundantRoot(TreePath& start, TreePath& stop, Int level);




    /**
     * remove a page from the btree. Do recursive removing if page's parent
     * has no more children.
     *
     * If after removing the parent is less than half filled than
     * merge it with siblings.
     */

    void removePage(TreePath& path, Int& idx);


    /**
     * Delete a node with it's children.
     */
    BigInt removeNode(NodeBaseG node);

    bool changeRootIfSingular(NodeBaseG& parent, NodeBaseG& node);

    bool canMerge(TreePath& tgt, TreePath& src, Int level)
    {
    	return src[level].node()->children_count() <= me()->getCapacity(tgt[level].node());
    }


    static bool IsTheSameParent(TreePath& left, TreePath& right, Int level)
    {
    	return left[level + 1].node() == right[level + 1].node();
    }

    void mergeNodes(TreePath& tgt, TreePath& src, Int level);
    bool mergeBTreeNodes(TreePath& tgt, TreePath& src, Int level);



    class RemoveElementsFn {

        Int 			from_;
        Int 			count_;
        bool 			reindex_;
        Accumulator& 	accum_;

    public:

        RemoveElementsFn(Int from, Int count, Accumulator& accum, bool reindex = true):
                from_(from), count_(count), reindex_(reindex),
                accum_(accum)
        {}

        template <typename Node>
        void operator()(Node *node)
        {
        	for (Int d = 0; d < Indexes; d++)
        	{
        		for (Int c = from_; c < from_ + count_; c++)
        		{
        			accum_.keys()[d] += node->map().key(d, c);
        		}
        	}


            if (from_ + count_ <= node->children_count())
            {
                node->map().removeSpace(from_, count_);
            }

            node->map().clear(node->children_count() - count_, node->children_count());

            node->set_children_count(node->map().size());

            if (reindex_)
            {
                node->map().reindex();
            }
        }
    };



    /**
     * Merge two nodes moving keys and values from 'page2' to 'page1' assuming
     * these two pages have the same parent.
     *
     */

    class MergeNodesFn {
    	Int start_;
    public:
    	template <typename T1, typename T2>
    	void operator()(T1 *page1, T2 *page2)
    	{
    		start_ = page1->children_count();

    		page2->map().copyTo(&page1->map(), 0, page2->children_count(), page1->children_count());
    		page1->inc_size(page2->children_count());
    		page1->map().reindex();
    	}

    	Int start() const {
    		return start_;
    	}
    };




MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::RemoveName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS




M_PARAMS
BigInt M_TYPE::removeRoom(TreePath& path, Int level, Int from, Int count, Accumulator& accumulator, bool remove_children)
{
	//FIXME: optimize for the case when count == 0

	BigInt key_count = 0;

	if (count > 0)
	{
		NodeBaseG& node = path[level].node();
		node.update();

		Accumulator tmp_accum;

		if (remove_children)
		{
			if (!node->is_leaf())
			{
				for (Int c = from; c < from + count; c++)
				{
					NodeBaseG child = 	me()->getChild(node, c, Allocator::READ);
					key_count 		+=	me()->removeNode(child);
				}
			}
			else {
				key_count = count;

				typename MyType::DataRemoveHandlerFn data_remove_handler_fn(from, count, *me());
				LeafDispatcher::Dispatch(node, data_remove_handler_fn);
			}
		}

		RemoveElementsFn fn(from, count, tmp_accum);
		NodeDispatcher::Dispatch(node, fn);

		me()->updateParentIfExists(path, level, -tmp_accum);

		path.moveLeft(level - 1, from, count);

		accumulator += tmp_accum;
	}

	return key_count;
}


M_PARAMS
BigInt M_TYPE::removeEntries(Iterator& from, Iterator& to, Accumulator& keys, bool merge)
{
	if (from.isEmpty() || from.isEnd())
	{
		return 0;
	}

	if (to.isEmpty())
	{
		return 0;
	}

	BigInt removed_key_count = 0;

	TreePath& start 	= from.path();
	Int& 	  start_idx	= from.key_idx();

	TreePath& stop 		= to.path();
	Int& 	  stop_idx	= to.key_idx();

	bool at_end = stop_idx >= stop[0].node()->children_count();

	bool from_start;

	if (start_idx == 0)
	{
		if (me()->getPrevNode(start, 0))
		{
			start_idx = start[0].node()->children_count();

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

		start_idx 		= stop_idx 		= 0;
		from.keyNum()   = to.keyNum()   = 0;
	}
	else if (from_start && !at_end)
	{
		removePagesFromStart(stop, stop_idx, keys, removed_key_count);

		if (merge) {
			me()->mergeWithRightSibling(stop, 0);
		}

		to.keyNum() -= removed_key_count;

		start 		= stop;
		start_idx 	= stop_idx;
	}
	else if ((!from_start) && at_end)
	{
		removePagesAtEnd(start, start_idx, keys, removed_key_count);

		if (merge) me()->mergeWithLeftSibling(start, 0, start_idx);

		stop 		= start;
		stop_idx 	= start_idx;
	}
	else {
		removePages(start, start_idx, stop, stop_idx, 0, keys, removed_key_count);

		if (merge) me()->mergeWithSiblings(stop, 0, stop_idx);

		to.keyNum() -= removed_key_count;

		start 		= stop;
		start_idx 	= stop_idx;
	}

	me()->addTotalKeyCount(stop, -removed_key_count);

	return removed_key_count;
}




M_PARAMS
void M_TYPE::removeAllPages(TreePath& start, TreePath& stop, Accumulator& accum, BigInt& removed_key_count)
{
	Int level = start.getSize() - 1;
	Int count = start[level].node()->children_count();

	removed_key_count += removeRoom(start, level, 0, count, accum);


	me()->removeNode(start[level].node());

	NodeBaseG node = me()->createRootNode(0, true, me()->getRootMetadata());

	me()->set_root(node->id());

	start.clear();
	stop.clear();

	start.append(TreePathItem(node, 0));
	stop. append(TreePathItem(node, 0));
}


M_PARAMS
void M_TYPE::removePagesFromStart(TreePath& stop, Int& stop_idx, Accumulator& accum, BigInt& removed_key_count)
{
	Int idx = stop_idx;
	for (Int c = 0; c < stop.getSize(); c++)
	{
		removed_key_count += removeRoom(stop, c, 0, idx, accum);
		idx = stop[c].parent_idx();
	}

	stop_idx = 0;

	removeRedundantRoot(stop, 0);
}


M_PARAMS
void M_TYPE::removePagesAtEnd(TreePath& start, Int& start_idx, Accumulator& accum, BigInt& removed_key_count)
{
	if (start_idx == 0)
	{
		me()->getPrevNode(start);
		start_idx = start[0]->children_count();
	}

	Int idx = start_idx;

	for (Int c = 0; c < start.getSize(); c++)
	{
		if (idx > 0)
		{
			removed_key_count += removeRoom(start, c, idx, start[c]->children_count() - idx, accum);
			idx = start[c].parent_idx() + 1;
		}
		else {
			idx = start[c].parent_idx();
		}
	}

	removeRedundantRoot(start, 0);

	me()->finishPathStep(start, start_idx);
}


M_PARAMS
void M_TYPE::removePages(TreePath& start, Int& start_idx, TreePath& stop, Int& stop_idx, Int level, Accumulator& accum, BigInt& removed_key_count)
{
	if (start_idx == 0)
	{
		me()->getPrevNode(start);
		start_idx = start[0]->children_count();
	}

	removePagesInternal(start, start_idx, stop, stop_idx, level, accum, removed_key_count);
}


M_PARAMS
void M_TYPE::removePagesInternal(TreePath& start, Int& start_idx, TreePath& stop, Int& stop_idx, Int level, Accumulator& accum, BigInt& removed_key_count)
{
	if (me()->isTheSameNode(start, stop, level))
	{
		// The root node of removed subtree

		if (stop_idx - start_idx >= 0)
		{
			//remove some space within the node
			Int count = stop_idx - start_idx;
			removed_key_count += removeRoom(start, level, start_idx, count, accum);

			stop.moveLeft(level - 1, 0, count);

			if (!start[level]->is_root())
			{
				removeRedundantRoot(start, stop, level);
			}

			stop_idx = start_idx;
		}
	}
	else
	{
		// The region to remove crosses node boundaries.
		// We need to up the tree until we found the node
		// enclosing the region. See the code branch above.

		removed_key_count 		+= removeRoom(start, level, start_idx, start[level].node()->children_count() - start_idx, accum);

		removed_key_count 		+= removeRoom(stop,  level, 0, stop_idx, accum);

		stop_idx = 0;

		Int start_parent_idx = start[level].parent_idx() + 1;

		// FIXME: stop[level].parent_idx() - can be updated elsewhere in makeRoom() - check it
		removePages(start, start_parent_idx, stop, stop[level].parent_idx(), level + 1, accum, removed_key_count);

		if (IsTheSameParent(start, stop, level))
		{
			if (canMerge(start, stop, level))
			{
				mergeNodes(start, stop, level);

				stop_idx = start_idx;

				removeRedundantRoot(start, stop, level);
			}
		}
	}
}





/**
 * remove a page from BTree. Do recursive removal if the page's parent
 * has no more children.
 *
 * If after removing the parent is less than half filled than
 * merge it with siblings.
 *
 */

M_PARAMS
void M_TYPE::removePage(TreePath& path, Int& key_idx)
{
	for (Int c = 1; c < path.getSize(); c++)
	{
		if (path[c]->children_count() > 1)
		{
			Accumulator accum;

			Int idx = path[c - 1].parent_idx();
			me()->removeRoom(path, c, idx, 1, accum);

			if (idx == path[c]->children_count())
			{
				if (me()->getNextNode(path, c, true))
				{
					key_idx = 0;
				}
				else
				{
					for (Int d = c - 1; d >= 0; d--)
					{
						path[d].node() 			= me()->getLastChild(path[d + 1].node(), Allocator::READ);
						path[d].parent_idx() 	= path[d + 1]->children_count() - 1;
					}

					key_idx = path.leaf()->children_count();

					me()->finishPathStep(path, key_idx);
				}
			}
			else
			{
				for (Int d = c - 1; d >= 0; d--)
				{
					path[d].node() 			= me()->getChild(path[d + 1].node(), idx, Allocator::READ);
					path[d].parent_idx() 	= idx;

					idx = 0;
				}

				me()->finishPathStep(path, 0);
				key_idx = 0;
			}

			return;
		}
		else if (path[c]->is_root())
		{
			me()->removeNode(path[c].node());

			NodeBaseG node = me()->createRootNode(0, true, me()->getRootMetadata());

			me()->set_root(node->id());
			path.clear();
			path.append(TreePathItem(node, 0));

			key_idx = 0;

			return;
		}
	}

	me()->removeNode(path.leaf().node());

	NodeBaseG node = me()->createRootNode(0, true, me()->getRootMetadata());

	me()->set_root(node->id());
	path.clear();
	path.append(TreePathItem(node, 0));

	key_idx = 0;
}


M_PARAMS
BigInt M_TYPE::removeNode(NodeBaseG node)
{
	const Int children_count = node->children_count();

	BigInt count = 0;

	if (!node->is_leaf())
	{
		for (Int c = 0; c < children_count; c++)
		{
			NodeBaseG child = me()->getChild(node, c, Allocator::READ);
			count += me()->removeNode(child);
		}
	}
	else {
		typename MyType::DataRemoveHandlerFn data_remove_handler_fn(0, children_count, *me());
		LeafDispatcher::Dispatch(node, data_remove_handler_fn);

		count += children_count;
	}

	if (node->is_root())
	{
		ID id;
		id.setNull();
		me()->set_root(id);
	}

	me()->allocator().removePage(node->id());

	return count;
}


M_PARAMS
bool M_TYPE::changeRootIfSingular(NodeBaseG& parent, NodeBaseG& node)
{
	if (parent.isSet() && parent->is_root() && parent->children_count() == 1)
	{
		Metadata meta = me()->getRootMetadata();

		me()->node2Root(node, meta);

		me()->set_root(node->id());

		me()->allocator().removePage(parent->id());

		return true;
	}
	else {
		return false;
	}
}



/**
 * remove key and data pointed by iterator 'iter' form the map.
 *
 */

M_PARAMS
bool M_TYPE::removeEntry(Iterator& iter, Accumulator& keys)
{
	if (iter.isNotEmpty() || iter.isNotEnd())
	{
		removeEntry(iter.path(), iter.key_idx(), keys);

		if (iter.isEnd())
		{
			iter.nextLeaf();
		}

		return true;
	}
	else {
		return false;
	}
}


M_PARAMS
void M_TYPE::removeEntry(TreePath& path, Int& idx, Accumulator& keys, bool merge)
{
	Int children_count 	= path.leaf()->children_count();

	//if leaf page has more than 1 key do regular remove

	if (children_count > 1)
	{
		//remove 1 element rom the leaf, update parent and
		//do not try to remove children (it's a leaf)

		removeRoom(path, 0, idx, 1, keys);

		//try merging this leaf with previous of following
		//leaf if filled by half of it's capacity.
		if (merge && me()->shouldMergeNode(path, 0))
		{
			me()->mergeWithSiblings(path, 0, idx);
		}

		me()->finishPathStep(path, idx);
	}
	else {
		keys = me()->getKeys(path.leaf().node(), idx);
		removePage(path, idx);
	}

	me()->addTotalKeyCount(-1);
}




M_PARAMS
void M_TYPE::removeRedundantRoot(TreePath& path, Int level)
{
	for (Int c = path.getSize() - 1; c > level; c--)
	{
		NodeBaseG& node = path[c].node();

		if (node->children_count() == 1)
		{
			Metadata root_metadata = me()->getRootMetadata();

			NodeBaseG& child = path[c - 1].node();

			me()->node2Root(child, root_metadata);

			me()->allocator().removePage(node->id());

			me()->set_root(child->id());

			path.removeLast();
		}
		else {
			break;
		}
	}
}


M_PARAMS
void M_TYPE::removeRedundantRoot(TreePath& start, TreePath& stop, Int level)
{
	for (Int c = start.getSize() - 1; c > level; c--)
	{
		NodeBaseG& node = start[c].node();

		if (node->children_count() == 1)
		{
			Metadata root_metadata = me()->getRootMetadata();

			NodeBaseG& child = start[c - 1].node();

			if (me()->canConvertToRoot(child))
			{
				me()->node2Root(child, root_metadata);

				me()->allocator().removePage(node->id());

				me()->set_root(child->id());

				start.removeLast();

				stop.removeLast();
			}
			else {
				break;
			}
		}
		else {
			break;
		}
	}
}




M_PARAMS
bool M_TYPE::mergeWithSiblings(TreePath& path, Int level, Int& key_idx)
{
	if (me()->mergeWithRightSibling(path, level))
	{
		return true;
	}
	else
	{
		return me()->mergeWithLeftSibling(path, level, key_idx);
	}
}


M_PARAMS
bool M_TYPE::mergeWithLeftSibling(TreePath& path, Int level, Int& key_idx)
{
	bool merged = false;

	if (me()->shouldMergeNode(path, level))
	{
		TreePath prev = path;

		if (me()->getPrevNode(prev, level))
		{
			Int size = prev[level]->children_count();

			merged = mergeBTreeNodes(prev, path, level);

			if (merged)
			{
				key_idx += size;
				path = prev;
			}
		}
		else {
			merged = false;
		}

	}

	return merged;
}


M_PARAMS
bool M_TYPE::mergeWithRightSibling(TreePath& path, Int level)
{
	bool merged = false;

	if (me()->shouldMergeNode(path, level))
	{
		TreePath next = path;

		if (me()->getNextNode(next))
		{
			merged = mergeBTreeNodes(path, next, level);
		}
	}

	return merged;
}



M_PARAMS
bool M_TYPE::mergePaths(TreePath& tgt, TreePath& src, Int level)
{
	bool merged_at_level = false;

	for (Int c = tgt.getSize() - 1; c >= level; c--)
	{
		if (canMerge(tgt, src, c))
		{
			if (tgt[c] != src[c])
			{
				mergeNodes(tgt, src, c);

				src[c] = tgt[c];

				merged_at_level = c == level;
			}
		}
		else {
			break;
		}
	}

	removeRedundantRoot(tgt, src, level);

	return merged_at_level;
}


M_PARAMS
void M_TYPE::mergeNodes(TreePath& tgt, TreePath& src, Int level)
{
	NodeBaseG& page1 = tgt[level].node();
	NodeBaseG& page2 = src[level].node();

	page1.update();

	Int tgt_children_count = page1->children_count();

	MergeNodesFn fn;
	NodeDispatcher::Dispatch(page1, page2, fn);


	NodeBaseG& parent 	= src[level + 1].node();
	Int parent_idx 		= src[level].parent_idx();

	Accumulator accum;

	removeRoom(src, level + 1, parent_idx, 1, accum, false);

	me()->updateUp(src, level + 1, parent_idx - 1, accum);

	me()->allocator().removePage(page2->id());

	src[level] = tgt[level];

	src.moveRight(level - 1, 0, tgt_children_count);

	me()->reindex(parent); //FIXME: does it necessary?
}

M_PARAMS
bool M_TYPE::mergeBTreeNodes(TreePath& tgt, TreePath& src, Int level)
{
	if (canMerge(tgt, src, level))
	{
		if (IsTheSameParent(tgt, src, level))
		{
			mergeNodes(tgt, src, level);

			removeRedundantRoot(tgt, src, level);

			return true;
		}
		else
		{
			if (me()->mergeBTreeNodes(tgt, src, level + 1))
			{
				mergeNodes(tgt, src, level);

				removeRedundantRoot(tgt, src, level);

				return true;
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}
}



M_PARAMS
void M_TYPE::drop()
{
	NodeBaseG root = me()->getRoot(Allocator::READ);

	if (root.isSet())
	{
		me()->removeNode(root);
	}
}


#undef M_TYPE
#undef M_PARAMS


}

#endif
