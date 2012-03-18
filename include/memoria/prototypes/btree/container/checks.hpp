
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
    typedef typename Types::NodeBaseG                                            NodeBaseG;
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
    	return me()->CheckTree();
    }

//PRIVATE API:
    void check_node_tree(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors);

    bool check_leaf_value(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& leaf, Int idx) {
        return false;
    }

    template <typename Node>
    bool CheckNodeContent(Node *node) {
        return false;
    }

    template <typename Node1, typename Node2>
    bool CheckNodeWithParentContent(Node1 *node, Node2 *parent, Int parent_idx);

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
        Int parent_idx_;

        CheckNodeContentFn2(MyType& map, Int parent_idx): map_(map), parent_idx_(parent_idx) {}

        template <typename Node1, typename Node2>
        void operator()(Node1* node, Node2* parent) {
            rtn_ = map_.CheckNodeWithParentContent(node, parent, parent_idx_);
        }
    };

    bool check_node_content(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node);

    bool check_keys() {
        return false;
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::ChecksName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::CheckTree()
{
	NodeBaseG root = me()->GetRoot(Allocator::READ);
	if (root != NULL)
	{
		bool errors = false;
		me()->check_node_tree(NodeBaseG(), 0, root, errors);
		return me()->check_keys() || errors;
	}
	else {
		return false;
	}
}






// ------------------------ PRIVATE API -----------------------------


M_PARAMS
void M_TYPE::check_node_tree(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors)
{
	errors = me()->check_node_content(parent, parent_idx, node) || errors;

	Int children = node->children_count();

	if (children == 0 && !node->is_root())
	{
		errors = true;
		MEMORIA_ERROR(me(), "children == 0 for non-root node", node->id());
	}

	if (node->is_leaf())
	{
		for (Int c = 0; c < children; c++)
		{
			errors = me()->check_leaf_value(parent, parent_idx, node, c) || errors;
		}
	}
	else {
		for (Int c = 0; c < children; c++)
		{
			ID child_id	= me()->GetINodeData(node, c);

			NodeBaseG child = me()->GetChild(node, c, Allocator::READ);

			if (child->id() != child_id)
			{
				errors = true;
				MEMORIA_ERROR(me(), "child.id != child_id", child->id(), child->id(), child_id);
			}

			me()->check_node_tree(node, c, child, errors);
		}
	}
}

M_PARAMS
template <typename Node1, typename Node2>
bool M_TYPE::CheckNodeWithParentContent(Node1 *node, Node2 *parent, Int parent_idx) {
	bool errors = false;
	for (Int c = 0; c < Indexes; c++)
	{
		if (node->map().max_key(c) != parent->map().key(c, parent_idx))
		{
			MEMORIA_TRACE(me(), "Invalid parent-child nodes chain", c, node->map().max_key(c), parent->map().key(c, parent_idx), "for node.id=", node->id(), "parent.id=", parent->id(), "parent_idx", parent_idx);
			errors = true;
		}
	}
	return errors;
}


M_PARAMS
bool M_TYPE::check_node_content(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node)
{
	bool errors = false;

	if (parent.is_set())
	{
		CheckNodeContentFn2 fn2(*me(), parent_idx);
		NodeDispatcher::DoubleDispatchConst(node, parent, fn2);
		errors = fn2.rtn_ || errors;
	}
	else {

		CheckNodeContentFn1 fn1(*me());
		RootDispatcher::DispatchConst(node, fn1);
		errors = fn1.rtn_ || errors;

	}

	return errors;
}

#undef M_TYPE
#undef M_PARAMS






}



#endif	/* _MEMORIA_PROTOTYPES_BTREE_MODEL_CHECKS_HPP */
