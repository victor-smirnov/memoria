
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQDENSE_SEQ_C_TOOLS_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_SEQ_C_TOOLS_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/containers/seq_dense/seqdense_names.hpp>


namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrToolsName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Base::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::TreeNodePage                                         TreeNodePage;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Base::NodeDispatcher                                       NodeDispatcher;
	typedef typename Base::RootDispatcher                                       RootDispatcher;
	typedef typename Base::LeafDispatcher                                       LeafDispatcher;
	typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


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


	template <typename Node1, typename Node2>
	bool checkNodeWithParentContent(const Node1 *node, const Node2 *parent, Int parent_idx) const;

	template <typename NodeTypes, bool root, bool leaf>
	bool canConvertToRootFn(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node) const
	{
		typedef TreeNode<TreeMapNode, NodeTypes, root, leaf> Node;
		typedef typename Node::RootNodeType RootType;

		Int node_children_count = node->size(0);

		Int root_block_size 	= node->page_size();

		Int root_children_count = RootType::max_tree_size_for_block(root_block_size);

		return node_children_count <= root_children_count;
	}

	template <typename NodeTypes, bool root, bool leaf>
	bool canConvertToRootFn(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node) const
	{
		typedef TreeNode<TreeLeafNode, NodeTypes, root, leaf> Node;
		typedef typename Node::RootNodeType RootType;

		Int root_block_size = node->page_size();

		Int root_available 	= RootType::client_area(root_block_size);
		Int node_used 		= node->total_size();

		return root_available >= node_used;
	}


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename Node1, typename Node2>
bool M_TYPE::checkNodeWithParentContent(const Node1 *node, const Node2 *parent, Int parent_idx) const
{
	return false;
}


#undef M_PARAMS
#undef M_TYPE

}


#endif
