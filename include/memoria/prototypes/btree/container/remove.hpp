
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






    bool MergeWithSiblings(TreePath& path, Int level)
    {
    	Int idx = 0;
    	return me()->MergeWithSiblings(path, level, idx);
    }

    bool MergeWithLeftSibling(TreePath& path, Int level, Int& key_idx);
    bool MergeWithRightSibling(TreePath& path, Int level);
    bool MergeWithSiblings(TreePath& path, Int level, Int& key_idx);

    bool MergePaths(TreePath& tgt, TreePath& src, Int level = 0);

    /**
     * Remove key and data pointed by iterator 'iter' form the map.
     *
     */
    bool RemoveEntry(Iterator& iter, Accumulator& keys);

    void RemoveEntry(TreePath& path, Int& idx, Accumulator& keys, bool merge = true);

    BigInt RemoveEntries(Iterator& from, Iterator& to, Accumulator& accum, bool merge = true);

    void Drop();

// PROTECTED API:

    void RemoveAllPages(TreePath& start, TreePath& stop, Accumulator& accum, BigInt& removed_key_count);
    void RemovePagesFromStart(TreePath& stop, Int& stop_idx, Accumulator& accum, BigInt& removed_key_count);
    void RemovePagesAtEnd(TreePath& start, Int& start_idx, Accumulator& accum, BigInt& removed_key_count);
    void RemovePages(TreePath& start, Int& start_idx, TreePath& stop, Int& stop_idx, Int level, Accumulator& accum, BigInt& removed_key_count);

    /**
         * Remove 'count' elements from tree node starting from 'from' element.
         *
         * For all _counters_ (page_count, key_count, ...)
         * Sum child._counter_ of all removed children and decrement current
         * node._counter_
         *
         * Remove children if 'remove_children' is TRUE
         *
         * Collapse elemnt's window in the node by shifting rest of elements
         * (keys & data) to the 'from' position.
         *
         * For all shifted children update child.parent_id
         */

        // FIXME: remove data pages for dynarray
    BigInt RemoveRoom(TreePath& path, Int level, Int from, Int count, Accumulator& accumulator, bool remove_children = true);


private:
    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    void RemovePagesInternal(TreePath& start, Int& start_idx, TreePath& stop, Int& stop_idx, Int level, Accumulator& accum, BigInt& removed_key_count);

    void RemoveRedundantRoot(TreePath& path, Int level);
    void RemoveRedundantRoot(TreePath& start, TreePath& stop, Int level);




    /**
     * Remove a page from the btree. Do recursive removing if page's parent
     * has no more children.
     *
     * If after removing the parent is less than half filled than
     * merge it with siblings.
     */

    void RemovePage(TreePath& path, Int& idx);


    /**
     * Delete a node with it's children.
     */
    BigInt RemoveNode(NodeBaseG node);

    bool ChangeRootIfSingular(NodeBaseG& parent, NodeBaseG& node);

    bool CanMerge(TreePath& tgt, TreePath& src, Int level)
    {
    	return src[level].node()->children_count() <= me()->GetCapacity(tgt[level].node());
    }


    static bool IsTheSameParent(TreePath& left, TreePath& right, Int level)
    {
    	return left[level + 1].node() == right[level + 1].node();
    }

    void MergeNodes(TreePath& tgt, TreePath& src, Int level);
    bool MergeBTreeNodes(TreePath& tgt, TreePath& src, Int level);



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
                node->map().RemoveSpace(from_, count_);
            }

            node->map().Clear(node->children_count() - count_, node->children_count());

            node->set_children_count(node->map().size());

            if (reindex_)
            {
                node->map().Reindex();
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

    		page2->map().CopyTo(&page1->map(), 0, page2->children_count(), page1->children_count());
    		page1->inc_size(page2->children_count());
    		page1->map().Reindex();
    	}

    	Int start() const {
    		return start_;
    	}
    };




MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::RemoveName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS




M_PARAMS
BigInt M_TYPE::RemoveRoom(TreePath& path, Int level, Int from, Int count, Accumulator& accumulator, bool remove_children)
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
					NodeBaseG child = 	me()->GetChild(node, c, Allocator::READ);
					key_count 		+=	me()->RemoveNode(child);
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

		me()->UpdateParentIfExists(path, level, -tmp_accum);

		path.MoveLeft(level - 1, from, count);

		accumulator += tmp_accum;
	}

	return key_count;
}


M_PARAMS
BigInt M_TYPE::RemoveEntries(Iterator& from, Iterator& to, Accumulator& keys, bool merge)
{
	if (from.IsEmpty() || from.IsEnd())
	{
		return 0;
	}

	if (to.IsEmpty())
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
		if (me()->GetPrevNode(start, 0))
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
		RemoveAllPages(start, stop, keys, removed_key_count);

		start_idx 		= stop_idx 		= 0;
		from.KeyNum()   = to.KeyNum()   = 0;
	}
	else if (from_start && !at_end)
	{
		RemovePagesFromStart(stop, stop_idx, keys, removed_key_count);

		if (merge) {
			me()->MergeWithRightSibling(stop, 0);
		}

		to.KeyNum() -= removed_key_count;

		start 		= stop;
		start_idx 	= stop_idx;
	}
	else if ((!from_start) && at_end)
	{
		RemovePagesAtEnd(start, start_idx, keys, removed_key_count);

		if (merge) me()->MergeWithLeftSibling(start, 0, start_idx);

		stop 		= start;
		stop_idx 	= start_idx;
	}
	else {
		RemovePages(start, start_idx, stop, stop_idx, 0, keys, removed_key_count);

		if (merge) me()->MergeWithSiblings(stop, 0, stop_idx);

		to.KeyNum() -= removed_key_count;

		start 		= stop;
		start_idx 	= stop_idx;
	}

	me()->AddTotalKeyCount(stop, -removed_key_count);

	return removed_key_count;
}




M_PARAMS
void M_TYPE::RemoveAllPages(TreePath& start, TreePath& stop, Accumulator& accum, BigInt& removed_key_count)
{
	Int level = start.GetSize() - 1;
	Int count = start[level].node()->children_count();

	removed_key_count += RemoveRoom(start, level, 0, count, accum);


	me()->RemoveNode(start[level].node());

	NodeBaseG node = me()->CreateRootNode(0, true, me()->GetRootMetadata());

	me()->set_root(node->id());

	start.Clear();
	stop.Clear();

	start.Append(TreePathItem(node, 0));
	stop. Append(TreePathItem(node, 0));
}


M_PARAMS
void M_TYPE::RemovePagesFromStart(TreePath& stop, Int& stop_idx, Accumulator& accum, BigInt& removed_key_count)
{
	Int idx = stop_idx;
	for (Int c = 0; c < stop.GetSize(); c++)
	{
		removed_key_count += RemoveRoom(stop, c, 0, idx, accum);
		idx = stop[c].parent_idx();
	}

	stop_idx = 0;

	RemoveRedundantRoot(stop, 0);
}


M_PARAMS
void M_TYPE::RemovePagesAtEnd(TreePath& start, Int& start_idx, Accumulator& accum, BigInt& removed_key_count)
{
	if (start_idx == 0)
	{
		me()->GetPrevNode(start);
		start_idx = start[0]->children_count();
	}

	Int idx = start_idx;

	for (Int c = 0; c < start.GetSize(); c++)
	{
		if (idx > 0)
		{
			removed_key_count += RemoveRoom(start, c, idx, start[c]->children_count() - idx, accum);
			idx = start[c].parent_idx() + 1;
		}
		else {
			idx = start[c].parent_idx();
		}
	}

	RemoveRedundantRoot(start, 0);

	me()->FinishPathStep(start, start_idx);
}


M_PARAMS
void M_TYPE::RemovePages(TreePath& start, Int& start_idx, TreePath& stop, Int& stop_idx, Int level, Accumulator& accum, BigInt& removed_key_count)
{
	if (start_idx == 0)
	{
		me()->GetPrevNode(start);
		start_idx = start[0]->children_count();
	}

	RemovePagesInternal(start, start_idx, stop, stop_idx, level, accum, removed_key_count);
}


M_PARAMS
void M_TYPE::RemovePagesInternal(TreePath& start, Int& start_idx, TreePath& stop, Int& stop_idx, Int level, Accumulator& accum, BigInt& removed_key_count)
{
	if (me()->IsTheSameNode(start, stop, level))
	{
		// The root node of removed subtree

		if (stop_idx - start_idx >= 0)
		{
			//Remove some space within the node
			Int count = stop_idx - start_idx;
			removed_key_count += RemoveRoom(start, level, start_idx, count, accum);

			stop.MoveLeft(level - 1, 0, count);

			if (!start[level]->is_root())
			{
				RemoveRedundantRoot(start, stop, level);
			}

			stop_idx = start_idx;
		}
	}
	else
	{
		// The region to remove crosses node boundaries.
		// We need to up the tree until we found the node
		// enclosing the region. See the code branch above.

		removed_key_count 		+= RemoveRoom(start, level, start_idx, start[level].node()->children_count() - start_idx, accum);

		removed_key_count 		+= RemoveRoom(stop,  level, 0, stop_idx, accum);

		stop_idx = 0;

		Int start_parent_idx = start[level].parent_idx() + 1;

		// FIXME: stop[level].parent_idx() - can be updated elsewhere in MakeRoom() - check it
		RemovePages(start, start_parent_idx, stop, stop[level].parent_idx(), level + 1, accum, removed_key_count);

		if (IsTheSameParent(start, stop, level))
		{
			if (CanMerge(start, stop, level))
			{
				MergeNodes(start, stop, level);

				stop_idx = start_idx;

				RemoveRedundantRoot(start, stop, level);
			}
		}
	}
}





/**
 * Remove a page from BTree. Do recursive removal if the page's parent
 * has no more children.
 *
 * If after removing the parent is less than half filled than
 * merge it with siblings.
 *
 */

M_PARAMS
void M_TYPE::RemovePage(TreePath& path, Int& key_idx)
{
	for (Int c = 1; c < path.GetSize(); c++)
	{
		if (path[c]->children_count() > 1)
		{
			Accumulator accum;

			Int idx = path[c - 1].parent_idx();
			me()->RemoveRoom(path, c, idx, 1, accum);

			if (idx == path[c]->children_count())
			{
				if (me()->GetNextNode(path, c, true))
				{
					key_idx = 0;
				}
				else
				{
					for (Int d = c - 1; d >= 0; d--)
					{
						path[d].node() 			= me()->GetLastChild(path[d + 1].node(), Allocator::READ);
						path[d].parent_idx() 	= path[d + 1]->children_count() - 1;
					}

					key_idx = path.leaf()->children_count();

					me()->FinishPathStep(path, key_idx);
				}
			}
			else
			{
				for (Int d = c - 1; d >= 0; d--)
				{
					path[d].node() 			= me()->GetChild(path[d + 1].node(), idx, Allocator::READ);
					path[d].parent_idx() 	= idx;

					idx = 0;
				}

				me()->FinishPathStep(path, 0);
				key_idx = 0;
			}

			return;
		}
		else if (path[c]->is_root())
		{
			me()->RemoveNode(path[c].node());

			NodeBaseG node = me()->CreateRootNode(0, true, me()->GetRootMetadata());

			me()->set_root(node->id());
			path.Clear();
			path.Append(TreePathItem(node, 0));

			key_idx = 0;

			return;
		}
	}

	me()->RemoveNode(path.leaf().node());

	NodeBaseG node = me()->CreateRootNode(0, true, me()->GetRootMetadata());

	me()->set_root(node->id());
	path.Clear();
	path.Append(TreePathItem(node, 0));

	key_idx = 0;
}


M_PARAMS
BigInt M_TYPE::RemoveNode(NodeBaseG node)
{
	const Int children_count = node->children_count();

	BigInt count = 0;

	if (!node->is_leaf())
	{
		for (Int c = 0; c < children_count; c++)
		{
			NodeBaseG child = me()->GetChild(node, c, Allocator::READ);
			count += me()->RemoveNode(child);
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
		id.set_null();
		me()->set_root(id);
	}

	me()->allocator().RemovePage(node->id());

	return count;
}


M_PARAMS
bool M_TYPE::ChangeRootIfSingular(NodeBaseG& parent, NodeBaseG& node)
{
	if (parent.is_set() && parent->is_root() && parent->children_count() == 1)
	{
		Metadata meta = me()->GetRootMetadata();

		me()->Node2Root(node, meta);

		me()->set_root(node->id());

		me()->allocator().RemovePage(parent->id());

		return true;
	}
	else {
		return false;
	}
}



/**
 * Remove key and data pointed by iterator 'iter' form the map.
 *
 */

M_PARAMS
bool M_TYPE::RemoveEntry(Iterator& iter, Accumulator& keys)
{
	if (iter.IsNotEmpty() || iter.IsNotEnd())
	{
		RemoveEntry(iter.path(), iter.key_idx(), keys);

		if (iter.IsEnd())
		{
			iter.NextLeaf();
		}

		return true;
	}
	else {
		return false;
	}
}


M_PARAMS
void M_TYPE::RemoveEntry(TreePath& path, Int& idx, Accumulator& keys, bool merge)
{
	Int children_count 	= path.leaf()->children_count();

	//if leaf page has more than 1 key do regular remove

	if (children_count > 1)
	{
		//remove 1 element rom the leaf, update parent and
		//do not try to remove children (it's a leaf)

		RemoveRoom(path, 0, idx, 1, keys);

		//try merging this leaf with previous of following
		//leaf if filled by half of it's capacity.
		if (merge && me()->ShouldMergeNode(path, 0))
		{
			me()->MergeWithSiblings(path, 0, idx);
		}

		me()->FinishPathStep(path, idx);
	}
	else {
		keys = me()->GetKeys(path.leaf().node(), idx);
		RemovePage(path, idx);
	}

	me()->AddTotalKeyCount(-1);
}




M_PARAMS
void M_TYPE::RemoveRedundantRoot(TreePath& path, Int level)
{
	for (Int c = path.GetSize() - 1; c > level; c--)
	{
		NodeBaseG& node = path[c].node();

		if (node->children_count() == 1)
		{
			Metadata root_metadata = me()->GetRootMetadata();

			NodeBaseG& child = path[c - 1].node();

			me()->Node2Root(child, root_metadata);

			me()->allocator().RemovePage(node->id());

			me()->set_root(child->id());

			path.RemoveLast();
		}
		else {
			break;
		}
	}
}


M_PARAMS
void M_TYPE::RemoveRedundantRoot(TreePath& start, TreePath& stop, Int level)
{
	for (Int c = start.GetSize() - 1; c > level; c--)
	{
		NodeBaseG& node = start[c].node();

		if (node->children_count() == 1)
		{
			Metadata root_metadata = me()->GetRootMetadata();

			NodeBaseG& child = start[c - 1].node();

			if (me()->CanConvertToRoot(child))
			{
				me()->Node2Root(child, root_metadata);

				me()->allocator().RemovePage(node->id());

				me()->set_root(child->id());

				start.RemoveLast();

				stop.RemoveLast();
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
bool M_TYPE::MergeWithSiblings(TreePath& path, Int level, Int& key_idx)
{
	if (me()->MergeWithRightSibling(path, level))
	{
		return true;
	}
	else
	{
		return me()->MergeWithLeftSibling(path, level, key_idx);
	}
}


M_PARAMS
bool M_TYPE::MergeWithLeftSibling(TreePath& path, Int level, Int& key_idx)
{
	bool merged = false;

	if (me()->ShouldMergeNode(path, level))
	{
		TreePath prev = path;

		if (me()->GetPrevNode(prev, level))
		{
			Int size = prev[level]->children_count();

			merged = MergeBTreeNodes(prev, path, level);

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
bool M_TYPE::MergeWithRightSibling(TreePath& path, Int level)
{
	bool merged = false;

	if (me()->ShouldMergeNode(path, level))
	{
		TreePath next = path;

		if (me()->GetNextNode(next))
		{
			merged = MergeBTreeNodes(path, next, level);
		}
	}

	return merged;
}



M_PARAMS
bool M_TYPE::MergePaths(TreePath& tgt, TreePath& src, Int level)
{
	bool merged_at_level = false;

	for (Int c = tgt.GetSize() - 1; c >= level; c--)
	{
		if (CanMerge(tgt, src, c))
		{
			if (tgt[c] != src[c])
			{
				MergeNodes(tgt, src, c);

				src[c] = tgt[c];

				merged_at_level = c == level;
			}
		}
		else {
			break;
		}
	}

	RemoveRedundantRoot(tgt, src, level);

	return merged_at_level;
}


M_PARAMS
void M_TYPE::MergeNodes(TreePath& tgt, TreePath& src, Int level)
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

	RemoveRoom(src, level + 1, parent_idx, 1, accum, false);

	me()->UpdateUp(src, level + 1, parent_idx - 1, accum);

	me()->allocator().RemovePage(page2->id());

	src[level] = tgt[level];

	src.MoveRight(level - 1, 0, tgt_children_count);

	me()->Reindex(parent); //FIXME: does it necessary?
}

M_PARAMS
bool M_TYPE::MergeBTreeNodes(TreePath& tgt, TreePath& src, Int level)
{
	if (CanMerge(tgt, src, level))
	{
		if (IsTheSameParent(tgt, src, level))
		{
			MergeNodes(tgt, src, level);

			RemoveRedundantRoot(tgt, src, level);

			return true;
		}
		else
		{
			if (me()->MergeBTreeNodes(tgt, src, level + 1))
			{
				MergeNodes(tgt, src, level);

				RemoveRedundantRoot(tgt, src, level);

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
void M_TYPE::Drop()
{
	NodeBaseG root = me()->GetRoot(Allocator::READ);

	if (root.is_set())
	{
		me()->RemoveNode(root);
	}
}


#undef M_TYPE
#undef M_PARAMS


}

#endif
