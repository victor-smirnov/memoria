
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_SEQ_C_INSERT_HPP
#define _MEMORIA_CONTAINERS_SEQD_SEQ_C_INSERT_HPP


#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/containers/seq_dense/seqd_names.hpp>

#include <memoria/core/tools/static_array.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrInsertName)

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


	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;
	typedef typename Base::Element                                              Element;

	typedef typename Base::Metadata                                             Metadata;

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position 											Position;

	typedef typename Base::TreePath                                             TreePath;
	typedef typename Base::TreePathItem                                         TreePathItem;

	typedef typename Types::PageUpdateMgr										PageUpdateMgr;

	static const Int Indexes                                                    = Types::Indexes;
	static const Int Streams                                                    = Types::Streams;

	static const Int MAIN_STREAM												= Types::MAIN_STREAM;



	struct InsertIntoLeafFn {

		template <Int Idx, typename SeqTypes>
		void stream(PkdFSSeq<SeqTypes>* seq, Int idx, Int symbol, Accumulator* delta)
		{
			MEMORIA_ASSERT_TRUE(seq != nullptr);

			typedef PkdFSSeq<SeqTypes> 	Seq;
			typedef typename Seq::Value 				Symbol;

			auto old_indexes = seq->sums();

			seq->insert(idx, 1, [=]() -> Symbol {
				return symbol;
			});

			auto new_indexes = seq->sums();

			auto indexes = new_indexes - old_indexes;

			std::get<Idx>(*delta) = indexes;
		}


		template <typename NTypes>
		void treeNode(LeafNode<NTypes>* node, Int stream, Int idx, Int symbol, Accumulator* delta)
		{
			node->layout(1);
			node->process(stream, *this, idx, symbol, delta);
		}
	};



	bool insertIntoLeaf(NodeBaseG& leaf, Int idx, Int symbol, Accumulator& indexes)
	{
		auto& self = this->self();

		PageUpdateMgr mgr(self);

		mgr.add(leaf);

		try {
			LeafDispatcher::dispatch(leaf, InsertIntoLeafFn(), 0, idx, symbol, &indexes);
			return true;
		}
		catch (PackedOOMException& e)
		{
			mgr.rollback();
			return false;
		}
	}

	void insert(BigInt idx, Int symbol)
	{
		auto& self 	= this->self();
		auto iter 	= self.seek(idx);

		self.insert(iter, symbol);
	}

	void insert(Iterator& iter, Int symbol)
	{
		auto& self 	= this->self();
		auto& leaf 	= iter.leaf();
		Int& idx	= iter.idx();
		Int stream 	= iter.stream();

		Accumulator sums;

		if (self.insertIntoLeaf(leaf, idx, symbol, sums))
		{
			self.updateParent(leaf, sums);
		}
		else
		{
			Int size 		= iter.leaf_size(0);
			Int split_idx	= size/2;

			auto right = self.splitLeafP(leaf, Position::create(0, split_idx));

			if (idx > split_idx)
			{
				leaf = right;
				idx -= split_idx;
			}

			MEMORIA_ASSERT_TRUE(self.insertIntoLeaf(leaf, idx, symbol, sums));
			self.updateParent(leaf, sums);
		}

		self.addTotalKeyCount(Position::create(stream, 1));

		iter++;
	}


	void append(Int symbol)
	{

	}


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
