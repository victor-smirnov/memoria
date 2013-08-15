
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_SEQ_C_FIND_HPP
#define _MEMORIA_CONTAINERS_SEQD_SEQ_C_FIND_HPP


#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/containers/seq_dense/seqd_names.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrFindName)

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

	static const Int Indexes                                                    = Types::Indexes;
	static const Int Streams                                                    = Types::Streams;

	static const Int MAIN_STREAM												= Types::MAIN_STREAM;

	BigInt size() const
	{
		return self().sizes()[0];
	}

	BigInt rank(BigInt idx, Int symbol)
	{
		auto& self = this->self();

		typename Types::template RankFWWalker<Types> walker(0, symbol, idx);

		auto iter = self.find0(0, walker);

		return walker.rank();
	}

	BigInt rank(BigInt start, BigInt idx, Int symbol)
	{
		auto& self = this->self();

		auto iter = self.seek(start);

		return iter.rankFw(idx, symbol);
	}



	Iterator select(Int symbol, BigInt rank)
	{
		auto& self = this->self();

		if (rank < 1) {
			int a = 0; a++;
		}

		MEMORIA_ASSERT(rank, >=, 1);
		MEMORIA_ASSERT(symbol, >=, 0);

		typename Types::template SelectFwWalker<Types> walker(0, symbol, rank);

		auto iter = self.find0(0, walker);

		return iter;
	}

	Iterator select(BigInt start, Int symbol, BigInt rank)
	{
		auto& self = this->self();

		auto iter = self.seek(start);

		iter.selectFw(symbol, rank);

		return iter;
	}

	Iterator seek(Int pos)
	{
		auto& self = this->self();

		return self.findLT(MAIN_STREAM, pos, 0);
	}


	Int symbol(Int idx)
	{
		auto& self = this->self();
		return seek(idx).symbol();
	}



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrFindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
