
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_WALK_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_WALK_HPP

#include <iostream>

#include <memoria/core/container/logs.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/tools/config.hpp>


namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::WalkName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::NodeDispatcher                               RootDispatcher;
    typedef typename Types::Pages::TreeDispatcher                               TreeDispatcher;

    typedef typename Types::Accumulator                                         Accumulator;


    void walkTree(ContainerWalker* walker)
    {
    	auto& self = this->self();

    	NodeBaseG root = self.getRoot(Allocator::READ);

    	walker->beginCtr(
    	    			TypeNameFactory<typename Types::ContainerTypeName>::name().c_str(),
    	    			self.name(),
    	    			IDValue(root->id())
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
    			NodeBaseG child = self.allocator().getPage(id, Allocator::READ, self.master_name());

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


#endif
