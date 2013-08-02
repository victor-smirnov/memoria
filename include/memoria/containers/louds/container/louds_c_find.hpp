
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LOUDS_LOUDS_C_FIND_HPP
#define MEMORIA_CONTAINERS_LOUDS_LOUDS_C_FIND_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/louds/louds_names.hpp>
#include <memoria/containers/louds/louds_tools.hpp>
#include <memoria/containers/seq_dense/seqd_walkers.hpp>


#include <memoria/prototypes/ctr_wrapper/iterator.hpp>

namespace memoria    {




MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrFindName)

	typedef TypesType                                                			Types;
	typedef typename Base::Iterator 											Iterator;


//	Iterator find(BigInt pos)
//	{
//		MyType& ctr = *me();
//
//		BigInt rank = 0;
//
//		auto rank_fn = [&rank](BigInt value, Int) {
//			rank += value;
//		};
//
//		Int node_indexes[1] = {2};
//		bt::FunctorExtenderState<> node_state(1, node_indexes, rank_fn);
//
//		Int data_indexes[1] = {1};
//		bt::FunctorExtenderState<> data_state(1, data_indexes, rank_fn);
//
//		typename MyType::WrappedCtr::Types::template SkipForwardWalker<
//			typename MyType::WrappedCtr::Types,
//			bt::NodeSumExtender,
//			bt::RankExtender,
//			bt::FunctorExtenderState<>
//		> walker(pos, 0, node_state, data_state);
//
//		auto seq_iter = ctr.ctr().find0(walker);
//
//		Iterator iter(*me(), seq_iter);
//
//		iter.node_rank() = rank + (!iter.isEof() ? iter.test(1) : 0);
//
//		return iter;
//	}


	Iterator select0(BigInt rank)
	{
		auto seq_iter = me()->ctr().select(rank, 0);

		Iterator iter(*me(), seq_iter);

		iter.node_rank() = seq_iter.pos() + 1 - rank;

		return iter;
	}



	Iterator select1(BigInt rank)
	{
		auto seq_iter = me()->ctr().select(rank, 1);

		Iterator iter(*me(), seq_iter);

		iter.node_rank() = rank;

		return iter;
	}

	BigInt rank1(BigInt idx)
	{
		return me()->ctr().rank(idx + 1, 1);
	}

	BigInt rank0(BigInt idx)
	{
		return me()->ctr().rank(idx + 1, 0);
	}

MEMORIA_CONTAINER_PART_END

}


#endif
