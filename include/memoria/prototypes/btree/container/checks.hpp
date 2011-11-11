
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_CHECKS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_CHECKS_HPP

#include <iostream>

#include <memoria/prototypes/btree/pages/pages.hpp>
#include <memoria/vapi/models/logs.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::ChecksName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                              Allocator;

    typedef typename Allocator::Page                                              Page;
    typedef typename Page::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::Counters                                            Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
    typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    static const Int Indexes                                                    = Types::Indexes;


//PROTECETED API:

//PUBLIC API:

    bool CheckTree();

    bool Check(void *data) {
    	return me_.CheckTree();
    }

//PRIVATE API:
    void check_node_tree(NodeBase* node, bool &errors);

    bool check_leaf_value(NodeBase* leaf, Int idx) {
        return false;
    }

    template <typename Node>
    bool CheckNodeContent(Node *node) {
        return false;
    }

    template <typename Node1, typename Node2>
    bool CheckNodeWithParentContent(Node1 *node, Node2 *parent);

    struct CheckNodeContentFn1 {
        MyType& map_;
        bool rtn_;
        CheckNodeContentFn1(MyType& map): map_(map) {}

        template <typename T>
        void operator()(T* node) {
            rtn_ = map_.CheckNodeContent(node);
        }
    };

    struct CheckNodeContentFn2 {
        MyType& map_;
        bool rtn_;

        CheckNodeContentFn2(MyType& map): map_(map) {}

        template <typename Node1, typename Node2>
        void operator()(Node1* node, Node2* parent) {
            rtn_ = map_.CheckNodeWithParentContent(node, parent);
        }
    };

    bool check_node_content(NodeBase* node);

    bool check_keys() {
        return false;
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::ChecksName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::CheckTree()
{
	NodeBase* root = me_.GetRoot();
	if (root != NULL)
	{
		bool errors = false;
		me_.check_node_tree(root, errors);
		return me_.check_keys() || errors;
	}
	else {
		return false;
	}
}






// ------------------------ PRIVATE API -----------------------------


M_PARAMS
void M_TYPE::check_node_tree(NodeBase* node, bool &errors)
{
	errors = me_.check_node_content(node) || errors;
	Int children = me_.GetChildrenCount(node);

	if (node->is_leaf())
	{
		if (node->counters().page_count() != 1) {
			errors = true;
			MEMORIA_ERROR(me_, "counters.page_count != 1 for leaf node", node->id(), node->counters().page_count());
		}

		for (Int c = 0; c < children; c++)
		{
			errors = me_.check_leaf_value(node, c) || errors;
		}
	}
	else {
		Counters cnt;
		for (Int c = 0; c < children; c++)
		{
			NodeBase* child = me_.GetChild(node, c);
			cnt += child->counters();

			me_.check_node_tree(child, errors);

			if (!(child->parent_id() == node->id()))
			{
				errors = true;
				MEMORIA_ERROR(me_, "child.parent_id != parent.id", child->id(), child->parent_id(), node->id());
			}

			if (child->parent_idx() != c)
			{
				errors = true;
				MEMORIA_ERROR(me_, "child.parent_idx != parent.child.idx", child->id(), child->parent_idx(), c);
			}
		}

		cnt.page_count()++;

		//FIXME: check counters
		if (cnt != node->counters()) {
			//errors = true;
			//MEMORIA_ERROR(me_, "node.counters doesn't match childrens.counters", node->id(), node->counters(), cnt);
		}
	}
}

M_PARAMS
template <typename Node1, typename Node2>
bool M_TYPE::CheckNodeWithParentContent(Node1 *node, Node2 *parent) {
	bool errors = false;
	for (Int c = 0; c < Indexes; c++)
	{
		if (node->map().max_key(c) != parent->map().key(c, node->parent_idx()))
		{
			MEMORIA_TRACE(me_, "Invalid parent-child nodes chain", c, node->map().max_key(c), parent->map().key(c, node->parent_idx()), "for node.id=", node->id(), "parent.id=", parent->id(), "node.parent_idx",node->parent_idx());
			errors = true;
		}
	}
	return errors;
}

M_PARAMS
bool M_TYPE::check_node_content(NodeBase* node)
{
	bool errors = false;
	if (node->is_root()) {
		if (!node->parent_id().is_null()) {
			errors = true;
			MEMORIA_ERROR(me_, "node.parent_id != NULL for root node", node->id(), node->parent_id());
		}

		if (node->parent_idx() != 0) {
			errors = true;
			MEMORIA_ERROR(me_, "node.parent_id != 0 for root node", node->id(), node->parent_idx());
		}

		CheckNodeContentFn1 fn1(me_);
		RootDispatcher::Dispatch(node, fn1);
		errors = fn1.rtn_ || errors;
	}
	else {
		if (node->parent_id().is_null()) {
			errors = true;
			MEMORIA_ERROR(me_, "node.parent_id == NULL for root node", node->id(), node->parent_id());
		}

		NodeBase* parent = me_.GetParent(node);
		if (parent == NULL) {
			MEMORIA_ERROR(me_, "node", node->id(), "has no parent");
		}
		else {
			CheckNodeContentFn2 fn2(me_);
			NodeDispatcher::DoubleDispatch(node, parent, fn2);
			errors = fn2.rtn_ || errors;
		}
	}

	return errors;
}

#undef M_TYPE
#undef M_PARAMS






}



#endif	/* _MEMORIA_PROTOTYPES_BTREE_MODEL_CHECKS_HPP */
