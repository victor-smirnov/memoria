
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_VTREE_I_API_HPP
#define MEMORIA_CONTAINERS_VTREE_I_API_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/vector_tree/vtree_names.hpp>

#include <functional>

namespace memoria {


MEMORIA_ITERATOR_PART_BEGIN(memoria::vtree::ItrApiName)

	typedef Ctr<VTreeCtrTypes<Types>>                      		ContainerType;

	typedef typename ContainerType::Tree::Iterator            	TreeIterator;
	typedef typename ContainerType::Vec::Iterator         		VectorIterator;

	BigInt next_siblings() const
	{
		return self().tree_iter().next_siblings();
	}

	BigInt prev_siblings() const
	{
		return self().tree_iter().prev_siblings();
	}

	bool next_sibling()
	{
		return self().tree_iter().next_sibling();
	}

	bool prev_sibling()
	{
		return self().tree_iter().prev_sibling();
	}

	LoudsNode node() const
	{
		return self().tree_iter().node();
	}

MEMORIA_ITERATOR_PART_END

}


#endif
