
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_vctr_C_CHECKS_HPP
#define _MEMORIA_CONTAINER_vctr_C_CHECKS_HPP


#include <memoria/containers/vector/vctr_names.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {



MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector::CtrChecksName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Base::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
	typedef typename Types::Pages::NodeDispatcher                               RootDispatcher;
	typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
	typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
	typedef typename Types::Pages::TreeDispatcher                               TreeDispatcher;


	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;
	typedef typename Base::Element                                              Element;

	typedef typename Base::Metadata                                             Metadata;

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position 											Position;

	typedef typename Base::TreePath                                             TreePath;
	typedef typename Base::TreePathItem                                         TreePathItem;

	static const Int Indexes                                                    = Types::Indexes;
	static const Int Streams                                                    = Types::Streams;


    bool check_node_content(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node) const;
    void check_node_tree(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors) const;

    bool check_leaf_node(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& leaf) const;

    bool check_keys() const {return false;}


    template <typename Node>
    bool checkNodeContent(const Node *node) const {
        return false;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(CheckNodeContentFn1, checkNodeContent, bool);
//
//    template <typename Node1, typename Node2>
//    bool checkNodeWithParentContent(const Node1 *node, const Node2 *parent, Int parent_idx) const {
//
//    }

    template <typename NodeTypes, bool leaf, typename Node2>
    bool checkNodeWithParentContent1(
    		const Node2 *parent,
    		const TreeNode<BranchNode, NodeTypes, leaf> *node,
    		Int parent_idx
    ) const;


    template <typename NodeTypes, bool leaf, typename Node2>
    bool checkNodeWithParentContent1(
    		const Node2 *parent,
    		const TreeNode<LeafNode, NodeTypes, leaf> *node,
    		Int parent_idx
    ) const;

    MEMORIA_CONST_FN_WRAPPER_RTN(CheckNodeContentFn2, checkNodeWithParentContent1, bool);


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector::CtrChecksName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



// ------------------------ PRIVATE API -----------------------------


M_PARAMS
void M_TYPE::check_node_tree(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors) const
{
	auto& self = this->self();

    errors = self.check_node_content(parent, parent_idx, node) || errors;

    if (node->is_leaf())
    {
        errors = self.check_leaf_node(parent, parent_idx, node) || errors;
    }
    else {
    	Int children = self.getNodeSize(node, 0);

    	if (children == 0 && !node->is_root())
    	{
    		errors = true;
    		MEMORIA_ERROR(me(), "children == 0 for non-root node", node->id());
    	}

        for (Int c = 0; c < children; c++)
        {
            ID child_id = self.getChildID(node, c);

            NodeBaseG child = self.getChild(node, c, Allocator::READ);

            if (child->id() != child_id)
            {
                errors = true;
                MEMORIA_ERROR(me(), "child.id != child_id", child->id(), child_id, node->id());
            }

            if (child->parent_idx() != c)
            {
            	errors = true;
            	MEMORIA_ERROR(me(), "child.parent_idx != idx", child->parent_idx(), c, child->id(), node->id());
            }

            if (child->parent_id() != node->id())
            {
            	errors = true;
            	MEMORIA_ERROR(me(), "child.parent_id != node.id", child->parent_id(), node->id(), child->id());
            }

            self.check_node_tree(node, c, child, errors);
        }
    }
}

M_PARAMS
bool M_TYPE::check_leaf_node(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& leaf) const
{
	return false;
}

M_PARAMS
template <typename NodeTypes, bool leaf, typename Node2>
bool M_TYPE::checkNodeWithParentContent1(
		const Node2 *parent,
		const TreeNode<BranchNode, NodeTypes, leaf> *node,
		Int parent_idx
) const
{
    bool errors = false;
    Accumulator max = node->maxKeys();
    Accumulator keys = parent->keysAt(parent_idx);

    if (max != keys)
    {
    	MEMORIA_TRACE(
    			me(),
    			"Invalid parent-child nodes chain",
    			(SBuf()<<max).str(),
    			(SBuf()<<keys).str(),
    			"for node.id=",
    			node->id(),
    			"parent.id=",
    			parent->id(),
    			"parent_idx",
    			parent_idx
    	);

    	errors = true;
    }
    return errors;
}

M_PARAMS
template <typename NodeTypes, bool leaf, typename Node2>
bool M_TYPE::checkNodeWithParentContent1(
		const Node2 *parent,
		const TreeNode<LeafNode, NodeTypes, leaf> *node,
		Int parent_idx
) const
{
    bool errors = false;
    Accumulator max = node->maxKeys();
    Accumulator keys = parent->keysAt(parent_idx);

    if (max != keys)
    {
    	MEMORIA_TRACE(
    			me(),
    			"Invalid parent-child nodes chain",
    			(SBuf()<<max).str(),
    			(SBuf()<<keys).str(),
    			"for node.id=",
    			node->id(),
    			"parent.id=",
    			parent->id(),
    			"parent_idx",
    			parent_idx
    	);

    	errors = true;
    }
    return errors;
}



M_PARAMS
bool M_TYPE::check_node_content(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node) const
{
	bool errors = self().checkContent(node);

    if (parent.isSet())
    {
        bool result = TreeDispatcher::dispatchTreeConstRtn(parent, node, CheckNodeContentFn2(me()), parent_idx);
        errors = result || errors;
    }
    else {
        bool result = RootDispatcher::dispatchConstRtn(node, CheckNodeContentFn1(me()));
        errors = result || errors;
    }

    return errors;
}

#undef M_TYPE
#undef M_PARAMS



}


#endif
