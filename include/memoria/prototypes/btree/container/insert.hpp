
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
    typedef typename Base::Allocator                                            Allocator;

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

    static const Int Indexes                                                    = Base::Indexes;

    typedef typename Base::Metadata                                             Metadata;


    void create_new();
    NodeBaseG CreateNode(Short level, bool root, bool leaf);
    void InsertSpace(NodeBaseG node, Int from, Int count, bool increase_children_count = true);
    NodeBaseG SplitNode(NodeBaseG one, NodeBaseG parent, Int parent_idx, Int from, Int shift);


    /**
     * page is the old page
     * count_leaf - is a number of nodes to leave in the 'old' page (count_leaf == 'split_start')
     * shift is an empty 'window' (i.e. space to invert new keys/datas to) in the new leaf page
     *
     * returns new leaf page that goes right after old leaf page in the index tree
     */
    NodeBaseG SplitBTreeNode(NodeBaseG page, Int count_leaf, Int shift = 0);


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
        return Iterator(*me());
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::InsertName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::create_new()
{
	NodeBaseG node = me()->CreateNode(0, true, true);
	me()->set_root(node->id());
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::CreateNode(Short level, bool root, bool leaf)
{
	NodeBaseG node = NodeFactory::Create(me()->allocator(), level, root, leaf);

	if (root) {
		Metadata meta = me()->GetRootMetadata(node);
		meta.model_name() = me()->name();
		me()->SetRootMetadata(node, meta);
	}

	node->model_hash() = me()->hash();

	return node;
}

M_PARAMS
void M_TYPE::InsertSpace(NodeBaseG node, Int from, Int count, bool increase_children_count)
{
	Int total_children_count = MoveElements<NodeDispatcher>(node.page(), from, count, increase_children_count);

	if (!node->is_leaf())
	{
		MoveChildren<NonLeafDispatcher, TreeNodePage>(node.page(), from, count, total_children_count, me()->allocator());
	}
	else if (me()->IsDynarray())
	{
		MoveChildren<LeafDispatcher, TreeNodePage>(node.page(), from, count, total_children_count, me()->allocator());
	}
}


M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::SplitNode(NodeBaseG one, NodeBaseG parent, Int parent_idx, Int from, Int shift)
{
	NodeBaseG two = me()->CreateNode(one->level(), false, one->is_leaf()); // one->is_root() was false here

	Int count = CopyElements<NodeDispatcher>(one.page(), two.page(), from, shift);

	Counters counters;

	if (!two->is_leaf())
	{
		//FIXME: AccumulateChildrenCounters() does not only sums counters but makes children reparenting
		// maybe CopyElemnts knows this secret
		counters = AccumulateChildrenCounters<NonLeafDispatcher, Counters>(two.page(), from, shift, count, me()->allocator());

		UpdateChildrenParentIdx<NonLeafDispatcher, TreeNodePage>(two.page(), from, shift, count, me()->allocator());

		two->counters() += counters;
		one->counters() -= counters;
	}
	else
	{
		if (me()->IsDynarray())
		{
			ShiftAndReparentChildren<LeafDispatcher, TreeNodePage>(two.page(), from, shift, count, me()->allocator());
			UpdateChildrenParentIdx<LeafDispatcher, TreeNodePage>(two.page(), from, shift, count, me()->allocator());
		}

		counters.page_count()           = 0;
		two->counters().page_count()    = 1;

		Int one_children_count = me()->GetChildrenCount(one);
		counters.key_count() = one->counters().key_count() - one_children_count;
		one->counters().key_count() = one_children_count;

		two->counters().key_count() = me()->GetChildrenCount(two);
	}

	two->parent_idx() = parent_idx;
	two->parent_id()  = parent->id();

	me()->PostSplit(one, two, from);

	NodeBaseG one_parent = me()->GetParent(one, Allocator::UPDATE);

	Key max[Indexes];
	me()->GetMaxKeys(one, max);

	me()->UpdateBTreeKeys(one_parent, one->parent_idx(), max);
	me()->SetINodeData(parent, parent_idx, &two->id());

	me()->GetMaxKeys(two, max);
	me()->UpdateBTreeKeys(parent, parent_idx, max);

	if (!(one_parent->id() == parent->id()))
	{
		me()->UpdateBTreeCounters(one_parent, -counters);
		me()->UpdateBTreeCounters(parent, Counters(counters.page_count() + 1, counters.key_count()));
	}
	else {
		me()->UpdateBTreeCounters(parent, Counters(1, 0));
	}

	return two;
}



M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::SplitBTreeNode(NodeBaseG page, Int count_leaf, Int shift)
{
	if (!page->is_root())
	{
		NodeBaseG new_page(&me()->allocator());
		NodeBaseG parent = me()->GetParent(page, Allocator::UPDATE);

		Int idx_in_parent = page->parent_idx();
		if (me()->GetCapacity(parent) == 0)
		{
			MEMORIA_TRACE(me(), "Parent", parent->id(), "is full", idx_in_parent, me()->GetChildrenCount(parent));
			Int parent_idx;

			if (idx_in_parent < me()->GetChildrenCount(parent) - 1)
			{
				parent = me()->SplitBTreeNode(parent, idx_in_parent + 1, 1);
				parent_idx = 0;
			}
			else
			{
				parent = SplitBTreeNode(parent, me()->GetChildrenCount(parent) / 2, 0);
				parent_idx = me()->GetChildrenCount(parent);
				me()->InsertSpace(parent, parent_idx, 1);
			}

			new_page = me()->SplitNode(page, parent, parent_idx, count_leaf, shift);
		}
		else
		{
			MEMORIA_TRACE(me(), "Make room in parent", parent->id(), "at", idx_in_parent + 1);
			InsertSpace(parent, idx_in_parent + 1, 1);
			new_page = me()->SplitNode(page, parent, idx_in_parent + 1, count_leaf, shift);
		}

		return new_page;
	}
	else
	{
		NodeBaseG root 		= me()->GetRoot(Allocator::UPDATE); // page == root
		NodeBaseG new_root 	= me()->CreateNode(root->level() + 1, true, false);

		MEMORIA_TRACE(me(), "Split root", root->id(), "new root", new_root->id());

		me()->CopyRootMetadata(root, new_root);

		me()->Root2Node(root);

		new_root->parent_id() 	= root->parent_id();
		new_root->parent_idx() 	= 0;

		root->parent_id() 		= new_root->id();
		root->parent_idx() 		= 0;

		Key keys[Indexes];
		for (Int c = 0; c <Indexes; c++) keys[c] = 0;

		me()->GetMaxKeys(root, keys);
		me()->SetKeys(new_root, 0, keys);
		me()->SetINodeData(new_root, 0, &root->id());
		me()->SetChildrenCount(new_root, 2);

		new_root->counters() 	= root->counters();
		new_root->counters().page_count() += 1;

		me()->set_root(new_root->id());

		return me()->SplitNode(page, new_root, 1, count_leaf, shift);
	}
}

M_PARAMS
void M_TYPE::InsertEntry(Iterator &iter, const Key *keys, const Value &value)
{
	if (iter.IsNotEmpty())
	{
		NodeBaseG node = iter.page();
		Int idx = iter.key_idx();

		if (me()->GetCapacity(node) > 0)
		{
			InsertSpace(node, idx, 1);
		}
		else if (idx == 0)
		{
			SplitBTreeNode(node, me()->GetChildrenCount(node) / 2, 0);
			idx = 0;
			InsertSpace(node, idx, 1);
		}
		else if (idx < me()->GetChildrenCount(node))
		{
			SplitBTreeNode(node, idx, 0);
			InsertSpace(node, idx, 1);
		}
		else {
			node = SplitBTreeNode(node, me()->GetChildrenCount(node) / 2, 0);

			idx = me()->GetChildrenCount(node);
			InsertSpace(node, idx, 1);

			iter.page() = node;
			iter.key_idx() = idx;
		}

		me()->SetLeafDataAndReindex(node, idx, keys, value);

		if (idx >= me()->GetChildrenCount(node) - 1) {
			me()->UpdateBTreeKeys(node);
		}

		me()->UpdateBTreeCounters(node, Counters(0, 1));
	}
	else
	{
		NodeBaseG node = CreateNode(0, true, true);

		me()->set_root(node->id());
		iter.page() = node;
		iter.key_idx() = 0;

		InsertFn<Key, Value> fn(keys, &value);
		LeafDispatcher::Dispatch(node, fn);

		me()->UpdateBTreeCounters(node, Counters(0, 1));
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



#endif
