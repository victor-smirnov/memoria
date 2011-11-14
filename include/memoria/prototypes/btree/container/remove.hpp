
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


    typedef typename Base::NodeBase                                             NodeBase;
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

    
    void MoveChildrenLeft(NodeBase *node, Int from, Int count);



    
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

            if (MapType == MapTypes::Index)
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

                NodeBase *child = map_.GetChild(node, c);
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
    bool RemoveSpace(NodeBase *node, Int from, Int count, bool update, bool remove_children, Key* keys = NULL, bool preserve_key_values = true);

    void RemoveNode(NodeBase *node);

    bool CanMerge(NodeBase *page1, NodeBase *page2)
    {
        return me_.GetChildrenCount(page2) <= me_.GetCapacity(page1);
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

    void MergeNodes(NodeBase *page1, NodeBase *page2, bool fix_parent = true);


    bool MergeBTreeNodes(NodeBase *page1, NodeBase *page2);


    /**
     * For Index type BTree if we remove some entries we need to preserve absolute key values
     * from the right. Removed values will be returned in 'keys' output parameter.
     *
     * For structures like DynVector if we remove range we have to substruct the values from the right index cell manually.
     *
     * FIXME: optimize. Add a flag for absolute key value preserving.
     *
     */
    bool RemovePages(NodeBase* start, Int start_idx, NodeBase* stop, Int stop_idx, Key* keys, bool preserve_key_values = true);


    bool MergeWithSiblings(NodeBase *node);

    /**
     * Remove a page from the btree. Do recursive removing if page's parent
     * has no more children.
     *
     * If after removing the parent is less than half filled than
     * merge it with siblings.
     */

    void RemovePage(NodeBase *node);

    /**
     * Remove key and data pointed by iterator 'iter' form the map.
     *
     */
    bool RemoveEntry(Iterator iter) ;

    bool RemoveEntries(Iterator from, Iterator to);

    virtual void Drop(ContainerCollection* container);

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::RemoveName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::MoveChildrenLeft(NodeBase *node, Int from, Int count) {
    RemoveElements<NodeDispatcher>(node, from, count, true);

    if (!node->is_leaf())
    {
        IncrementChildPids<NonLeafDispatcher, TreeNodePage>(node, from, -count, me_.allocator());
    }
    else if (me_.IsDynarray()) {
    	IncrementChildPids<LeafDispatcher, TreeNodePage>(node, from, -count, me_.allocator());
    }
}

M_PARAMS
bool M_TYPE::RemoveSpace(NodeBase *node, Int from, Int count, bool update, bool remove_children, Key* keys, bool preserve_key_values)
{
	MEMORIA_TRACE(me_, "RemoveSpace", node->id(), from, count, update, remove_children);

	if (count  == 0) return false;

	Key keys0[Indexes] = {0};

	if (MapType == MapTypes::Index)
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

		me_.SumKeys(node, from, count, keys);

		if (upd0 && preserve_key_values)
		{
			Int size = me_.GetChildrenCount(node);
			if (from + count < size)
			{
				me_.AddKeys(node, from + count, keys);
			}
			else {
				Iterator tmp(me_);
				NodeBase* next = tmp.GetNextNode(node);
				if (next != NULL) {
					me_.AddKeys(next, 0, keys);
					me_.UpdateBTreeKeys(next);
				}
			}
		}
	}

	Counters counters;

	if (!node->is_leaf())
	{
		RemoveSpaceFn fn(from, count, remove_children, me_);
		NonLeafDispatcher::Dispatch(node, fn);
		counters = fn.counters();

		node->counters() -= counters;
	}
	else {
		node->counters().key_count() -= count;
		counters.key_count() = count;

		typename MyType::DataRemoveHandlerFn data_remove_handler_fn(from, count, me_);
		LeafDispatcher::Dispatch(node, data_remove_handler_fn);
	}

	me_.MoveChildrenLeft(node, from, count);

	if (update)
	{
		me_.UpdateBTreeKeys(node);

		NodeBase *parent = me_.GetParent(node);
		if (parent != NULL) {
			me_.UpdateBTreeCounters(parent, -counters);
		}
	}

	return true;
}

M_PARAMS
void M_TYPE::RemoveNode(NodeBase *node)
{
	const Int children_count = me_.GetChildrenCount(node);


	if (!node->is_leaf())
	{
		for (Int c = 0; c < children_count; c++)
		{
			NodeBase *child = me_.GetChild(node, c);
			me_.RemoveNode(child);
		}
	}
	else {
		typename MyType::DataRemoveHandlerFn data_remove_handler_fn(0, children_count, me_);
		LeafDispatcher::Dispatch(node, data_remove_handler_fn);
	}

	if (node->is_root())
	{
		ID id;
		id.set_null();
		me_.set_root(id);
	}

	me_.allocator().RemovePage(node->id());
}

//struct ReindexFn0 {
//    template <typename T>
//    void operator()(T *node) {
//        node->map().Reindex();
//    }
//};

M_PARAMS
void M_TYPE::MergeNodes(NodeBase *page1, NodeBase *page2, bool fix_parent)
{
	MEMORIA_TRACE(me_, "MergeNodes", page1->id(), page2->id(), fix_parent);
	//me_.PreMerge(page1, page2);

	MergeNodesFn fn;
	NodeDispatcher::Dispatch(page1, page2, fn);
	Int fn_start = fn.start();

	if (!page1->is_leaf())
	{
		NodeBase *page11 = page1;
		IncrementChildPidsAndReparent<NonLeafDispatcher, TreeNodePage>(page11, fn_start, fn_start, me_.allocator());
	}
	else if (me_.IsDynarray()) {
		NodeBase *page11 = page1;
		IncrementChildPidsAndReparent<LeafDispatcher, TreeNodePage>(page11, fn_start, fn_start, me_.allocator());
	}

	page1->counters() += page2->counters();
	page1->counters().page_count() -= 1;

	if (fix_parent)
	{
		NodeBase *parent = me_.GetParent(page1);
		Int parent_idx = page2->parent_idx();

		Key keys[Indexes];
		for (Int c = 0; c <Indexes; c++) keys[c] = 0;

		me_.GetKeys(parent, parent_idx, keys);
		if (MapType == MapTypes::Value)
		{
			me_.SetKeys(parent, parent_idx - 1, keys);
		}
		else {
			me_.AddKeys(parent, parent_idx - 1, keys, false);
		}

		MoveChildrenLeft(parent, parent_idx, 1);

		me_.UpdateBTreeCounters(parent, Counters(-1, 0));

		ReindexFn fn;
		NodeDispatcher::Dispatch(parent, fn);

		//Reindex<NodeDispatcher>(parent);
	}

	me_.allocator().RemovePage(page2->id());
}

M_PARAMS
bool M_TYPE::MergeBTreeNodes(NodeBase *page1, NodeBase *page2)
{
	if (CanMerge(page1, page2))
	{
		if (IsSameParent(page1, page2))
		{
			MEMORIA_TRACE(me_,"MergeBTReeNodes with the same parent");
			me_.MergeNodes(page1, page2);

			NodeBase *parent = me_.GetParent(page1);
			if (parent->is_root() && me_.GetChildrenCount(parent) == 1 && me_.CanConvertToRoot(page1))
			{
				page1 = me_.Node2Root(page1);

				me_.CopyRootMetadata(parent, page1);

				me_.set_root(page1->id());
				page1->parent_idx() = 0;

				me_.allocator().RemovePage(parent->id());
			}

			return true;
		}
		else
		{
			NodeBase *parent1 = me_.GetParent(page1);
			NodeBase *parent2 = me_.GetParent(page2);
			if (me_.MergeBTreeNodes(parent1, parent2))
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
bool M_TYPE::RemovePages(NodeBase* start, Int start_idx, NodeBase* stop, Int stop_idx, Key* keys, bool preserve_key_values)
{
	if (start == NULL || stop == NULL)
	{
		MEMORIA_TRACE(me_, "RemovePages", start, stop);
		return false;
	}

	MEMORIA_TRACE(me_, "RemovePages", start->id(), start_idx, stop->id(), stop_idx);

	if (start->id() == stop->id())
	{
		bool affected = false;

		Int children_count = me_.GetChildrenCount(start);

		if (start_idx == -1 && stop_idx == children_count)
		{
			MEMORIA_TRACE(me_, "RemovePages: full page");

			NodeBase* parent = NULL;
			Int parent_idx = start->parent_idx();

			while (!start->is_root())
			{
				parent      = me_.GetParent(start);
				parent_idx  = start->parent_idx();

				if (me_.GetChildrenCount(parent) > 1)
				{
					affected = me_.RemoveSpace(parent, parent_idx, 1, true, true, keys, preserve_key_values) || affected;
					break;
				}
				else
				{
					start = parent;
				}
			}

			if (start->is_root())
			{
				me_.RemoveNode(start);
				affected = true;
				me_.set_root(ID(0));
			}
			else if (MapType == MapTypes::Index && preserve_key_values && parent != NULL)
			{
				if (parent_idx < me_.GetChildrenCount(parent) - 1)
				{
					MEMORIA_TRACE(me_, "RemovePages: within ranges", parent_idx, me_.GetChildrenCount(parent));
					me_.AddKeys(parent, parent_idx + 1, keys);
					me_.UpdateBTreeKeys(parent);
				}
				else {
					MEMORIA_TRACE(me_, "RemovePages: out of ranges", parent_idx, me_.GetChildrenCount(parent));

					Iterator i(me_);
					parent = i.GetNextNode(parent);
					if (parent != NULL)
					{
						me_.AddKeys(parent, 0, keys);
						me_.UpdateBTreeKeys(parent);
					}
					else {
						MEMORIA_TRACE(me_, "RemovePages: no right sibling for", parent->id());
					}
				}
			}
		}
		else if (stop_idx - start_idx > 1)
		{
			MEMORIA_TRACE(me_, "RemovePages: remove page part");
			affected = me_.RemoveSpace(start, start_idx + 1, stop_idx - start_idx - 1, true, true, keys, preserve_key_values) || affected;

			if (start_idx >= 0 && stop_idx < children_count)
			{
				while (!start->is_leaf())
				{
					NodeBase* child0 = me_.GetChild(start, start_idx);
					NodeBase* child1 = me_.GetChild(start, start_idx + 1);

					if (me_.CanMerge(child0, child1))
					{
						Int child0_size = me_.GetChildrenCount(child0);
						me_.MergeNodes(child0, child1);

						if (start->is_root() && me_.GetChildrenCount(start) == 1 && me_.CanConvertToRoot(child0))
						{
							child0 = me_.Node2Root(child0);

							me_.CopyRootMetadata(start, child0);

							me_.set_root(child0->id());
							child0->parent_idx() = 0;

							me_.allocator().RemovePage(start->id());
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

			if (MapType == MapTypes::Index && preserve_key_values)
			{
				if (start_idx + 1 < me_.GetChildrenCount(start))
				{
					MEMORIA_TRACE(me_, "RemovePages: part within ranges", start_idx + 1, me_.GetChildrenCount(start));
					me_.AddKeys(start, start_idx + 1, keys);
					me_.UpdateBTreeKeys(start);
				}
				else {
					MEMORIA_TRACE(me_, "RemovePages: out of ranges", me_.GetChildrenCount(start));

					Iterator i(me_);
					start = i.GetNextNode(start);
					if (start != NULL)
					{
						me_.AddKeys(start, 0, keys);
						me_.UpdateBTreeKeys(start);
					}
					else {
						MEMORIA_TRACE(me_, "RemovePages: no right sibling for", start->id());
					}
				}
			}
		}
		else {
			MEMORIA_TRACE(me_, "RemovePages: Update page keys");
			if (MapType == MapTypes::Index && preserve_key_values && start_idx + 1 < me_.GetChildrenCount(start))
			{
				MEMORIA_TRACE(me_, "RemovePages: part within ranges", start_idx + 1, me_.GetChildrenCount(start));
				me_.AddKeys(start, start_idx + 1, keys);
				me_.UpdateBTreeKeys(start);
			}
			else {
				MEMORIA_TRACE(me_, "RemovePages: Do not update page keys");
			}
		}

		MEMORIA_TRACE(me_, "RemovePages: done");

		//FIXME: check return status
		return true;
	}
	else
	{
		NodeBase *start_parent  = me_.GetParent(start);
		NodeBase *stop_parent   = me_.GetParent(stop);
		Int start_parent_idx = start->parent_idx();
		Int stop_parent_idx  = stop->parent_idx();

		bool affected = false;

		if (start_idx >= 0)
		{
			affected = me_.RemoveSpace(start, start_idx + 1, me_.GetChildrenCount(start) - start_idx - 1, true, true, keys, preserve_key_values);
		}
		else
		{
			start_parent_idx--;
		}

		if (stop_idx < me_.GetChildrenCount(stop))
		{
			affected = me_.RemoveSpace(stop, 0, stop_idx, true, true, keys, preserve_key_values) || affected;
		}
		else
		{
			stop_parent_idx++;
		}

		return me_.RemovePages(start_parent, start_parent_idx, stop_parent, stop_parent_idx, keys, preserve_key_values) || affected;
	}
}

M_PARAMS
bool M_TYPE::MergeWithSiblings(NodeBase *node) {
	MEMORIA_TRACE(me_, "MergeWithSiblings", node->id());

	Iterator tmp(me_);

	bool merged = false;

	NodeBase *next = tmp.GetNextNode(node);

	if (next != NULL)
	{
		if (!me_.MergeBTreeNodes(node, next))
		{
			NodeBase *prev = tmp.GetPrevNode(node);
			if (prev != NULL)
			{
				merged = me_.MergeBTreeNodes(prev, node);
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
		NodeBase *prev = tmp.GetPrevNode(node);
		if (prev != NULL)
		{
			merged = me_.MergeBTreeNodes(prev, node);
			node = prev;
		}
		else {
			merged = false;
		}
	}

	if (!node->is_root())
	{
		NodeBase *parent = me_.GetParent(node);
		if (parent->is_root() && me_.GetChildrenCount(parent) == 1 && me_.CanConvertToRoot(node))
		{
			NodeBase *new_root = me_.Node2Root(node);
			me_.allocator().RemovePage(parent->id());
			me_.set_root(new_root->id());
		}
	}
	else {
		while (me_.GetChildrenCount(node) == 1 && !node->is_leaf()) {
			NodeBase *child = me_.GetChild(node, 0);
			if (me_.GetChildrenCount(child) == 1)
			{
				NodeBase *new_root = me_.Node2Root(child);
				me_.allocator().RemovePage(node->id());
				me_.set_root(new_root->id());
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
void M_TYPE::RemovePage(NodeBase *node)
{
	if (!node->is_root())
	{
		NodeBase *parent = me_.GetParent(node);

		//recursively remove parent if has only one child.

		Int size = me_.GetChildrenCount(parent);
		if (size == 1)
		{
			me_.RemovePage(parent);
		}
		else {
			//Remove element from parent ponting tho this node.
			//This node (it is a child there) will be removed automatically
			//and all parents chain will be updated.
			me_.RemoveSpace(parent, node->parent_idx(), 1, true, true);

			//if after removing parent is less than half filled than
			//merge it with it's siblings if possible
			if (me_.GetChildrenCount(parent) < me_.GetMaxCapacity(parent) / 2)
			{
				me_.MergeWithSiblings(parent);
			}
		}
	}
	else {
		me_.RemoveNode(node);
	}
}

/**
 * Remove key and data pointed by iterator 'iter' form the map.
 *
 */
M_PARAMS
bool M_TYPE::RemoveEntry(Iterator iter) {
	if (iter.IsNotEmpty() || !iter.IsEnd()) {
		NodeBase *node = iter.page();
		Int idx = iter.key_idx();

		//if leaf page has more than 1 key do regular remove
		if (me_.GetChildrenCount(node) > 1) {
			//remove 1 element rom the leaf, update parent and
			//do not try to remove children (it's a leaf)

			me_.RemoveSpace(node, idx, 1, true, false);

			//try merging this leaf with previous of following
			//leaf if filled by half of it's capacity.
			if (me_.GetChildrenCount(node) > me_.GetMaxCapacity(node) / 2) {
				me_.MergeWithSiblings(node);
			}
		}
		else {
			//otherwise remove the leaf page.
			me_.RemovePage(node);
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
bool M_TYPE::RemoveEntries(Iterator from, Iterator to) {
	if (from.IsEmpty() || from.IsEnd())
	{
		return false;
	}

	if (to.IsEmpty())
	{
		return false;
	}

	NodeBase *stop  = to.page();
	Int    stop_idx = to.key_idx();

	if (to.key_idx() == 0 && to.PrevLeaf())
	{
		stop        = to.page();
		stop_idx    = me_.GetChildrenCount(stop);
	}
	else
	{
		stop        = to.page();
		stop_idx    = to.key_idx();
	}

	Key keys[Indexes] = {0};

	from.key_idx()--;
	return me_.RemovePages(from.page(), from.key_idx(), stop, stop_idx, keys);
}

M_PARAMS
void M_TYPE::Drop(ContainerCollection* container) {
	if (container == NULL) throw NullPointerException(MEMORIA_SOURCE, "Container must not be null");

	NodeBase* root = me_.GetRoot();

	if (root != NULL) {
		me_.RemovePage(root);
	}
}


#undef M_TYPE
#undef M_PARAMS


}

#endif
