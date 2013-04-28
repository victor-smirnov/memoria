
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_VECTOR2_C_INSERT_HPP
#define _MEMORIA_CONTAINER_VECTOR2_C_INSERT_HPP


#include <memoria/containers/vector2/vector_names.hpp>
#include <memoria/containers/vector2/vector_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector2::CtrInsertName)

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
	typedef typename Base::DefaultDispatcher                                    DefaultDispatcher;


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

	typedef typename Types::DataSource											DataSource;
	typedef typename Types::DataTarget											DataTarget;

    void insert(Iterator& iter, DataSource& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector2::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::insert(Iterator& iter, DataSource& data)
{
	auto& self = this->self();
	auto& ctr  = self;

	TreePath& path = iter.path();
	Position idx(iter.key_idx());

	mvector2::VectorSource source(&data);

	typename Base::DefaultSubtreeProvider provider(self, Position(data.getSize()), source);

	ctr.insertSubtree(path, idx, provider);

	ctr.addTotalKeyCount(Position(data.getSize()));

	if (iter.isEof())
	{
		iter.nextLeaf();
	}

	iter.skipFw(data.getSize());
}





#undef M_PARAMS
#undef M_TYPE

}


#endif
