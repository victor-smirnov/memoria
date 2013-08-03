
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


	Iterator select0(BigInt rank)
	{
		return self().select(0, rank);
	}

	Iterator select1(BigInt rank)
	{
		return self().select(1, rank);
	}

	BigInt rank1(BigInt idx)
	{
		return self().rank(idx + 1, 1);
	}

	BigInt rank0(BigInt idx)
	{
		return self().rank(idx + 1, 0);
	}

MEMORIA_CONTAINER_PART_END

}


#endif
