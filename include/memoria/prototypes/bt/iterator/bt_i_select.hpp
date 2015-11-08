
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_SELECT_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_SELECT_H

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bt::IteratorSelectName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::Accumulator                                         Accumulator;
    typedef typename Container::Iterator                                            Iterator;

    using CtrSizeT = typename Container::Types::CtrSizeT;


    template <typename LeafPath>
    auto select_fw_(Int index, CtrSizeT rank)
    {
    	MEMORIA_ASSERT(index, >=, 0);
    	MEMORIA_ASSERT(rank, >=, 0);

    	typename Types::template SelectForwardWalker<Types, LeafPath> walker(index, rank);

    	return self().find_fw(walker);
    }

    template <typename LeafPath>
    auto select_bw_(Int index, CtrSizeT rank)
    {
    	MEMORIA_ASSERT(index, >=, 0);
    	MEMORIA_ASSERT(rank, >=, 0);

    	typename Types::template SelectBackwardWalker<Types, LeafPath> walker(index, rank);

    	return self().find_bw(walker);
    }
MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bt::IteratorSelectName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS






#undef M_PARAMS
#undef M_TYPE

}


#endif
