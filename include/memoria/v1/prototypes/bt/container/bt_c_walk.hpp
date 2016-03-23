
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <iostream>

#include <memoria/v1/core/container/logs.hpp>
#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/tools/config.hpp>


namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::WalkName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;


    void walkTree(ContainerWalker* walker)
    {
        auto& self = this->self();

        NodeBaseG root = self.getRoot();

        walker->beginCtr(
                        TypeNameFactory<typename Types::ContainerTypeName>::name().c_str(),
                        self.name(),
                        root->id()
                );

        this->traverseTree(root, walker);

        walker->endCtr();
    }

    void beginNode(const NodeBaseG& node, ContainerWalker* walker)
    {
        if (node->is_root())
        {
            if (node->is_leaf())
            {
                walker->rootLeaf(node->parent_idx(), node.page());
            }
            else {
                walker->beginRoot(node->parent_idx(), node.page());
            }
        }
        else if (node->is_leaf())
        {
            walker->leaf(node->parent_idx(), node.page());
        }
        else {
            walker->beginNode(node->parent_idx(), node.page());
        }
    }

    void endNode(const NodeBaseG& node, ContainerWalker* walker)
    {
        if (node->is_root())
        {
            if (!node->is_leaf())
            {
                walker->endRoot();
            }
        }
        else if (!node->is_leaf())
        {
            walker->endNode();
        }
    }

private:

    void traverseTree(const NodeBaseG& node, ContainerWalker* walker)
    {
        auto& self = this->self();

        self.beginNode(node, walker);

        if (!node->is_leaf())
        {
            self.forAllIDs(node, 0, self.getNodeSize(node, 0), [&self, walker](const ID& id, Int idx)
            {
                NodeBaseG child = self.allocator().getPage(id, self.master_name());

                self.traverseTree(child, walker);
            });
        }

        self.endNode(node, walker);
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::WalkName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
