
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_VECTORMAP_C_REMOVE_HPP
#define _MEMORIA_CONTAINER_VECTORMAP_C_REMOVE_HPP


#include <memoria/containers/vector_map/vectormap_names.hpp>
#include <memoria/containers/vector_map/vectormap_tools.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::vmap::CtrRemoveName)

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


    void removeEntry(Iterator& iter);
    void removeData(Iterator& iter, BigInt size);

private:
    bool mergeLeaf(Iterator& iter);

    void removeWithinPage(Iterator& iter, BigInt size);
    void removeMultiPage(Iterator& iter, BigInt size);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::vmap::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::removeEntry(Iterator& iter)
{
	auto& self = this->self();

	Int idx					= iter.idx();
	BigInt local_offset		= iter.data_offset();
	BigInt size 			= iter.blob_size();
	BigInt data_leaf_size 	= iter.leafSize(1);

	Accumulator keys;

	if (local_offset + size <= data_leaf_size)
	{
		VectorAdd(keys, self.removeLeafContent(iter.leaf(), {idx, local_offset}, {idx + 1, local_offset + size}));
		self.addTotalKeyCount({-1, -size});

		if (iter.isEnd())
		{
			iter.cache().set(0,0, iter.idx(), 0, iter.cache().blob_base());
		}
		else {
			auto entry = iter.entry();
			iter.cache().set(entry.first, entry.second, iter.idx(), 0, iter.cache().blob_base());
		}

		self.mergeLeaf(iter);

		// Is it necessary here?
		iter.initState();

		if (iter.isEnd()) {
			iter++;
		}
	}
	else {
		Iterator to = iter;
		to.findEntry();

		if (!to++)
		{
			to--;
			to.skipFw(iter.blob_size());
		}
		else {
			to.seekLocal();
		}

		NodeBaseG& from_node 	= iter.leaf();
		Position from_idx		= {idx, local_offset};

		NodeBaseG& to_node 		= to.leaf();
		Position to_idx			= {0, to.idx()};

		Base::removeEntries(from_node, from_idx, to_node, to_idx, keys, true);

		iter.idx() = to_idx[0];

		iter.initState();

		if (iter.isEnd()) {
			iter++;
		}
	}

	if (!iter.isEnd())
	{
		std::get<0>(keys)[1] = 0;
		std::get<1>(keys)[0] = 0;

		iter.update(keys);
	}
}

M_PARAMS
bool M_TYPE::mergeLeaf(Iterator& iter)
{
	auto& self = this->self();

	MergeType merged = self.mergeWithSiblings(
			iter.leaf(), [&](const NodeBaseG& left, const NodeBaseG& right)
	{
		if (left->is_leaf())
		{
			Position sizes = self.getNodeSizes(left);

			Int stream = iter.stream();

			iter.idx() += sizes[stream];

			iter.cache().addEntryIdx(sizes[0]);
		}
	});

	return merged != MergeType::NONE;
}


M_PARAMS
void M_TYPE::removeData(Iterator& iter, BigInt size)
{
//	auto& self = this->self();

	MEMORIA_ASSERT_TRUE(iter.stream() == 1);

	BigInt idx 				= iter.idx();
	BigInt data_leaf_size 	= iter.leaf_size(1);
	BigInt map_leaf_size 	= iter.leaf_size(0);

	BigInt pos = iter.pos();

	if (pos + size > iter.blob_size())
	{
		size = iter.blob_size() - pos;
	}

	if (map_leaf_size > 0)
	{
		Int first_entry_data_offset = iter.data_offset_for(0);

		if (idx < first_entry_data_offset)
		{
			MEMORIA_ASSERT(idx + size, <=, first_entry_data_offset);

			removeWithinPage(iter, size);
		}
		else if (idx == first_entry_data_offset && pos > 0 )
		{
			MEMORIA_ASSERT(idx + size, <=, first_entry_data_offset);

			removeWithinPage(iter, size);
		}
		else if (idx + size <= data_leaf_size)
		{
			MEMORIA_ASSERT(idx + size, <=, data_leaf_size);

			removeWithinPage(iter, size);
		}
		else {
			MEMORIA_ASSERT(iter.cache().entry_idx(), ==, iter.leaf_size(0) - 1);

			removeMultiPage(iter, size);

//			if (pos == 0 && iter.blob_size() > 0)
//			{
//				iter.findEntry();
//
//				iter.dump();
//
//				Int map_size  = iter.leaf_size(0);
//
//				if (iter.idx() == map_size - 1)
//				{
//					Int data_size = iter.leaf_size(1);
//
//					if (iter.data_offset() == data_size)
//					{
//						auto tmp = iter;
//
//						if (tmp.nextLeaf())
//						{
//							if (map_size == 1) {
//
//							}
//
//
//							self.splitLeaf(iter, iter.cache().entry_idx());
//
//
//
//							if (!tmp.checkCapacities({1, 0}))
//							{
//								self.splitLeafData(tmp, 1);
//							}
//
//							if (self.mergeWithRightSibling(iter.path(), 0))
//							{
//								iter.cache().setEntries(iter.leaf_size(0));
//							}
//							else {
//								throw Exception(MA_SRC,
//										"VectorMap data integrity failure: entry page hasn't been merged with the data page");
//							}
//						}
//						else {
//							throw Exception(MA_SRC, "VectorMap data integrity failure: no data page found");
//						}
//					}
//				}
//			}
		}
	}
	else if (idx + size <= data_leaf_size)
	{
		removeWithinPage(iter, size);
	}
	else {
		removeMultiPage(iter, size);
	}
}

M_PARAMS
void M_TYPE::removeWithinPage(Iterator& iter, BigInt size)
{
	auto& self = this->self();

	BigInt local_offset	= iter.idx();

	Accumulator keys;
	std::get<0>(keys)[1] = -size;

	auto tmp = iter;
	tmp.findEntry();
	tmp.update(keys);

	iter.cache().addToEntry(0, -size);

	self.removeLeafContent(iter.leaf(), 1, local_offset, local_offset + size);

	self.addTotalKeyCount({0, -size});

	self.mergeLeaf(iter);
}

M_PARAMS
void M_TYPE::removeMultiPage(Iterator& iter, BigInt size)
{
	Accumulator keys;
	std::get<0>(keys)[1] = -size;

	Iterator to = iter;
	to.skipFw(size);

	auto iter_tmp = iter;

	iter_tmp.findEntry();
	iter_tmp.update(keys);

	iter.cache().addToEntry(0, -size);
	to.cache().addToEntry(0, -size);

	BigInt local_offset	= iter.idx();

	NodeBaseG& from_node 	= iter.leaf();
	Position from_idx		= {iter.leaf_size(0), local_offset};

	NodeBaseG to_node 		= to.leaf();
	Position to_idx			= {0, to.idx()};

	Base::removeEntries(from_node, from_idx, to_node, to_idx, keys, true);

	iter.idx() = to_idx[0];
}


}

#undef M_TYPE
#undef M_PARAMS


#endif
