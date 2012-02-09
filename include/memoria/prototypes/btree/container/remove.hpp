
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
    bool RemovePages(NodeBaseG& start, Int& start_idx, NodeBaseG& stop, Int& stop_idx, Key* keys);



    bool MergeWithSiblings(NodeBaseG& node)
    {
    	Int idx = 0;
    	return me()->MergeWithSiblings(node, idx);
    }

    bool MergeWithSiblings(NodeBaseG& node, Int& key_idx);

    /**
     * Remove key and data pointed by iterator 'iter' form the map.
     *
     */
    bool RemoveEntry(Iterator& iter) ;

    bool RemoveEntries(Iterator& from, Iterator& to);

    void Drop();

private:
    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    bool RemovePages(NodeBaseG& start, Int& start_idx, NodeBaseG& stop, Int& stop_idx, Key* keys_left, Key* keys_right, Int level);

    bool RemoveRedundantRoot(NodeBaseG& node, NodeBaseG& child);

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
    bool RemoveSpace(NodeBaseG& node, Int from, Int count, typename UpdateType::Enum update_type, bool remove_children, Key* keys = NULL);

    /**
     * Remove a page from the btree. Do recursive removing if page's parent
     * has no more children.
     *
     * If after removing the parent is less than half filled than
     * merge it with siblings.
     */

    void RemovePage(NodeBaseG node, typename UpdateType::Enum update_type = UpdateType::FULL);


    /**
     * Delete a node with it's children.
     */
    void RemoveNode(NodeBaseG node);

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
bool M_TYPE::RemoveSpace(NodeBaseG& node, Int from, Int count, typename UpdateType::Enum update_type, bool remove_children, Key* keys)
{
	if (count  == 0) return false;

	node.update();

	if (MapType == MapTypes::Sum)
	{
		me()->SumKeys(node, from, count, keys);
	}

	Counters counters;

	if (!node->is_leaf())
	{
		RemoveSpaceFn fn(from, count, remove_children, *me());
		NonLeafDispatcher::Dispatch(node, fn);
		counters = fn.counters();

		node->counters() -= counters;
	}
	else {
		node->counters().key_count() -= count;
		counters.key_count() = count;

		typename MyType::DataRemoveHandlerFn data_remove_handler_fn(from, count, *me());
		LeafDispatcher::Dispatch(node, data_remove_handler_fn);
	}

	me()->MoveChildrenLeft(node, from, count);

	if (update_type == UpdateType::FULL)
	{
		me()->UpdateBTreeKeys(node);
	}
	else if (MapType == MapTypes::Sum && update_type == UpdateType::PARENT_ONLY && !node->is_root())
	{
		Key keys0[Indexes];
		me()->NegateKeys(keys0, keys);

		NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);
		me()->AddKeys(parent, node->parent_idx(), keys0, false);
	}

	if (update_type != UpdateType::NONE && !node->is_root())
	{
		NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);
		me()->UpdateBTreeCounters(parent, -counters);
	}

	return true;
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
			me()->AddKeys(parent, parent_idx - 1, keys, false);
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
			MEMORIA_TRACE(me(),"MergeBTReeNodes with the same parent");
			me()->MergeNodes(page1, page2);

			NodeBaseG parent = me()->GetParent(page1, Allocator::READ);
			if (parent->is_root() && parent->children_count() == 1 && me()->CanConvertToRoot(page1))
			{
				Metadata meta = me()->GetRootMetadata(parent);

				me()->Node2Root(page1, meta);

				me()->set_root(page1->id());
				page1->parent_idx() = 0;

				me()->allocator().RemovePage(parent->id());
			}

			return true;
		}
		else
		{
			//FIXME Use more clever logic for the update flag
			NodeBaseG parent1 = me()->GetParent(page1, Allocator::UPDATE);
			NodeBaseG parent2 = me()->GetParent(page2, Allocator::UPDATE);
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
bool M_TYPE::RemovePages(NodeBaseG& start, Int& start_idx, NodeBaseG& stop, Int& stop_idx, Key* keys)
{
	Key keys_right[Indexes];
	me()->ClearKeys(keys_right);

	bool result = RemovePages(start, start_idx, stop, stop_idx, keys, keys_right, 0);

	if (start != NULL && stop == NULL)
	{
		stop 		= start;
		stop_idx 	= start_idx;
	}
	else //if (start == NULL && stop != NULL)
	{
		start 		= stop;
		start_idx 	= stop_idx;
	}

	return result;
}


M_PARAMS
bool M_TYPE::RemovePages(NodeBaseG& start, Int& start_idx, NodeBaseG& stop, Int& stop_idx, Key* keys_left, Key* keys_right, Int level)
{
	if (start->id() == stop->id())
	{
		// Removal within the same BTree node

		me()->AddKeys(keys_left, keys_right);

		bool affected = false;

		if (start_idx == 0 && stop_idx == start->children_count())
		{
			if (!start->is_root())
			{
				Iterator i(*me());

				NodeBaseG parent 	= me()->GetParent(start, Allocator::UPDATE);
				Int parent_idx 		= start->parent_idx();

				start 		= i.GetPrevNode(start);
				start_idx	= start != NULL ? start->children_count() : 0;

				stop		= i.GetNextNode(start);
				stop_idx 	= 0;

				affected = RemoveSpace(parent, parent_idx, 1, UpdateType::FULL, true, keys_left) || affected;

				if (!parent->is_root() && me()->ShouldMerge(parent))
				{
					me()->MergeWithSiblings(parent);
				}

			}
			else {
				me()->RemoveNode(start);
				me()->set_root(ID(0));

				start 		= NULL;
				stop  		= NULL;
				start_idx 	= stop_idx = 0;

				affected 	= true;
			}
		}
		else if (stop_idx - start_idx >= 1)
		{
			//Remove some space within the node

			affected = RemoveSpace(start, start_idx, stop_idx - start_idx, UpdateType::FULL, true, keys_left) || affected;

			if (me()->ShouldMerge(start))
			{
				me()->MergeWithSiblings(start, start_idx);
				stop = start;
			}

			stop_idx = start_idx;
		}

		//FIXME: check return status
		return affected;
	}
	else
	{
		// The region to remove crosses node boundaries.
		// We need to up the tree until we found the node
		// enclosing the region. See the code branch above.

		NodeBaseG start_parent  = me()->GetParent(start, Allocator::UPDATE);
		NodeBaseG stop_parent   = me()->GetParent(stop,  Allocator::UPDATE);

		Int start_parent_idx 	= start->parent_idx() + 1;
		Int stop_parent_idx  	= stop->parent_idx();

		bool affected = false;

		Iterator i(*me());

		if (start_idx > 0)
		{
			affected = RemoveSpace(start, start_idx, start->children_count() - start_idx, UpdateType::PARENT_ONLY, true, keys_left);
		}
		else
		{
			start_parent_idx--;

			start = i.GetPrevNode(start);

			if (start != NULL)
			{
				start_idx = start->children_count();
			}
			else {
				start_idx = 0;
			}
		}

		if (stop_idx < stop->children_count())
		{
			affected = RemoveSpace(stop, 0, stop_idx, UpdateType::PARENT_ONLY, true, keys_right) || affected;
		}
		else
		{
			stop_parent_idx++;
			stop = i.GetNextNode(stop);
		}

		stop_idx = 0;

		affected = RemovePages(start_parent, start_parent_idx, stop_parent, stop_parent_idx, keys_left, keys_right, level + 1) || affected;


		if (start_parent != NULL)
		{
			if (start != NULL && stop != NULL && IsTheSameParent(start, stop) && CanMerge(start, stop))
			{
				MergeNodes(start, stop);

				stop 	 = start;
				stop_idx = start_idx;
			}

			if (start_parent->is_root() && start_parent->children_count() == 1)
			{
				Metadata meta = me()->GetRootMetadata(start_parent);

				if (start != NULL)
				{
					me()->Node2Root(start, meta);
					me()->set_root(start->id());
				}
				else {
					me()->Node2Root(stop, meta);
					me()->set_root(stop->id());
				}

				me()->allocator().RemovePage(start_parent->id());
			}

			if (start != NULL && me()->ShouldMerge(start))
			{
				me()->MergeWithSiblings(start, start_idx);
			}

			if (stop != NULL && me()->ShouldMerge(stop))
			{
				me()->MergeWithSiblings(stop, stop_idx);
			}
		}
		else if (stop_parent == NULL) {
			start 		= stop 		= NULL;
			start_idx 	= stop_idx 	= 0;
		}

		return affected;
	}
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
				Metadata root_metadata = me()->GetRootMetadata(parent);

				me()->Node2Root(node, root_metadata);
				me()->allocator().RemovePage(parent->id());
				me()->set_root(node->id());

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
bool M_TYPE::MergeWithSiblings(NodeBaseG& node, Int& key_idx)
{
	Iterator tmp(*me());

	bool merged = false;

	NodeBaseG next = tmp.GetNextNode(node);

	if (next != NULL)
	{
		if (!MergeBTreeNodes(node, next))
		{
			NodeBaseG prev = tmp.GetPrevNode(node);
			if (prev != NULL)
			{
				Int size = prev->children_count();

				merged = MergeBTreeNodes(prev, node);

				if (merged)
				{
					node = prev;
					key_idx += size;
				}
			}
			else {
				merged = false;
			}
		}
		else {
			merged = true;
		}
	}
	else
	{
		NodeBaseG prev = tmp.GetPrevNode(node);
		if (prev != NULL)
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

	if (!node->is_root())
	{
		NodeBaseG parent = me()->GetParent(node, Allocator::READ);
		RemoveRedundantRoot(parent, node);
	}

	return merged;
}

/**
 * Remove a page from BTree. Do recursive removal if the page's parent
 * has no more children.
 *
 * If after removing the parent is less than half filled than
 * merge it with siblings.
 */

M_PARAMS
void M_TYPE::RemovePage(NodeBaseG node, typename UpdateType::Enum update_type)
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
			me()->RemoveSpace(parent, node->parent_idx(), 1, update_type, true, keys);

			//if after removing parent is less than half filled than
			//merge it with it's siblings if possible
			if (me()->ShouldMerge(parent))
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



/**
 * Remove key and data pointed by iterator 'iter' form the map.
 *
 */
M_PARAMS
bool M_TYPE::RemoveEntry(Iterator& iter)
{
	if (iter.IsNotEmpty() || iter.IsNotEnd())
	{
		NodeBaseG& node = iter.page();
		Int& idx = iter.key_idx();

		Int children_count = node->children_count();

		//if leaf page has more than 1 key do regular remove

		Key keys[Indexes];

		if (children_count > 1)
		{
			//remove 1 element rom the leaf, update parent and
			//do not try to remove children (it's a leaf)
			me()->ClearKeys(keys);
			me()->RemoveSpace(node, idx, 1, UpdateType::FULL, false, keys);

			//try merging this leaf with previous of following
			//leaf if filled by half of it's capacity.
			if (node->children_count() > me()->GetMaxCapacity(node) / 2)
			{
				me()->MergeWithSiblings(node, idx);
			}

			if (idx == node->children_count())
			{
				NodeBaseG next = iter.GetNextNode(node);
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

		if (MapType == MapTypes::Sum && iter.IsNotEnd())
		{
			me()->AddKeysUp(node, idx, keys);
		}

		return true;
	}
	else {
		return false;
	}
}

/**
 * FIXME: check for iterator correctness after the operation is completed.
 *
 */

M_PARAMS
bool M_TYPE::RemoveEntries(Iterator& from, Iterator& to)
{
	if (from.IsEmpty() || from.IsEnd())
	{
		return false;
	}

	if (to.IsEmpty())
	{
		return false;
	}

	Int    stop_idx = to.key_idx();

	if (to.key_idx() == 0 && to.PrevLeaf())
	{
		stop_idx = to.page()->children_count();
	}
	else
	{
		stop_idx = to.key_idx();
	}

	Key keys[Indexes];
	me()->ClearKeys(keys);

	bool result = me()->RemovePages(from.page(), from.key_idx(), to.page(), stop_idx, keys);

	to.key_idx() = stop_idx;

	if (to.IsNotEnd())
	{
		me()->AddKeysUp(to.page(), to.key_idx(), keys);
	}

	return result;
}

M_PARAMS
void M_TYPE::Drop()
{
	NodeBaseG root = me()->GetRoot(Allocator::READ);

	if (root != NULL) {
		me()->RemovePage(root);
	}
}


#undef M_TYPE
#undef M_PARAMS


}

#endif
