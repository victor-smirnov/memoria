
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_DBLMAP2_INNER_I_NAV_HPP
#define _MEMORIA_CONTAINER_DBLMAP2_INNER_I_NAV_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>
#include <memoria/containers/dbl_map/dblmap_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {




MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::dblmap::InnerItrNavName)

    typedef Ctr<typename Types::CtrTypes>                       				Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;

    typedef typename Container::CtrSizeT                        				CtrSizeT;


    CtrSizeT skipFw(CtrSizeT amount) {
    	return self().template _findFw<Types::template SkipForwardWalker>(0, amount);
    }

    CtrSizeT skipBw(CtrSizeT amount) {
    	return self().template _findBw<Types::template SkipBackwardWalker>(0, amount);
    }

    bool operator++() {
    	return self().skipFw(1);
    }

    bool operator--() {
    	return self().skipBw(1);
    }

    bool operator++(int) {
    	return self().skipFw(1);
    }

    bool operator--(int) {
    	return self().skipBw(1);
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::dblmap::InnerItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS
}


#endif
