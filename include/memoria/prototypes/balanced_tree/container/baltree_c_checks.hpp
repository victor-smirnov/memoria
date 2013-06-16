
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_CHECKS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_CHECKS_HPP

#include <iostream>

#include <memoria/core/container/logs.hpp>
#include <memoria/core/container/macros.hpp>


namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::ChecksName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;

    typedef typename Types::Accumulator                               			Accumulator;

    static const Int Indexes                                                    = Types::Indexes;


//PROTECETED API:

//PUBLIC API:

    bool checkTree() const;

    bool check(void *data) const
    {
        return self().checkTree();
    }

//PRIVATE API:
    void check_node_tree(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors) const;

    bool check_leaf_value(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& leaf, Int idx) const {
        return false;
    }

    template <typename Node>
    bool checkNodeContent(const Node *node) const
    {
        return false;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(CheckNodeContentFn1, checkNodeContent, bool);

    template <typename Node1, typename Node2>
    bool checkNodeWithParentContent(const Node1 *node, const Node2 *parent, Int parent_idx) const;

    MEMORIA_CONST_FN_WRAPPER_RTN(CheckNodeContentFn2, checkNodeWithParentContent, bool);


    bool check_node_content(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node) const;

    bool check_keys() const {
        return false;
    }

    MEMORIA_DECLARE_NODE_FN(CheckContentFn, check);

    bool checkContent(const NodeBaseG& node) const
    {
    	try {
    		NodeDispatcher::dispatchConst(node, CheckContentFn());
    		return false;
    	}
    	catch (Exception ex)
    	{
    		self().dump(node);

    		MEMORIA_ERROR(me(), ex.message());
    		return true;
    	}
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::ChecksName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::checkTree() const
{
    auto& self = this->self();

	NodeBaseG root = self.getRoot(Allocator::READ);
    if (root)
    {
        bool errors = false;
        self.check_node_tree(NodeBaseG(), 0, root, errors);
        return self.check_keys() || errors;
    }
    else {
        return false;
    }
}






// ------------------------ PRIVATE API -----------------------------


M_PARAMS
void M_TYPE::check_node_tree(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors) const
{
	auto& self = this->self();

    errors = self.check_node_content(parent, parent_idx, node) || errors;

    Int children = self.getNodeSize(node, 0);

    if (children == 0 && !node->is_root())
    {
        errors = true;
        MEMORIA_ERROR(me(), "children == 0 for non-root node", node->id());
    }

    if (node->is_leaf())
    {
        for (Int c = 0; c < children; c++)
        {
            errors = self.check_leaf_value(parent, parent_idx, node, c) || errors;
        }
    }
    else {
        for (Int c = 0; c < children; c++)
        {
            ID child_id = self.getChildID(node, c);

            NodeBaseG child = self.getChild(node, c, Allocator::READ);

            if (child->id() != child_id)
            {
                errors = true;
                MEMORIA_ERROR(me(), "child.id != child_id", child->id(), child->id(), child_id);
            }

            if (child->parent_idx() != c)
            {
            	errors = true;
            	MEMORIA_ERROR(me(), "child.parent_idx != idx", child->parent_idx(), c, node->id(), child->id());
            	cout<<"parent_idx: "<<child->parent_idx()<<" "<<c<<endl;
            }

            if (child->parent_id() != node->id())
            {
            	errors = true;
            	MEMORIA_ERROR(me(), "child.parent_id != node.id", child->parent_id(), node->id());
            	cout<<"parent_idx: "<<child->parent_id()<<" "<<node->id()<<endl;
            }

            self.check_node_tree(node, c, child, errors);
        }
    }
}

M_PARAMS
template <typename Node1, typename Node2>
bool M_TYPE::checkNodeWithParentContent(const Node1 *node, const Node2 *parent, Int parent_idx) const
{
    bool errors = false;
    Accumulator max = node->maxKeys();
    Accumulator keys = parent->keysAt(parent_idx);

    if (max != keys)
    {
    	MEMORIA_ERROR(
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
        bool result = NodeDispatcher::doubleDispatchConstRtn(node, parent, CheckNodeContentFn2(me()), parent_idx);
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



#endif  /* _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_CHECKS_HPP */
