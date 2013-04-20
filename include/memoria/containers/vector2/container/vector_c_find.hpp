
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_VECTOR2_C_FIND_HPP
#define _MEMORIA_CONTAINER_VECTOR2_C_FIND_HPP


#include <memoria/containers/vector2/vector_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector2::CtrFindName)

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


//    template <typename Walker>
//    Int findFw(TreePath& path, Int stream, Int idx, Walker&& walker);
//
//    template <typename Walker>
//    Int findBw(TreePath& path, Int stream, Int idx, Walker&& walker);


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector2::CtrFindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

//M_PARAMS
//template <typename Walker>
//Int M_TYPE::findFw(TreePath& path, Int stream, Int idx, Walker&& walker)
//{
//	auto& self = this->self();
//
//	NodeBaseG node = path.leaf();
//
//	if (node->is_root())
//	{
//		walker.direction() 	= WalkDirection::DOWN;
//	}
//	else {
//		walker.direction() 	= WalkDirection::UP;
//	}
//
//	walker.idx() = walker.start() = idx;
//
//	Int size = node->size(stream);
//
//	Int result;
//
//	if (idx < size)
//	{
//		result = LeafDispatcher::dispatchConstRtn(node, walker);
//	}
//	else {
//		reult = size;
//	}
//
//	if (result >= size)
//	{
//		if (!node->is_root())
//		{
//			// Step up the tree
//			Int child_idx = self.nonLeafFindFw(path, path.leaf().parent_idx() + 1, walker, 1);
//
//			if (child_idx < path[1].node()->children_count())
//			{
//				// Step down the tree
//				NodeBaseG child_node		= self.getChild(path[1].node(), child_idx, Allocator::READ);
//
//				path.leaf().node() 			= child_node;
//				path.leaf().parent_idx()	= child_idx;
//
//				return LeafDispatcher::dispatchConstRtn(child_node, walker);
//			}
//			else {
//				return path[level].node()->children_count();
//			}
//		}
//		else {
//			walker.direction() 	= WalkDirection::DOWN;
//			walker.start() 		= 0;
//
//			return walker.idx();
//		}
//	}
//	else {
//		walker.direction() 	= WalkDirection::DOWN;
//		walker.start() 		= 0;
//
//		return walker.idx();
//	}
//
//
//	return Base::template nonLeafFindFw(path, idx, walker, 0);
//}
//
//M_PARAMS
//template <typename Walker>
//Int M_TYPE::findBw(TreePath& path, Int stream, Int idx, Walker&& walker)
//{
//	return Base::template nonLeafFindBw(path, idx, walker, 0);
//}



#undef M_TYPE
#undef M_PARAMS

}


#endif
