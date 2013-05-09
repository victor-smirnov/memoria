
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_VECTORMAP_C_INSERT_HPP
#define _MEMORIA_CONTAINER_VECTORMAP_C_INSERT_HPP


#include <memoria/containers/vector_map/vectormap_names.hpp>
#include <memoria/containers/vector_map/vectormap_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/tools/idata.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::vmap::CtrInsertName)

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

	typedef typename Types::IDataSourceType										DataSource;
	typedef typename Types::IDataTargetType										DataTarget;


	void replaceEntry(BigInt id, DataSource& data);


    void insertData(Iterator& iter, DataSource& data);
    BigInt removeData(Iterator& iter, BigInt size);
    BigInt updateData(Iterator& iter, DataSource& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::vmap::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::replaceEntry(BigInt id, DataSource& data)
{
	auto& self = this->self();

	Iterator iter = self.find(id);

	if (!iter.found())
	{
		self.insertEntry(iter, std::pair<BigInt, BigInt>(id, data.getSize()));
		iter.findData();
		self.insertData(iter, data);
	}
	else {
		BigInt entry_size 	= iter.entrySize();
		BigInt data_size	= data.getSize();

		if (entry_size < data_size)
		{
			memoria::vapi::DataSourceProxy<Value> proxy(data, entry_size);

			self.updateData(iter, proxy);
			self.insertData(iter, data);
		}
		else if (entry_size > data_size)
		{
			self.updateData(iter, data);
			self.removeData(iter, entry_size - data_size);
		}
		else { // entry_size == data_size
			self.updateData(iter, data);
		}
	}
}

M_PARAMS
void M_TYPE::insertData(Iterator& iter, DataSource& data)
{
	auto& self = this->self();
	auto& ctr  = self;

	TreePath& path = iter.path();
	Position idx(iter.key_idx());

	vmap::VectorMapSource source(&data);

	typename Base::DefaultSubtreeProvider provider(self, Position::create(1, data.getSize()), source);

	ctr.insertSubtree(path, idx, provider);

	ctr.addTotalKeyCount(Position::create(1, data.getSize()));

	if (iter.isEof())
	{
		iter.nextLeaf();
	}

	iter.skipFw(data.getSize());
}

M_PARAMS
BigInt M_TYPE::removeData(Iterator& iter, BigInt size)
{
	return size;
}

M_PARAMS
BigInt M_TYPE::updateData(Iterator& iter, DataSource& data)
{
//	auto& self = this->self();
//
//	BigInt sum = 0;
//	BigInt len = data.getRemainder();
//
//	while (len > 0)
//	{
//		Int to_read = self.size() - self.dataPos();
//
//		if (to_read > len) to_read = len;
//
//		mvector::VectorTarget target(&data);
//
////		LeafDispatcher::dispatchConst(self.leaf().node(), ReadFn(), &target, Position(self.dataPos()), Position(to_read));
//
//		len     -= to_read;
//		sum     += to_read;
//
//		self.skipFw(to_read);
//
//		if (self.isEof())
//		{
//			break;
//		}
//	}
//
//	return sum;

	return data.getSize();
}


#undef M_PARAMS
#undef M_TYPE

}


#endif
