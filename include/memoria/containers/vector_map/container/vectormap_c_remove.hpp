
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
		self.removeRoom(iter.path(), 0, {idx, local_offset}, {1, size}, keys);
		self.addTotalKeyCount(iter.path(), {-1, -size});

		if (iter.isEnd())
		{
			iter.cache().set(0,0, iter.idx());
		}
		else {
			auto entry = iter.entry();
			iter.cache().set(entry.first, entry.second, iter.idx());
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
		to.seek(size);

		TreePath& from_path = iter.path();
		Position from_idx	= {idx, local_offset};

		TreePath to_path 	= to.path();
		Position to_idx		= {0, to.idx()};

		Base::removeEntries(from_path, from_idx, to_path, to_idx, keys, true);

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

	Position idx;

	idx[0] = iter.idx();

	bool merged = self.mergeWithSiblings(iter.path(), 0, idx);

	iter.idx() = idx[0];

	return merged;
}


M_PARAMS
void M_TYPE::removeData(Iterator& iter, BigInt size)
{
	MEMORIA_ASSERT_TRUE(iter.stream() == 1);

	auto& self = this->self();

	BigInt local_offset		= iter.idx();
	BigInt data_leaf_size 	= iter.leaf_size();
	BigInt pos				= iter.pos();

	Accumulator keys;

	if (local_offset + size <= data_leaf_size)
	{
		self.removeRoom(iter.path(), 0, {0, local_offset}, {0, size}, keys);
		self.addTotalKeyCount(iter.path(), {0, -size});

		self.mergeLeaf(iter);
	}
	else {
		Iterator to = iter;
		to.seek(size);

		TreePath& from_path = iter.path();
		Position from_idx	= {iter.cache().entry_idx() + 1, local_offset};

		TreePath to_path 	= to.path();
		Position to_idx		= {0, to.idx()};

		Base::removeEntries(from_path, from_idx, to_path, to_idx, keys, true);

		iter.idx() = to_idx[0];
	}

	iter.findEntry();

	if (!iter.isEnd())
	{
		std::get<0>(keys)[0] = 0;
		std::get<0>(keys)[1] = -size;
		std::get<1>(keys)[0] = 0;

		iter.update(keys);

		iter.seek(pos);
	}


}



}

#undef M_TYPE
#undef M_PARAMS


#endif
