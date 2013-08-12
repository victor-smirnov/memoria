
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_VECTORMAP_C_UPDATE_HPP
#define _MEMORIA_CONTAINER_VECTORMAP_C_UPDATE_HPP


#include <memoria/containers/vector_map/vmap_names.hpp>
#include <memoria/containers/vector_map/vmap_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/tools/idata.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::vmap::CtrUpdateName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Base::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
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

	void replaceData(Iterator& iter, DataSource& data);
	BigInt updateData(Iterator& iter, DataSource& data);

private:

	MEMORIA_DECLARE_NODE_FN(UpdateFn, update);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::vmap::CtrUpdateName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::replaceData(Iterator& iter, DataSource& data)
{
	MEMORIA_ASSERT_TRUE(iter.stream() == 1);

	auto& self = this->self();

	BigInt entry_size 	= iter.blob_size();
	BigInt data_size	= data.getRemainder();

	if (entry_size < data_size)
	{
		if (entry_size > 0)
		{
			memoria::vapi::DataSourceProxy<Value> proxy(data, entry_size);

			BigInt updated = self.updateData(iter, proxy);
			MEMORIA_ASSERT(updated, ==, entry_size);

//			iter.skipBw(1);
//			iter.idx()++;
		}

		self.insertData(iter, data);
	}
	else if (entry_size > data_size)
	{
		BigInt updated = self.updateData(iter, data);
		MEMORIA_ASSERT(updated, ==, data_size);

		self.removeData(iter, entry_size - data_size);
	}
	else { // entry_size == data_size
		self.updateData(iter, data);
	}
}



M_PARAMS
BigInt M_TYPE::updateData(Iterator& iter, DataSource& data)
{
	MEMORIA_ASSERT_TRUE(iter.stream() == 1);

//	auto& self = this->self();

	BigInt sum = 0;
	BigInt len = data.getRemainder();

	while (len > 0)
	{
		Int to_read = iter.leaf_size() - iter.idx();

		if (to_read > len) to_read = len;

		vmap::VectorMapSource target(&data);

		LeafDispatcher::dispatch(iter.leaf(), UpdateFn(), &target, Position({0, iter.idx()}), Position({0, to_read}));

		len     -= to_read;
		sum     += to_read;

		iter.skipFw(to_read);

		if (iter.isEof())
		{
			break;
		}
	}

	return sum;
}


#undef M_PARAMS
#undef M_TYPE

}


#endif
