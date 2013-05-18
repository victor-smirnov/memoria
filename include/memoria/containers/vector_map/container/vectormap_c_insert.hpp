
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



	void insertData(Iterator& iter, DataSource& data);
    void insert(Iterator& iter, BigInt id, DataSource& data);


private:

    void splitLeaf(Iterator& iter, Int split_idx = -1);

    template <typename EntryData>
    void insertMapEntry(Iterator& iter, const EntryData&);


    void splitLeafData(Iterator& iter, Int split_idx = -1);
    void insertDataInternal(Iterator& iter, const Position& idx, DataSource& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::vmap::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS




M_PARAMS
void M_TYPE::insertData(Iterator& iter, DataSource& data)
{
	auto& self = this->self();

	MEMORIA_ASSERT_TRUE(iter.stream() == 1);

	BigInt data_size = data.getRemainder();
	BigInt pos		 = iter.pos();

	NodeBaseG& leaf = iter.leaf();

	Int idx = iter.idx();

	if (self.checkCapacities(leaf, {0, data_size}) || self.isNodeEmpty(leaf))
	{
		insertDataInternal(iter, {-1, idx}, data);
	}
	else {

		if (iter.leaf_size(0) == 0)
		{
			splitLeafData(iter, iter.idx());
		}
		else {
			Int first_entry_data_offset = iter.data_offset_for(0);
			Int entry_idx				= iter.cache().entry_idx();

			if (idx <= first_entry_data_offset)
			{
				auto right = iter.path();

				self.splitPath(iter.path(), right, 0, {0, idx}, 3);

				insertDataInternal(iter, {0, idx}, data);
			}
			else
			{
				auto right = iter.path();

				self.splitPath(iter.path(), right, 0, {entry_idx + 1, idx}, 3);

				insertDataInternal(iter, {0, idx}, data);
			}
		}
	}

	iter.findEntry();

	Accumulator accum;

	std::get<0>(accum)[1] = data_size;

	iter.update(accum);

	iter.seek(pos + data_size);

	self.addTotalKeyCount({0, data_size});
}



M_PARAMS
void M_TYPE::insert(Iterator& iter, BigInt id, DataSource& src)
{
	auto& self = this->self();

	BigInt id_entry_value = id - iter.cache().id_prefix();

	std::pair<BigInt, BigInt> pair(id_entry_value, src.getSize());

	NodeBaseG& leaf = iter.leaf();

	bool at_the_end = iter.isEnd();

	BigInt data_size = src.getRemainder();

	if (self.checkCapacities(leaf, {1, data_size}) || self.isNodeEmpty(leaf))
	{
		insertMapEntry(iter, pair);
		iter.seekLocal();
		insertDataInternal(iter, {-1, iter.idx()}, src);
		iter.skipFw(data_size);
	}
	else
	{
		Int map_stream_size = iter.leafSize(0);

		if (iter.idx() < map_stream_size)
		{
			self.splitLeaf(iter, iter.idx());

			insertMapEntry(iter, pair);

			iter.seekLocal();

			insertDataInternal(iter, {-1, iter.idx()}, src);
			iter.skipFw(data_size);
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

				self.insertDataInternal(iter, {-1, iter.idx()}, src);
				iter.skipFw(data_size);
			}
			else {
				MEMORIA_ASSERT_TRUE(blob_offset + blob_size == leaf_data_size);

				iter++;

				Int data_capacity = iter.leaf_capacity({1, 0}, 1);

				if (data_capacity > 0)
				{
					insertMapEntry(iter, pair);

					iter.seekLocal();

					insertDataInternal(iter, {-1, iter.idx()}, src);
					iter.skipFw(data_size);
				}
				else {
					Int map_capacity = iter.leaf_capacity(0);

					if (map_capacity > 0 && data_size == 0)
					{
						insertMapEntry(iter, pair);
					}
					else {
						self.splitLeaf(iter, iter.idx());

						insertMapEntry(iter, pair);

						iter.stream() = 1;
						iter.idx()    = 0;

						insertDataInternal(iter, {-1, iter.idx()}, src);
						iter.skipFw(data_size);
					}
				}
			}
		}
	}

	self.addTotalKeyCount({0, data_size});

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

		bool return_right_path = split_idx == iter.leafSize(0);

		self.splitPath(iter.path(), right, 0, {split_idx, data_offset}, active_streams);

		if (return_right_path)
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
template <typename EntryData>
void M_TYPE::insertMapEntry(Iterator &iter, const EntryData& entry)
{
	Base::insertEntry(iter, entry);
	iter.cache().set(entry.first, entry.second, iter.idx());
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
void M_TYPE::insertDataInternal(Iterator& iter, const Position& idx, DataSource& data)
{
	if (data.getSize() > 0)
	{
		auto& self = this->self();
		auto& ctr  = self;

		TreePath& path = iter.path();

		vmap::VectorMapSource source(&data);

		typename Base::DefaultSubtreeProvider provider(self, {0, data.getRemainder()}, source);

		Position idx0 = idx;

		ctr.insertSubtree(path, idx0, provider);

		if (iter.isEof())
		{
			iter.nextLeaf();
		}
	}
}



#undef M_PARAMS
#undef M_TYPE

}


#endif
