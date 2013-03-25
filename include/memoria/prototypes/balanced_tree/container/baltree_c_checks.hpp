
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_CHECKS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_CHECKS_HPP

#include <iostream>

#include <memoria/prototypes/balanced_tree/pages/pages.hpp>
#include <memoria/core/container/logs.hpp>
#include <memoria/core/container/macros.hpp>


namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::ChecksName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;


    static const Int Indexes                                                    = Types::Indexes;


//PROTECETED API:

//PUBLIC API:

    bool checkTree();

    bool check(void *data) {
        return me()->checkTree();
    }

//PRIVATE API:
    void check_node_tree(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors) const;

    bool check_leaf_value(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& leaf, Int idx) const {
        return false;
    }

    template <typename Node>
    bool checkNodeContent(const Node *node) const {
        return false;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(CheckNodeContentFn1, checkNodeContent, bool);

    template <typename Node1, typename Node2>
    bool checkNodeWithParentContent(const Node1 *node, const Node2 *parent, Int parent_idx) const;

    MEMORIA_CONST_FN_WRAPPER_RTN(CheckNodeContentFn2, checkNodeWithParentContent, bool);


    bool check_node_content(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node) const;

    bool check_keys() {
        return false;
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::ChecksName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::checkTree()
{
    NodeBaseG root = me()->getRoot(Allocator::READ);
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
void M_TYPE::check_node_tree(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors) const
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
            ID child_id = me()->getChildID(node, c);

            NodeBaseG child = me()->getChild(node, c, Allocator::READ);

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
bool M_TYPE::checkNodeWithParentContent(const Node1 *node, const Node2 *parent, Int parent_idx) const {
    bool errors = false;
    for (Int c = 0; c < Indexes; c++)
    {
        if (node->map().maxKey(c) != parent->map().key(c, parent_idx))
        {
            MEMORIA_TRACE(
                    me(),
                    "Invalid parent-child nodes chain",
                    c,
                    node->map().maxKey(c),
                    parent->map().key(c, parent_idx),
                    "for node.id=",
                    node->id(),
                    "parent.id=",
                    parent->id(),
                    "parent_idx",
                    parent_idx
            );

            errors = true;
        }
    }
    return errors;
}


M_PARAMS
bool M_TYPE::check_node_content(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node) const
{
    bool errors = false;

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
