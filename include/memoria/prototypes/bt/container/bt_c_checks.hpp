
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_CHECKS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_CHECKS_HPP

#include <iostream>

#include <memoria/core/container/logs.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>


namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::ChecksName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;
    using TreeDispatcher    = typename Types::Pages::TreeDispatcher;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;


//PROTECETED API:

//PUBLIC API:

    bool checkTree() const;

    bool check(void *data) const
    {
        return self().checkTree();
    }

    void checkIt()
    {
        auto& self = this->self();

        self.logger().level() = Logger::ERROR;

        if (self.checkTree())
        {
            throw Exception(MA_SRC, SBuf()<<"Container "<<self.name()<<" ("<<self.type_name_str()<<") check failed");
        }
    }

    MEMORIA_DECLARE_NODE_FN(CheckContentFn, check);
    bool checkContent(const NodeBaseG& node) const
    {
        try {
            NodeDispatcher::dispatch(node, CheckContentFn());
            return false;
        }
        catch (Exception ex)
        {
            self().dump(node);

            MEMORIA_ERROR(self(), "Node content check failed", ex.message(), ex.source());
            return true;
        }
    }



private:
    void checkTreeStructure(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors) const;


    template <typename Node1, typename Node2>
    bool checkTypedNodeContent(const Node1 *node, const Node2 *parent, Int parent_idx) const;

    MEMORIA_CONST_FN_WRAPPER_RTN(CheckTypedNodeContentFn, checkTypedNodeContent, bool);

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::ChecksName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::checkTree() const
{
    auto& self = this->self();

    NodeBaseG root = self.getRoot();
    if (root)
    {
        bool errors = false;
        self.checkTreeStructure(NodeBaseG(), 0, root, errors);
        return errors;
    }
    else {
        MEMORIA_ERROR(self, "No root node for container");
        return true;
    }
}






// ------------------------ PRIVATE API -----------------------------


M_PARAMS
void M_TYPE::checkTreeStructure(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& node, bool &errors) const
{
    auto& self = this->self();

    errors = self.checkContent(node) || errors;

    if (!node->is_root())
    {
        errors = TreeDispatcher::dispatchTree(parent, node, CheckTypedNodeContentFn(self), parent_idx) || errors;

        if (!node->is_leaf())
        {
            Int children = self.getNodeSize(node, 0);

            if (children == 0 && !node->is_root())
            {
                errors = true;
                MEMORIA_ERROR(self, "children == 0 for non-root node", node->id());
                self.dump(node);
            }
        }
    }

    if (!node->is_leaf())
    {
        Int children = self.getNodeSize(node, 0);

        for (Int c = 0; c < children; c++)
        {
            ID child_id = self.getChildID(node, c);

            NodeBaseG child = self.getChild(node, c);

            if (child->id() != child_id)
            {
                errors = true;
                MEMORIA_ERROR(self, "child.id != child_id", child->id(), child->id(), child_id);
            }

            if (child->parent_idx() != c)
            {
                errors = true;
                MEMORIA_ERROR(self, "child.parent_idx != idx", child->parent_idx(), c, node->id(), child->id());
                cout<<"parent_idx: "<<child->parent_idx()<<" "<<c<<endl;
            }

            if (child->parent_id() != node->id())
            {
                errors = true;
                MEMORIA_ERROR(self, "child.parent_id != node.id", child->parent_id(), node->id());
                cout<<"parent_idx: "<<child->parent_id()<<" "<<node->id()<<endl;
            }

            self.checkTreeStructure(node, c, child, errors);
        }
    }
}

M_PARAMS
template <typename Node1, typename Node2>
bool M_TYPE::checkTypedNodeContent(const Node1 *parent, const Node2* node, Int parent_idx) const
{
    bool errors = false;

    BranchNodeEntry sums;
    node->max(sums);

    BranchNodeEntry keys = parent->keysAt(parent_idx);

    if (sums != keys)
    {
        MEMORIA_ERROR(
                self(),
                "Invalid parent-child nodes chain",
                (SBuf()<<sums).str(),
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

#undef M_TYPE
#undef M_PARAMS

}


#endif
