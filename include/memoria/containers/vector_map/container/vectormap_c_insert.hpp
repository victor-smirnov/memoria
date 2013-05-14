
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

    void insert(Iterator& iter, BigInt id, DataSource& data);

    void splitLeaf(Iterator& iter, Int split_idx = -1);

    template <typename EntryData>
    void insertEntry(Iterator& iter, const EntryData&);

private:
    void splitLeafData(Iterator& iter, Int split_idx = -1);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::vmap::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename EntryData>
void M_TYPE::insertEntry(Iterator &iter, const EntryData& entry)
{
	Base::insertEntry(iter, entry);
	iter.cache().set(entry.first, entry.second, iter.idx());
}

M_PARAMS
void M_TYPE::insert(Iterator& iter, BigInt id, DataSource& src)
{
	auto& self = this->self();

	BigInt id_entry_value = id - iter.id();

	std::pair<BigInt, BigInt> pair(id_entry_value, src.getSize());

	NodeBaseG& leaf = iter.leaf();

	bool at_the_end = iter.isEnd();

	if (self.checkCapacities(leaf, {1, src.getSize()}) || self.isNodeEmpty(leaf))
	{
		self.insertEntry(iter, pair);
		iter.seek(0);
		self.insertData(iter, src);
	}
	else
	{
		Int map_stream_size = iter.leafSize(0);

		if (iter.idx() < map_stream_size)
		{
			self.splitLeaf(iter, iter.idx());

			self.insertEntry(iter, pair);
			iter.seek(0);
			self.insertData(iter, src);
		}
		else {
			iter--;

			BigInt blob_size 	= iter.blob_size();
			Int leaf_data_size 	= iter.leafSize(1);
			Int blob_offset 	= iter.data_offset();

			if (blob_offset + blob_size > leaf_data_size)
			{
				iter.seek(blob_size);

				Int capacity = iter.leaf_capacity(0);

				if (capacity == 1)
				{
					if (src.getSize() > 0)
					{
						Int data_capacity = iter.leaf_capacity({1, 0}, 1);
						if (data_capacity == 0)
						{
							self.splitLeafData(iter);
						}
					}
				}
				else if (capacity == 0)
				{
					self.splitLeafData(iter);
				}

				Int data_pos = iter.idx();

				iter.idx() 		= 0;
				iter.stream() 	= 0;

				Base::insertEntry(iter, pair);

				iter.cache().add(pair.first, pair.second, iter.idx());

				iter.idx() 		= data_pos;
				iter.stream() 	= 1;

				self.insertData(iter, src);
			}
			else {
				MEMORIA_ASSERT_TRUE(blob_offset + blob_size == leaf_data_size);

				iter++;

				Int data_capacity = iter.leaf_capacity({1, 0}, 1);

				if (data_capacity > 0 || src.getSize() == 0)
				{
					self.insertEntry(iter, pair);
					iter.seek(0);
					self.insertData(iter, src);
				}
				else {
					self.splitLeaf(iter, iter.idx());
					self.insertEntry(iter, pair);

					iter.stream() = 1;
					iter.idx()    = 0;

					self.insertData(iter, src);
				}
			}
		}
	}

	if (!at_the_end)
	{
		iter.findEntry();
		iter++;

		Accumulator accum;

		std::get<0>(accum)[0] = -id_entry_value;

		iter.update(accum);

		iter--;
	}
}



M_PARAMS
void M_TYPE::splitLeaf(Iterator& iter, Int split_idx)
{
	auto& self = this->self();

	if (split_idx == -1)
	{
		split_idx = iter.leaf_size(0) / 2;
	}

	UBigInt active_streams = self.getActiveStreams(iter.leaf());

	if (split_idx < iter.idx())
	{
		Int data_offset = iter.data_offset_for(split_idx);

		auto right = iter.path();

		self.splitPath(iter.path(), right, 0, {split_idx, data_offset}, active_streams);

		iter.idx() -= split_idx;
	}
	else if (split_idx == iter.idx())
	{
		Int data_offset = iter.data_offset();

		auto right = iter.path();

		self.splitPath(iter.path(), right, 0, {split_idx, data_offset}, active_streams);

		Int left_data_capacity  = iter.leaf_capacity({1, 0}, 1);
		Int right_data_capacity = self.getStreamCapacity(right.leaf(), {1, 0}, 1);

		if (right_data_capacity > left_data_capacity)
		{
			iter.path() = right;
			iter.idx()  = 0;

			iter.cache().setEntryIdx(iter.idx());
		}
		else {
			iter.cache().set(0, 0, iter.idx());
		}
	}
	else {
		Int data_offset = iter.data_offset_for(split_idx);

		auto right = iter.path();

		self.splitPath(iter.path(), right, 0, {split_idx, data_offset}, active_streams);
	}
}

M_PARAMS
void M_TYPE::splitLeafData(Iterator& iter, Int split_idx)
{
	auto& self = this->self();

	if (split_idx == -1)
	{
		split_idx = iter.idx() / 2;
	}

	auto right = iter.path();
	self.split(iter.path(), right, 0, {0, split_idx});

	iter.path() = right;

	if (split_idx <= iter.idx())
	{
		iter.idx() -= split_idx;
	}
}


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
	Position idx(iter.idx());

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
