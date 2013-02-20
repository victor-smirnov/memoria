
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LOUDS_LOUDS_C_API_HPP
#define MEMORIA_CONTAINERS_LOUDS_LOUDS_C_API_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/louds/louds_names.hpp>

namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrApiName)


	BigInt parent()
	{
		return 0;
	}

	BigInt child(BigInt child_num)
	{
		return 0;
	}

	BigInt nextSibling(BigInt node)
	{
		return 0;
	}

	BigInt prevSibling(BigInt node)
	{
		return 0;
	}

	bool isLeaf(BigInt node)
	{
		return true;
	}

	BigInt firstChild(BigInt name)
	{
		return 0;
	}

	BigInt lastChild(BigInt name)
	{
		return 0;
	}

MEMORIA_CONTAINER_PART_END

}


#endif
