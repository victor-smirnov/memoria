
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_INSERT_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_INSERT_HPP

#include <memoria/vapi/models/logs.hpp>
#include <memoria/prototypes/btree/pages/tools.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::InsertName)

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

    static const Int Indexes                                                    = Base::Indexes;

    typedef typename Base::Metadata                                             Metadata;


    void create_new();
    NodeBase *CreateNode(Short level, bool root, bool leaf);
    void InsertSpace(NodeBase *node, Int from, Int count, bool increase_children_count = true);
    NodeBase* SplitNode(NodeBase *one, NodeBase *parent, Int parent_idx, Int from, Int shift);


    /**
     * page is the old page
     * count_leaf - is a number of nodes to leave in the 'old' page (count_leaf == 'split_start')
     * shift is an empty 'window' (i.e. space to invert new keys/datas to) in the new leaf page
     *
     * returns new leaf page that is right after old leaf page in the index tree
     */
    NodeBase* SplitBTreeNode(NodeBase *page, Int count_leaf, Int shift = 0);


    template <typename Keys, typename Data>
    class InsertFn {
        const Keys *keys_;
        const Data *data_;
    public:
        InsertFn(const Keys *keys, const Data *data): keys_(keys), data_(data) {}

        template <typename Node>
        void operator()(Node *node) {
            node->map().size() = 1;

            for (Int c = 0; c < Node::INDEXES; c++) {
                node->map().key(c, 0) = keys_[c];
            }

            node->map().data(0) = *data_;
            node->map().Reindex();
        }
    };


    void InsertEntry(Iterator &iter, const Key *keys, const Value &value);
    void InsertEntry(Iterator &iter, Key key, const Value &value);

    Iterator EmptyIterator() {
        return Iterator(me_);
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::InsertName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::create_new() {
	MEMORIA_TRACE(me_, "BEGIN");
	NodeBase *node = me_.CreateNode(0, true, true);
	MEMORIA_TRACE(me_, "Set Root", node->id());
	me_.set_root(node->id());
}

M_PARAMS
typename M_TYPE::NodeBase *M_TYPE::CreateNode(Short level, bool root, bool leaf)
{
	MEMORIA_TRACE(me_, "CreateNode:", level, root, leaf, me_.name());

	NodeBase* node = NodeFactory::Create(me_.allocator(), level, root, leaf);

	if (root) {
		Metadata meta = me_.GetRootMetadata(node);
		meta.model_name() = me_.name();
		me_.SetRootMetadata(node, meta);
	}

	node->model_hash() = me_.hash();

	MEMORIA_TRACE(me_, "Node ID:", node, node->id(), node->id().value());

	return node;
}

M_PARAMS
void M_TYPE::InsertSpace(NodeBase *node, Int from, Int count, bool increase_children_count)
{
	Int total_children_count = MoveElements<NodeDispatcher>(node, from, count, increase_children_count);

	if (!node->is_leaf())
	{
		MoveChildren<NonLeafDispatcher, TreeNodePage>(node, from, count, total_children_count, me_.allocator());
	}
	else if (me_.IsDynarray())
	{
		MoveChildren<LeafDispatcher, TreeNodePage>(node, from, count, total_children_count, me_.allocator());
	}
}


M_PARAMS
typename M_TYPE::NodeBase* M_TYPE::SplitNode(NodeBase *one, NodeBase *parent, Int parent_idx, Int from, Int shift)
{
	MEMORIA_TRACE(me_, "SplitNode", one->id(), one->parent_idx(), "parent", parent->id(), parent_idx, "at", from, "shift", shift);

	NodeBase* two = me_.CreateNode(one->level(), false, one->is_leaf()); // one->is_root() was false here

	Int count = CopyElements<NodeDispatcher>(one, two, from, shift);

	Counters counters;

	if (!two->is_leaf())
	{
		//FIXME: AccumulateChildrenCounters() does not only sums counters but makes children reparenting
		// maybe CopyElemnts knows this secret
		counters = AccumulateChildrenCounters<NonLeafDispatcher, Counters>(two, from, shift, count, me_.allocator());

		UpdateChildrenParentIdx<NonLeafDispatcher, TreeNodePage>(two, from, shift, count, me_.allocator());

		two->counters() += counters;
		one->counters() -= counters;
	}
	else
	{
		if (me_.IsDynarray())
		{
			ShiftAndReparentChildren<LeafDispatcher, TreeNodePage>(two, from, shift, count, me_.allocator());
			UpdateChildrenParentIdx<LeafDispatcher, TreeNodePage>(two, from, shift, count, me_.allocator());
		}

		counters.page_count()           = 0;
		two->counters().page_count()    = 1;

		Int one_children_count = me_.GetChildrenCount(one);
		counters.key_count() = one->counters().key_count() - one_children_count;
		one->counters().key_count() = one_children_count;

		two->counters().key_count() = me_.GetChildrenCount(two);
	}

	two->parent_idx() = parent_idx;
	two->parent_id()  = parent->id();

	me_.PostSplit(one, two, from);

	NodeBase *one_parent = me_.GetParent(one);

	Key max[Indexes];
	me_.GetMaxKeys(one, max);

	me_.UpdateBTreeKeys(one_parent, one->parent_idx(), max);
	me_.SetINodeData(parent, parent_idx, &two->id());

	me_.GetMaxKeys(two, max);
	me_.UpdateBTreeKeys(parent, parent_idx, max);

	if (!(one_parent->id() == parent->id()))
	{
		me_.UpdateBTreeCounters(one_parent, -counters);
		me_.UpdateBTreeCounters(parent, Counters(counters.page_count() + 1, counters.key_count()));
	}
	else {
		me_.UpdateBTreeCounters(parent, Counters(1, 0));
	}

	return two;
}



M_PARAMS
typename M_TYPE::NodeBase* M_TYPE::SplitBTreeNode(NodeBase *page, Int count_leaf, Int shift)
{
	if (!page->is_root())
	{
		NodeBase *new_page;
		NodeBase *parent = me_.GetParent(page);

		Int idx_in_parent = page->parent_idx();
		if (me_.GetCapacity(parent) == 0)
		{
			MEMORIA_TRACE(me_, "Parent", parent->id(), "is full", idx_in_parent, me_.GetChildrenCount(parent));
			Int parent_idx;

			if (idx_in_parent < me_.GetChildrenCount(parent) - 1)
			{
				NodeBase* tmp = me_.SplitBTreeNode(parent, idx_in_parent + 1, 1);
				parent = tmp;
				parent_idx = 0;
			}
			else
			{
				parent = SplitBTreeNode(parent, me_.GetChildrenCount(parent) / 2, 0);
				parent_idx = me_.GetChildrenCount(parent);
				me_.InsertSpace(parent, parent_idx, 1);
			}

			new_page = me_.SplitNode(page, parent, parent_idx, count_leaf, shift);
		}
		else
		{
			MEMORIA_TRACE(me_, "Make room in parent", parent->id(), "at", idx_in_parent + 1);
			InsertSpace(parent, idx_in_parent + 1, 1);
			new_page = me_.SplitNode(page, parent, idx_in_parent + 1, count_leaf, shift);
		}

		return new_page;
	}
	else
	{
		NodeBase* root 		= me_.GetRoot(); // page == root
		NodeBase* new_root 	= me_.CreateNode(root->level() + 1, true, false);

		MEMORIA_TRACE(me_, "Split root", root->id(), "new root", new_root->id());

		me_.CopyRootMetadata(root, new_root);

		root = me_.Root2Node(root);

		new_root->parent_id() 	= root->parent_id();
		new_root->parent_idx() 	= 0;

		root->parent_id() 		= new_root->id();
		root->parent_idx() 		= 0;

		Key keys[Indexes];
		for (Int c = 0; c <Indexes; c++) keys[c] = 0;

		me_.GetMaxKeys(root, keys);
		me_.SetKeys(new_root, 0, keys);
		me_.SetINodeData(new_root, 0, &root->id());
		me_.SetChildrenCount(new_root, 2);

		new_root->counters() 	= root->counters();
		new_root->counters().page_count() += 1;

		me_.set_root(new_root->id());

		return me_.SplitNode(page, new_root, 1, count_leaf, shift);
	}
}

M_PARAMS
void M_TYPE::InsertEntry(Iterator &iter, const Key *keys, const Value &value)
{
	MEMORIA_TRACE(me_, "InsertEntry", iter.page(), iter.key_idx(), iter.IsEnd(), iter.IsEmpty(), keys[0]);

	if (iter.IsNotEmpty())
	{
		NodeBase *node = iter.page();
		Int idx = iter.key_idx();
		MEMORIA_TRACE(me_, "Insert value. idx =", idx);

		if (me_.GetCapacity(node) > 0)
		{
			MEMORIA_TRACE(me_, "Node", node, "has enough capacity");
			InsertSpace(node, idx, 1);
		}
		else if (idx == 0)
		{
			SplitBTreeNode(node, me_.GetChildrenCount(node) / 2, 0);
			idx = 0;
			InsertSpace(node, idx, 1);
		}
		else if (idx < me_.GetChildrenCount(node))
		{
//			NodeBase *new_node =
			SplitBTreeNode(node, idx, 0);
			InsertSpace(node, idx, 1);
		}
		else {
			node = SplitBTreeNode(node, me_.GetChildrenCount(node) / 2, 0);

			idx = me_.GetChildrenCount(node);
			InsertSpace(node, idx, 1);

			iter.page() = node;
			iter.key_idx() = idx;
		}

		me_.SetLeafDataAndReindex(node, idx, keys, value);

		if (idx >= me_.GetChildrenCount(node) - 1) {
			me_.UpdateBTreeKeys(node);
		}

		me_.UpdateBTreeCounters(node, Counters(0, 1));
	}
	else
	{
		NodeBase *node = CreateNode(0, true, true);

		me_.set_root(node->id());
		iter.page() = node;
		iter.key_idx() = 0;

		InsertFn<Key, Value> fn(keys, &value);
		LeafDispatcher::Dispatch(node, fn);

		me_.UpdateBTreeCounters(node, Counters(0, 1));
	}

	iter.reset_state();
}

M_PARAMS
void M_TYPE::InsertEntry(Iterator &iter, Key key, const Value &value) {
	Key keys[Indexes];

	for (Int c = 0; c < Indexes; c++)
	{
		keys[c] = key;
	}

	InsertEntry(iter, keys, value);
}




#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif	/* _MEMORIA_PROTOTYPES_BTREE_MODEL_INSERT_HPP */
