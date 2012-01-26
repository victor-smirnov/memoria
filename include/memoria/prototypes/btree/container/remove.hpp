
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
    
    void MoveChildrenLeft(NodeBaseG& node, Int from, Int count);



    
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


    struct DataRemoveHandlerFn {

    	Int idx_, count_;
    	MyType& me_;

    	DataRemoveHandlerFn(Int idx, Int count, MyType& me): idx_(idx), count_(count), me_(me) {}

    	template <typename Node>
    	void operator()(Node* node) {}
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
    bool RemoveSpace(NodeBaseG& node, Int from, Int count, bool update, bool remove_children, Key* keys = NULL, bool preserve_key_values = true);

    void RemoveNode(NodeBaseG node);

    bool CanMerge(NodeBase *page1, NodeBase *page2)
    {
        return me()->GetChildrenCount(page2) <= me()->GetCapacity(page1);
    }

    static bool IsSameParent(NodeBase *page1, NodeBase *page2)
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
        void operator()(T1 *page1, T2 *page2) {
            start_ = page1->map().size();

            page2->map().CopyData(0, page2->map().size(), page1->map(), page1->map().size());
            page1->map().size() += page2->map().size();
            page1->map().Reindex();
        }

        Int start() const {
            return start_;
        }
    };

    void MergeNodes(NodeBaseG& page1, NodeBaseG& page2, bool fix_parent = true);


    bool MergeBTreeNodes(NodeBaseG& page1, NodeBaseG& page2);


    /**
     * For Index type BTree if we remove some entries we need to preserve absolute key values
     * from the right. Removed values will be returned in 'keys' output parameter.
     *
     * For structures like DynVector if we remove range we have to substruct the values from the right index cell manually.
     *
     * FIXME: optimize. Add a flag for absolute key value preserving.
     *
     */
    bool RemovePages(NodeBaseG start, Int start_idx, NodeBaseG stop, Int stop_idx, Key* keys, bool preserve_key_values = true);


    bool MergeWithSiblings(NodeBaseG& node);

    /**
     * Remove a page from the btree. Do recursive removing if page's parent
     * has no more children.
     *
     * If after removing the parent is less than half filled than
     * merge it with siblings.
     */

    void RemovePage(NodeBaseG node);

    /**
     * Remove key and data pointed by iterator 'iter' form the map.
     *
     */
    bool RemoveEntry(Iterator iter) ;

    bool RemoveEntries(Iterator from, Iterator to);

    void Drop();

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
bool M_TYPE::RemoveSpace(NodeBaseG& node, Int from, Int count, bool update, bool remove_children, Key* keys, bool preserve_key_values)
{
	if (count  == 0) return false;

	Key keys0[Indexes] = {0};

	node.update();

	if (MapType == MapTypes::Sum)
	{
		bool upd0;
		if (keys == NULL)
		{
			keys = keys0;
			upd0 = true;
		}
		else {
			upd0 = false;
		}

		me()->SumKeys(node, from, count, keys);

		if (upd0 && preserve_key_values)
		{
			Int size = me()->GetChildrenCount(node);
			if (from + count < size)
			{
				me()->AddKeys(node, from + count, keys);
			}
			else {
				Iterator tmp(*me());
				NodeBaseG next = tmp.GetNextNode(node);
				if (next != NULL)
				{
					me()->AddKeys(next, 0, keys);
					me()->UpdateBTreeKeys(next);
				}
			}
		}
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

	if (update)
	{
		me()->UpdateBTreeKeys(node);

		NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);
		if (parent != NULL) {
			me()->UpdateBTreeCounters(parent, -counters);
		}
	}

	return true;
}

M_PARAMS
void M_TYPE::RemoveNode(NodeBaseG node)
{
	const Int children_count = me()->GetChildrenCount(node);


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
}

M_PARAMS
bool M_TYPE::MergeBTreeNodes(NodeBaseG& page1, NodeBaseG& page2)
{
	if (CanMerge(page1, page2))
	{
		if (IsSameParent(page1, page2))
		{
			MEMORIA_TRACE(me(),"MergeBTReeNodes with the same parent");
			me()->MergeNodes(page1, page2);

			NodeBaseG parent = me()->GetParent(page1, Allocator::READ);
			if (parent->is_root() && me()->GetChildrenCount(parent) == 1 && me()->CanConvertToRoot(page1))
			{
				me()->Node2Root(page1);

				me()->CopyRootMetadata(parent, page1);

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
bool M_TYPE::RemovePages(NodeBaseG start, Int start_idx, NodeBaseG stop, Int stop_idx, Key* keys, bool preserve_key_values)
{
	if (start == NULL || stop == NULL)
	{
		MEMORIA_TRACE(me(), "RemovePages", start, stop);
		return false;
	}

	MEMORIA_TRACE(me(), "RemovePages", start->id(), start_idx, stop->id(), stop_idx);

	if (start->id() == stop->id())
	{
		bool affected = false;

		Int children_count = me()->GetChildrenCount(start);

		if (start_idx == -1 && stop_idx == children_count)
		{
			MEMORIA_TRACE(me(), "RemovePages: full page");

			NodeBaseG parent;
			Int parent_idx = start->parent_idx();

			while (!start->is_root())
			{
				parent      = me()->GetParent(start, Allocator::UPDATE);
				parent_idx  = start->parent_idx();

				if (me()->GetChildrenCount(parent) > 1)
				{
					affected = me()->RemoveSpace(parent, parent_idx, 1, true, true, keys, preserve_key_values) || affected;
					break;
				}
				else
				{
					start = parent;
				}
			}

			if (start->is_root())
			{
				me()->RemoveNode(start);
				affected = true;
				me()->set_root(ID(0));
			}
			else if (MapType == MapTypes::Sum && preserve_key_values && parent != NULL)
			{
				if (parent_idx <= me()->GetChildrenCount(parent) - 1)
				{
					MEMORIA_TRACE(me(), "RemovePages: within ranges", parent_idx, me()->GetChildrenCount(parent));
					me()->AddKeys(parent, parent_idx, keys);
					me()->UpdateBTreeKeys(parent);
				}
				else {
					MEMORIA_TRACE(me(), "RemovePages: out of ranges", parent_idx, me()->GetChildrenCount(parent));

					parent = Iterator(*me()).GetNextNode(parent);
					if (parent != NULL)
					{
						me()->AddKeys(parent, 0, keys);
						me()->UpdateBTreeKeys(parent);
					}
					else {
						MEMORIA_TRACE(me(), "RemovePages: no right sibling for", parent->id());
					}
				}
			}
		}
		else if (stop_idx - start_idx > 1)
		{
			MEMORIA_TRACE(me(), "RemovePages: remove page part");
			affected = me()->RemoveSpace(start, start_idx + 1, stop_idx - start_idx - 1, true, true, keys, preserve_key_values) || affected;

			if (start_idx >= 0 && stop_idx < children_count)
			{
				while (!start->is_leaf())
				{
					NodeBaseG child0 = me()->GetChild(start, start_idx, Allocator::UPDATE);
					NodeBaseG child1 = me()->GetChild(start, start_idx + 1, Allocator::UPDATE);

					if (me()->CanMerge(child0, child1))
					{
						Int child0_size = me()->GetChildrenCount(child0);
						me()->MergeNodes(child0, child1);

						if (start->is_root() && me()->GetChildrenCount(start) == 1 && me()->CanConvertToRoot(child0))
						{
							me()->Node2Root(child0);

							me()->CopyRootMetadata(start, child0);

							me()->set_root(child0->id());
							child0->parent_idx() = 0;

							me()->allocator().RemovePage(start->id());
						}

						start = child0;
						start_idx = child0_size - 1;
					}
					else
					{
						break;
					}
				}
			}

			if (MapType == MapTypes::Sum && preserve_key_values)
			{
				if (start_idx + 1 < me()->GetChildrenCount(start))
				{
					MEMORIA_TRACE(me(), "RemovePages: part within ranges", start_idx + 1, me()->GetChildrenCount(start));
					me()->AddKeys(start, start_idx + 1, keys);
					me()->UpdateBTreeKeys(start);
				}
				else {
					MEMORIA_TRACE(me(), "RemovePages: out of ranges", me()->GetChildrenCount(start));

					Iterator i(*me());
					start = i.GetNextNode(start);
					if (start != NULL)
					{
						me()->AddKeys(start, 0, keys);
						me()->UpdateBTreeKeys(start);
					}
					else {
						MEMORIA_TRACE(me(), "RemovePages: no right sibling for", start->id());
					}
				}
			}
		}
		else {
			MEMORIA_TRACE(me(), "RemovePages: Update page keys");
			if (MapType == MapTypes::Sum && preserve_key_values && start_idx + 1 < me()->GetChildrenCount(start))
			{
				MEMORIA_TRACE(me(), "RemovePages: part within ranges", start_idx + 1, me()->GetChildrenCount(start));
				me()->AddKeys(start, start_idx + 1, keys);
				me()->UpdateBTreeKeys(start);
			}
			else {
				MEMORIA_TRACE(me(), "RemovePages: Do not update page keys");
			}
		}

		MEMORIA_TRACE(me(), "RemovePages: done");

		//FIXME: check return status
		return true;
	}
	else
	{
		NodeBaseG start_parent  = me()->GetParent(start, Allocator::UPDATE);
		NodeBaseG stop_parent   = me()->GetParent(stop, Allocator::UPDATE);
		Int start_parent_idx = start->parent_idx();
		Int stop_parent_idx  = stop->parent_idx();

		bool affected = false;

		if (start_idx >= 0)
		{
			affected = me()->RemoveSpace(start, start_idx + 1, me()->GetChildrenCount(start) - start_idx - 1, true, true, keys, preserve_key_values);
		}
		else
		{
			start_parent_idx--;
		}

		if (stop_idx < me()->GetChildrenCount(stop))
		{
			affected = me()->RemoveSpace(stop, 0, stop_idx, true, true, keys, preserve_key_values) || affected;
		}
		else
		{
			stop_parent_idx++;
		}

		return me()->RemovePages(start_parent, start_parent_idx, stop_parent, stop_parent_idx, keys, preserve_key_values) || affected;
	}
}

M_PARAMS
bool M_TYPE::MergeWithSiblings(NodeBaseG& node)
{
	Iterator tmp(*me());

	bool merged = false;

	NodeBaseG next = tmp.GetNextNode(node);

	if (next != NULL)
	{
		if (!me()->MergeBTreeNodes(node, next))
		{
			NodeBaseG prev = tmp.GetPrevNode(node);
			if (prev != NULL)
			{
				merged = me()->MergeBTreeNodes(prev, node);
				node = prev;
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
			merged = me()->MergeBTreeNodes(prev, node);
			node = prev;
		}
		else {
			merged = false;
		}
	}

	if (!node->is_root())
	{
		NodeBaseG parent = me()->GetParent(node, Allocator::READ);
		if (parent->is_root() && me()->GetChildrenCount(parent) == 1 && me()->CanConvertToRoot(node))
		{
			me()->Node2Root(node);
			me()->allocator().RemovePage(parent->id());
			me()->set_root(node->id());
		}
	}
	else {
		while (me()->GetChildrenCount(node) == 1 && !node->is_leaf())
		{
			NodeBaseG child = me()->GetChild(node, 0, Allocator::UPDATE);
			if (me()->GetChildrenCount(child) == 1)
			{
				me()->Node2Root(child);
				me()->allocator().RemovePage(node->id());
				me()->set_root(child->id());
			}

			node = child;
		}
	}

	return merged;
}

/**
 * Remove a page from the btree. Do recursive removing if page's parent
 * has no more children.
 *
 * If after removing the parent is less than half filled than
 * merge it with siblings.
 */

M_PARAMS
void M_TYPE::RemovePage(NodeBaseG node)
{
	if (!node->is_root())
	{
		NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);

		//recursively remove parent if has only one child.

		Int size = me()->GetChildrenCount(parent);
		if (size == 1)
		{
			me()->RemovePage(parent);
		}
		else {
			//Remove element from parent ponting tho this node.
			//This node (it is a child there) will be removed automatically
			//and all parents chain will be updated.
			me()->RemoveSpace(parent, node->parent_idx(), 1, true, true);

			//if after removing parent is less than half filled than
			//merge it with it's siblings if possible
			if (me()->GetChildrenCount(parent) < me()->GetMaxCapacity(parent) / 2)
			{
				me()->MergeWithSiblings(parent);
			}
		}
	}
	else {
		me()->RemoveNode(node);
	}
}

/**
 * Remove key and data pointed by iterator 'iter' form the map.
 *
 */
M_PARAMS
bool M_TYPE::RemoveEntry(Iterator iter)
{
	if (iter.IsNotEmpty() || !iter.IsEnd())
	{
		NodeBaseG& node = iter.page();
		Int idx = iter.key_idx();

		//if leaf page has more than 1 key do regular remove
		if (me()->GetChildrenCount(node) > 1) {
			//remove 1 element rom the leaf, update parent and
			//do not try to remove children (it's a leaf)

			me()->RemoveSpace(node, idx, 1, true, false);

			//try merging this leaf with previous of following
			//leaf if filled by half of it's capacity.
			if (me()->GetChildrenCount(node) > me()->GetMaxCapacity(node) / 2) {
				me()->MergeWithSiblings(node);
			}
		}
		else {
			//otherwise remove the leaf page.
			me()->RemovePage(node);
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
bool M_TYPE::RemoveEntries(Iterator from, Iterator to)
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
		stop_idx    = me()->GetChildrenCount(to.page());
	}
	else
	{
		stop_idx    = to.key_idx();
	}


	Key keys[Indexes] = {0};

	from.key_idx()--;
	return me()->RemovePages(from.page(), from.key_idx(), to.page(), stop_idx, keys);
}

M_PARAMS
void M_TYPE::Drop() {
	NodeBaseG root = me()->GetRoot(Allocator::READ);

	if (root != NULL) {
		me()->RemovePage(root);
	}
}


#undef M_TYPE
#undef M_PARAMS


}

#endif
