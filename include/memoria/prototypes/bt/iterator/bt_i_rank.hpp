
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_RANK_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_RANK_H

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bt::IteratorRankName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::Accumulator                                         Accumulator;
    typedef typename Container::Iterator                                            Iterator;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    template <typename LeafPath>
    WalkerResultFnType<typename Types::template RankForwardWalker<Types, LeafPath>>
    _rankFw(Int index, CtrSizeT pos)
    {
    	MEMORIA_ASSERT(pos, >=, 0);
    	MEMORIA_ASSERT(index, >=, 0);

    	typename Types::template RankForwardWalker<Types, LeafPath> walker(index, pos);

    	return self()._findFw2(walker);
    }

    template <typename LeafPath>
    WalkerResultFnType<typename Types::template RankBackwardWalker<Types, LeafPath>>
    _rankBw(Int index, CtrSizeT pos)
    {
    	MEMORIA_ASSERT(pos, >=, 0);
    	MEMORIA_ASSERT(index, >=, 0);

    	typename Types::template RankBackwardWalker<Types, LeafPath> walker(index, pos);

    	return self()._findBw2(walker);
    }

    template <typename LeafPath>
    WalkerResultFnType<typename Types::template RankForwardWalker<Types, LeafPath>>
    _rank(Int index, CtrSizeT pos)
    {
    	if (pos >= 0)
    	{
    		return self().template _rankFw<LeafPath>(index, pos);
    	}
    	else {
    		return self().template _rankBw<LeafPath>(index, -pos);
    	}
    }


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bt::IteratorRankName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS





#undef M_PARAMS
#undef M_TYPE

}


#endif
