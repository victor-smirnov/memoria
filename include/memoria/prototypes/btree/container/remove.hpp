
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

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                              Allocator;

    typedef typename Base::Page                                                 Page;
    typedef typename Base::ID                                                   ID;


    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::TreeNodePage                                     	TreeNodePage;
    typedef typename Base::Counters                                             Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Base::Node2RootMap                                         Node2RootMap;
    typedef typename Base::Root2NodeMap                                         Root2NodeMap;

    typedef typename Base::NodeFactory                                          NodeFactory;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    static const Int  Indexes                                                   = Base::Indexes;
    static const bool MapType                                                   = Types::MapType;

    typedef typename Base::Metadata                                             Metadata;

    void move() {}

    void PreMerge(NodeBaseG& one, NodeBase& two) {}
    
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


    /**
     * For Index type BTree if we remove some entries we need to preserve absolute key values
     * from the right. Removed values will be returned in 'keys' output parameter.
     *
     * For structures like DynVector if we remove range we have to substruct the values from the right index cell manually.
     *
     * FIXME: optimize. Add a flag for absolute key value preserving.
     *
     */
    BigInt RemovePages(NodeBaseG& start, Int& start_idx, NodeBaseG& stop, Int& stop_idx, Key* keys, bool merge = true);

    void RemovePages(NodeBaseG& start, Int& start_idx, NodeBaseG& stop, Int& stop_idx, Key* keys_left, Key* keys_right, BigInt& removed_key_count);


    bool MergeWithSiblings(NodeBaseG& node)
    {
    	Int idx = 0;
    	return me()->MergeWithSiblings(node, idx);
    }

    bool MergeWithLeftSibling(NodeBaseG& node, Int& key_idx);
    bool MergeWithRightSibling(NodeBaseG& node);
    bool MergeWithSiblings(NodeBaseG& node, Int& key_idx);

    /**
     * Remove key and data pointed by iterator 'iter' form the map.
     *
     */
    bool RemoveEntry(Iterator& iter, bool preserve_key_values = true);

    void RemoveEntry(NodeBaseG& node, Int& idx, Key* keys);

    BigInt RemoveEntries(Iterator& from, Iterator& to);
    BigInt RemoveEntries(Iterator& from, Iterator& to, Key* keys);

    void Drop();

private:
    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------



    bool RemoveRedundantRoot(NodeBaseG& node, NodeBaseG& child);
    void RemoveSingularNodeChain(NodeBaseG& node, Int key_idx);

    struct RemoveSpaceFn {
    	Counters    counters_;
    	Int         from_;
    	Int         count_;
    	bool        remove_children_;
    	MyType&     map_;
    	Key         keys[Indexes];

    public:
    	RemoveSpaceFn(Int from, Int count, bool remove_children, MyType &map):
    		from_(from), count_(count), remove_children_(remove_children),
    		map_(map) {}

    	template <typename T>
    	void operator()(T *node) {

    		if (MapType == MapTypes::Sum)
    		{
    			for (Int d = 0; d < Indexes; d++)
    			{
    				keys[d] = 0;
    				for (Int c = from_; c < from_ + count_; c++)
    				{
    					keys[d] += node->map().key(d, c);
    				}
    			}
    		}

    		for (Int c = from_; c < from_ + count_; c++)
    		{
    			//decrement node.page_count for child's value
    			//and remove child if requested

    			NodeBaseG child = map_.GetChild(node, c, Allocator::READ);
    			counters_ += child->counters();

    			if (remove_children_)
    			{
    				map_.RemoveNode(child);
    			}
    		}
    	}

    	Counters counters() const {
    		return counters_;
    	}
    };



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
    BigInt RemoveSpace(NodeBaseG& node, Int from, Int count, typename UpdateType::Enum update_type, Key* keys = NULL, bool remove_children = true);

    /**
     * Remove a page from the btree. Do recursive removing if page's parent
     * has no more children.
     *
     * If after removing the parent is less than half filled than
     * merge it with siblings.
     */

    void RemovePage(NodeBaseG& node, typename UpdateType::Enum update_type = UpdateType::FULL);


    /**
     * Delete a node with it's children.
     */
    void RemoveNode(NodeBaseG node);

    bool ChangeRootIfSingular(NodeBaseG& parent, NodeBaseG& node);

    bool CanMerge(NodeBaseG& page1, NodeBaseG& page2)
    {
    	return page2->children_count() <= me()->GetCapacity(page1);
    }


    static bool IsTheSameParent(NodeBaseG& page1, NodeBaseG& page2)
    {
    	return page1->parent_id() == page2->parent_id();
    }

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

    		page2->map().CopyData(0, page2->children_count(), page1->map(), page1->children_count());
    		page1->inc_size(page2->children_count());
    		page1->map().Reindex();
    	}

    	Int start() const {
    		return start_;
    	}
    };

    void MergeNodes(NodeBaseG& page1, NodeBaseG& page2, bool fix_parent = true);


    bool MergeBTreeNodes(NodeBaseG& page1, NodeBaseG& page2);

    void MoveChildrenLeft(NodeBaseG& node, Int from, Int count);

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::RemoveName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::MoveChildrenLeft(NodeBaseG& node, Int from, Int count)
{
    node.update();
	RemoveElements<NodeDispatcher>(node.page(), from, count, true);

    if (!node->is_leaf())
    {
        IncrementChildPids<NonLeafDispatcher, TreeNodePage>(node.page(), from, -count, me()->allocator());
    }
    else if (me()->IsDynarray())
    {
    	IncrementChildPids<LeafDispatcher, TreeNodePage>(node.page(), from, -count, me()->allocator());
    }
}

M_PARAMS
BigInt M_TYPE::RemoveSpace(NodeBaseG& node, Int from, Int count, typename UpdateType::Enum update_type, Key* keys, bool remove_children)
{
	//FIXME: optimize for the case when count == 0

	node.update();

	if (MapType == MapTypes::Sum)
	{
		me()->SumKeys(node, from, count, keys);
	}

	Counters counters;

	BigInt key_count;

	if (!node->is_leaf())
	{
		RemoveSpaceFn fn(from, count, remove_children, *me());
		NonLeafDispatcher::Dispatch(node, fn);
		counters = fn.counters();

		key_count = counters.key_count();

		node->counters() -= counters;
	}
	else {
		node->counters().key_count() -= count;
		counters.key_count() = count;

		key_count = count;

		typename MyType::DataRemoveHandlerFn data_remove_handler_fn(from, count, *me());
		LeafDispatcher::Dispatch(node, data_remove_handler_fn);
	}

	me()->MoveChildrenLeft(node, from, count);

	if (MapType == MapTypes::Sum)
	{
		if (!node->is_root())
		{
			NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);

			Key keys0[Indexes];
			me()->NegateKeys(keys0, keys);

			if (update_type == UpdateType::FULL)
			{
				me()->AddKeysUp(parent, node->parent_idx(), keys0);
			}
			else if (update_type == UpdateType::PARENT_ONLY)
			{
				me()->AddKeys(parent, node->parent_idx(), keys0);
			}
		}
	}
	else if (update_type != UpdateType::NONE)
	{
		me()->UpdateBTreeKeys(node);
	}


	if (update_type != UpdateType::NONE && !node->is_root())
	{
		NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);
		me()->UpdateBTreeCounters(parent, -counters);
	}

	return key_count;
}





M_PARAMS
BigInt M_TYPE::RemovePages(NodeBaseG& start, Int& start_idx, NodeBaseG& stop, Int& stop_idx, Key* keys, bool merge)
{
	Key keys_right[Indexes];
	me()->ClearKeys(keys_right);

	BigInt removed_key_count = 0;
	RemovePages(start, start_idx, stop, stop_idx, keys, keys_right, removed_key_count);

	if (start.is_set() && stop.is_empty())
	{
		me()->MergeWithLeftSibling(start, start_idx);

		stop 		= start;
		stop_idx 	= start_idx;
	}
	else if (start.is_empty() && stop.is_set())
	{
		me()->MergeWithRightSibling(stop);

		start 		= stop;
		start_idx 	= stop_idx;
	}
	else if (start.is_set() && stop.is_set())
	{
		me()->MergeWithSiblings(stop, stop_idx);

		start 		= stop;
		start_idx 	= stop_idx;
	}
	else {
		start_idx 	= stop_idx = 0;
	}

	return removed_key_count;
}



M_PARAMS
void M_TYPE::RemovePages(NodeBaseG& start, Int& start_idx, NodeBaseG& stop, Int& stop_idx, Key* keys_left, Key* keys_right, BigInt& removed_key_count)
{
	if (start.is_set() && stop.is_set() && me()->IsTheSameNode(start, stop))
	{
		// The root node of removed subtree

		me()->AddKeys(keys_left, keys_right);

		if (start_idx == 0 && stop_idx == stop->children_count())
		{
			// Special case for the most right leaf in the tree.

			Iterator i(*me());
			NodeBaseG prev = i.GetPrevNode(start);

			me()->RemovePage(start);

			stop 		= NULL;
			stop_idx 	= 0;

			start = prev;

			start_idx = start.is_set() ? start->children_count() : 0;

		}
		else if (stop_idx - start_idx >= 0)
		{
			//Remove some space within the node
			removed_key_count += RemoveSpace(start, start_idx, stop_idx - start_idx, UpdateType::FULL, keys_left);

			me()->SetKeys(keys_right, keys_left);

			if (!start->is_root())
			{
				NodeBaseG parent = me()->GetParent(start, Allocator::READ);
				RemoveRedundantRoot(parent, start);
			}

			stop_idx = start_idx;
		}
		else {

			me()->SetKeys(keys_right, keys_left);
		}
	}
	else
	{
		// The region to remove crosses node boundaries.
		// We need to up the tree until we found the node
		// enclosing the region. See the code branch above.

		NodeBaseG stop_parent;
		NodeBaseG start_parent;

		Int start_parent_idx	= 0;
		Int stop_parent_idx		= 0;

		Iterator i(*me());

		if (start.is_set())
		{
			if (start->is_root())
			{
				removed_key_count += RemoveSpace(start, start_idx, start->children_count() - start_idx, UpdateType::NONE, keys_left);
				me()->SetKeys(keys_right, keys_left);
			}
			else
			{
				if (start_idx > 0)
				{
					removed_key_count 	+= RemoveSpace(start, start_idx, start->children_count() - start_idx, UpdateType::PARENT_ONLY, keys_left);

					start_parent 		= me()->GetParent(start, Allocator::UPDATE);
					start_parent_idx 	= start->parent_idx() + 1;
				}
				else
				{
					start = i.GetPrevNode(start);

					if (start.is_set())
					{
						start_parent 		= me()->GetParent(start, Allocator::UPDATE);
						start_parent_idx 	= start->parent_idx() + 1;
						start_idx			= start->children_count();
					}
					else {
						start_idx 			= 0;
					}
				}
			}
		}


		if (stop.is_set())
		{
			if (stop->is_root())
			{
				removed_key_count += RemoveSpace(stop, 0, stop_idx, UpdateType::NONE, keys_right);
				me()->SetKeys(keys_left, keys_right);
			}
			else
			{
				if (stop_idx < stop->children_count())
				{
					removed_key_count 	+= RemoveSpace(stop, 0, stop_idx, UpdateType::PARENT_ONLY, keys_right);

					stop_parent 		= me()->GetNodeParent(stop, start_parent, Allocator::UPDATE);
					stop_parent_idx 	= stop->parent_idx();
				}
				else
				{
					stop = i.GetNextNode(stop);

					if (stop.is_set())
					{
						stop_parent 		= me()->GetNodeParent(stop, start_parent, Allocator::UPDATE);
						stop_parent_idx 	= stop->parent_idx();
					}
				}
			}
		}

		stop_idx = 0;


		if (start_parent.is_set() || stop_parent.is_set())
		{
			RemovePages(start_parent, start_parent_idx, stop_parent, stop_parent_idx, keys_left, keys_right, removed_key_count);

			if (start_parent.is_set() && stop_parent.is_set())
			{
				if(start_parent->id() == stop_parent->id())
				{
					if (start.is_set() && stop.is_set())
					{
						if (CanMerge(start, stop))
						{
							MergeNodes(start, stop);

							stop 	 = start;
							stop_idx = start_idx;

							ChangeRootIfSingular(start_parent, start);
						}
					}
				}
			}
			else if (start.is_set() && stop.is_empty())
			{
				ChangeRootIfSingular(start_parent, start);
			}
			else if (start.is_empty() && stop.is_set())
			{
				ChangeRootIfSingular(stop_parent, stop);
			}
		}
		else if (start.is_empty() && stop.is_empty())
		{
			if (me()->root().is_set())
			{
				NodeBaseG root = me()->GetRoot(Allocator::UPDATE);

				removed_key_count = root->counters().key_count();

				me()->GetMaxKeys(root, keys_left);
				me()->SetKeys(keys_right, keys_left);

				me()->RemoveNode(root);
			}
			else {
				// the container is empty
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
 */

M_PARAMS
void M_TYPE::RemovePage(NodeBaseG& node, typename UpdateType::Enum update_type)
{
	if (!node->is_root())
	{
		NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);

		//recursively remove parent if has only one child.

		if (parent->children_count() == 1)
		{
			me()->RemovePage(parent);
		}
		else {
			//Remove element from parent ponting tho this node.
			//This node (it is a child there) will be removed automatically
			//and all parents chain will be updated.
			Key keys[Indexes];
			me()->ClearKeys(keys);
			me()->RemoveSpace(parent, node->parent_idx(), 1, update_type, keys);

			//if after removing parent is less than half filled than
			//merge it with it's siblings if possible
			if (me()->ShouldMergeNode(parent))
			{
				me()->MergeWithSiblings(parent);
			}
		}
	}
	else {
		me()->RemoveNode(node);
	}
}


M_PARAMS
void M_TYPE::RemoveNode(NodeBaseG node)
{
	const Int children_count = node->children_count();

	if (!node->is_leaf())
	{
		for (Int c = 0; c < children_count; c++)
		{
			NodeBaseG child = me()->GetChild(node, c, Allocator::READ);
			me()->RemoveNode(child);
		}
	}
	else {
		typename MyType::DataRemoveHandlerFn data_remove_handler_fn(0, children_count, *me());
		LeafDispatcher::Dispatch(node, data_remove_handler_fn);
	}

	if (node->is_root())
	{
		ID id;
		id.set_null();
		me()->set_root(id);
	}

	me()->allocator().RemovePage(node->id());
}


M_PARAMS
bool M_TYPE::ChangeRootIfSingular(NodeBaseG& parent, NodeBaseG& node)
{
	if (parent.is_set() && parent->is_root() && parent->children_count() == 1)
	{
		Metadata meta = me()->GetRootMetadata(parent);

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
bool M_TYPE::RemoveEntry(Iterator& iter, bool preserve_key_values)
{
	if (iter.IsNotEnd())
	{
		Key keys[Indexes];
		me()->ClearKeys(keys);

		RemoveEntry(iter.page(), iter.key_idx(), keys);

		if (MapType == MapTypes::Sum && iter.IsNotEnd())
		{
			me()->AddKeysUp(iter.page(), iter.key_idx(), keys);
		}

		return true;
	}
	else {
		return false;
	}
}



M_PARAMS
void M_TYPE::RemoveEntry(NodeBaseG& node, Int& idx, Key* keys)
{
	Int children_count = node->children_count();

	//if leaf page has more than 1 key do regular remove

	if (children_count > 1)
	{
		//remove 1 element rom the leaf, update parent and

		me()->RemoveSpace(node, idx, 1, UpdateType::FULL, keys);

		//try merging this leaf with previous of following
		//leaf if filled by half of it's capacity.
		if (node->children_count() > me()->GetMaxCapacity(node) / 2)
		{
			me()->MergeWithSiblings(node, idx);
		}

		if (idx == node->children_count())
		{
			Iterator i(*me());
			NodeBaseG next = i.GetNextNode(node);
			if (next != NULL)
			{
				node 	= next;
				idx 	= 0;
			}
		}
	}
	else {
		//otherwise remove the leaf page.
		//FIXME: preserve key values?

		me()->GetKeys(node, 0, keys);

		Iterator i(*me());

		Int		  new_idx;

		NodeBaseG new_node = i.GetNextNode(node);

		if (new_node != NULL)
		{
			new_idx = 0;
		}
		else
		{
			new_node = i.GetPrevNode(node);
			if (new_node != NULL)
			{
				new_idx = new_node->children_count();
			}
			else {
				new_idx = 0;
			}
		}

		me()->RemovePage(node);

		node 	= new_node;
		idx		= new_idx;
	}
}

M_PARAMS
BigInt M_TYPE::RemoveEntries(Iterator& from, Iterator& to)
{
	Key keys[Indexes];
	me()->ClearKeys(keys);

	BigInt removed = me()->RemoveEntries(from, to, keys);

	if (to.IsNotEnd())
	{
		me()->AddKeysUp(to.page(), to.key_idx(), keys);
	}

	return removed;
}


M_PARAMS
BigInt M_TYPE::RemoveEntries(Iterator& from, Iterator& to, Key* keys)
{
	if (from.IsEmpty() || from.IsEnd())
	{
		return 0;
	}

	if (to.IsEmpty())
	{
		return 0;
	}

	return me()->RemovePages(from.page(), from.key_idx(), to.page(), to.key_idx(), keys);
}



M_PARAMS
bool M_TYPE::RemoveRedundantRoot(NodeBaseG& node, NodeBaseG& child)
{
	if (node->is_root())
	{
		if (node->children_count() == 1)
		{
			Metadata root_metadata = me()->GetRootMetadata(node);

			me()->Node2Root(child, root_metadata);
			me()->allocator().RemovePage(node->id());
			me()->set_root(child->id());

			return true;
		}
		else {
			return false;
		}
	}
	else
	{
		NodeBaseG parent = me()->GetParent(node, Allocator::READ);
		if (RemoveRedundantRoot(parent, node))
		{
			if (node->children_count() == 1)
			{
				Metadata root_metadata = me()->GetRootMetadata(node);

				me()->Node2Root(child, root_metadata);
				me()->allocator().RemovePage(node->id());
				me()->set_root(child->id());

				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}
}


M_PARAMS
void M_TYPE::RemoveSingularNodeChain(NodeBaseG& node, Int key_idx)
{
	if (node->children_count() > 1)
	{
		Key keys[Indexes];
		me()->ClearKeys(keys);
		RemoveSpace(node, key_idx, 1, UpdateType::FULL, keys);
	}
	else if (node->is_root())
	{
		me()->RemoveNode(node);
	}
	else
	{
		NodeBaseG parent = me()->GetParent(node, Allocator::READ);
		RemoveSingularNodeChain(parent, node->parent_idx());
	}
}



M_PARAMS
bool M_TYPE::MergeWithSiblings(NodeBaseG& node, Int& key_idx)
{
	if (me()->MergeWithRightSibling(node))
	{
		return true;
	}
	else
	{
		return me()->MergeWithLeftSibling(node, key_idx);
	}
}


M_PARAMS
bool M_TYPE::MergeWithLeftSibling(NodeBaseG& node, Int& key_idx)
{
	bool merged = false;

	if (me()->ShouldMergeNode(node))
	{
		Iterator tmp(*me());

		NodeBaseG prev = tmp.GetPrevNode(node);
		if (prev.is_set())
		{
			Int size = prev->children_count();

			merged = MergeBTreeNodes(prev, node);

			if (merged)
			{
				key_idx += size;
				node = prev;
			}
		}
		else {
			merged = false;
		}

	}

	return merged;
}


M_PARAMS
bool M_TYPE::MergeWithRightSibling(NodeBaseG& node)
{
	bool merged = false;

	if (me()->ShouldMergeNode(node))
	{
		Iterator tmp(*me());

		NodeBaseG next = tmp.GetNextNode(node);

		if (next.is_set())
		{
			merged = MergeBTreeNodes(node, next);
		}
	}

	return merged;
}




M_PARAMS
void M_TYPE::MergeNodes(NodeBaseG& page1, NodeBaseG& page2, bool fix_parent)
{
	page1.update();

	MergeNodesFn fn;
	NodeDispatcher::Dispatch(page1, page2, fn);
	Int fn_start = fn.start();

	if (!page1->is_leaf())
	{
		IncrementChildPidsAndReparent<NonLeafDispatcher, TreeNodePage>(page1.page(), fn_start, fn_start, me()->allocator());
	}
	else if (me()->IsDynarray())
	{
		IncrementChildPidsAndReparent<LeafDispatcher, TreeNodePage>(page1.page(), fn_start, fn_start, me()->allocator());
	}

	page1->counters() += page2->counters();
	page1->counters().page_count() -= 1;

	if (fix_parent)
	{
		NodeBaseG parent = me()->GetParent(page1, Allocator::UPDATE);
		Int parent_idx = page2->parent_idx();

		Key keys[Indexes];
		for (Int c = 0; c <Indexes; c++) keys[c] = 0;

		me()->GetKeys(parent, parent_idx, keys);
		if (MapType == MapTypes::Value)
		{
			me()->SetKeys(parent, parent_idx - 1, keys);
		}
		else {
			me()->AddKeys(parent, parent_idx - 1, keys);
		}

		MoveChildrenLeft(parent, parent_idx, 1);

		me()->UpdateBTreeCounters(parent, Counters(-1, 0));

		ReindexFn fn;
		NodeDispatcher::Dispatch(parent, fn);
	}

	me()->allocator().RemovePage(page2->id());

	page2 = page1;
}

M_PARAMS
bool M_TYPE::MergeBTreeNodes(NodeBaseG& page1, NodeBaseG& page2)
{
	if (CanMerge(page1, page2))
	{
		if (IsTheSameParent(page1, page2))
		{
			me()->MergeNodes(page1, page2);

			NodeBaseG parent = me()->GetParent(page1, Allocator::READ);

			me()->RemoveRedundantRoot(parent, page1);

			return true;
		}
		else
		{
			//FIXME Use more clever logic for the update flag
			NodeBaseG parent1 = me()->GetParent(page1, Allocator::READ);
			NodeBaseG parent2 = me()->GetParent(page2, Allocator::READ);

			if (me()->MergeBTreeNodes(parent1, parent2))
			{
				MergeNodes(page1, page2);
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
