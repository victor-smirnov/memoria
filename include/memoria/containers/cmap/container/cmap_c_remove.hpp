
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_CMAP_CTR_REMOVE_HPP
#define _MEMORIA_CONTAINERS_CMAP_CTR_REMOVE_HPP


#include <memoria/containers/cmap/cmap_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::cmap::CtrRemoveName)

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

	bool removeMapEntries(Iterator& from, Iterator& to, Accumulator& keys);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::cmap::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::removeMapEntries(Iterator& from, Iterator& to, Accumulator& keys)
{
	auto& ctr = self();

	auto& from_path 	= from.leaf();
	Position from_pos 	= Position(from.idx());

	auto& to_path 		= to.leaf();
	Position to_pos 	= Position(to.idx());

	bool result = ctr.removeEntries(from_path, from_pos, to_path, to_pos, keys, true).gtAny(0);

	from.idx() = to.idx() = to_pos.get();

	return result;
}


}

#undef M_TYPE
#undef M_PARAMS


#endif
