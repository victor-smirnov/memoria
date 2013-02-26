
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LOUDS_LOUDS_C_FIND_HPP
#define MEMORIA_CONTAINERS_LOUDS_LOUDS_C_FIND_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/louds/louds_names.hpp>
#include <memoria/containers/louds/louds_tools.hpp>
#include <memoria/containers/seq_dense/walkers.hpp>


#include <memoria/prototypes/ctr_wrapper/iterator.hpp>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrFindName)

	typedef TypesType                                                			Types;
	typedef typename Base::Iterator 											Iterator;


	Iterator find(BigInt pos)
	{
		BigInt 	prefixes[2] = {0, 0};
		Int 	key_nums[2] = {0, 2};

		BigInt& size_prefix 	= prefixes[0];
		BigInt& rank1_prefix 	= prefixes[2];

		SequenceFWWalker<typename MyType::WrappedCtr::Types, SumCompareLE> walker(me()->ctr(), pos, 2, key_nums, prefixes);
		auto iter = me()->ctr().find0(walker);

		if (iter.isNotEmpty())
		{
			BigInt local_pos = pos - size_prefix;

			BigInt rank;

			if (local_pos < iter.data()->size())
			{
				rank = iter.data()->sequence().rank1(local_pos + 1, 1);
				iter.dataPos() = local_pos;
			}
			else
			{
				rank = iter.data()->sequence().maxIndex(1);
				iter.dataPos() = iter.data()->size();
			}

			iter.cache().setup(size_prefix, 0);

			Iterator louds_iter(*me(), iter);

			louds_iter.node_rank() = rank1_prefix + rank;

			return louds_iter;
		}
		else {
			return Iterator(*me(), iter);
		}
	}


	Iterator select0(BigInt rank)
	{
		BigInt 	prefixes[2] = {0, 0};
		Int 	key_nums[2] = {1, 0};

		BigInt& rank0_prefix	= prefixes[0];
		BigInt& size_prefix 	= prefixes[1];
//		BigInt& rank1_prefix 	= prefixes[2];

		SequenceFWWalker<typename MyType::WrappedCtr::Types, SumCompareLT> walker(me()->ctr(), rank, 2, key_nums, prefixes);
		auto iter = me()->ctr().find0(walker);

		if (iter.isNotEmpty())
		{
			BigInt local_rank = rank - rank0_prefix;

			louds::Select0Walker<
				typename MyType::WrappedCtr::Types::DataPage::Sequence
			>
			data_walker(iter.data()->sequence(), local_rank);

			Int idx = iter.data()->sequence().findFw(0, data_walker);

			if (data_walker.is_found())
			{
				iter.dataPos() = idx;
			}
			else {
				iter.dataPos() = iter.data()->size();
			}

			iter.cache().setup(size_prefix, 0);

			Iterator louds_iter(*me(), iter);

			louds_iter.node_rank() = (size_prefix - rank0_prefix) + (idx + 1 - data_walker.rank0());

			return louds_iter;
		}
		else {
			return Iterator(*me(), iter);
		}
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
