
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_ITERATOR_SELECT_HPP
#define _MEMORIA_CONTAINERS_SEQD_ITERATOR_SELECT_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterSelectName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;


    auto select(BigInt rank_delta, Int symbol)
    {
    	return self().template select_<IntList<0>>(symbol, rank_delta);
    }

    auto selectFw(BigInt rank_delta, Int symbol)
    {
    	return self().template _selectFw<IntList<0>>(symbol, rank_delta);
    }

    auto selectBw(BigInt rank_delta, Int symbol)
    {
    	return self().template _selectBw<IntList<0>>(symbol, rank_delta);
    }
    
MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterSelectName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS


}



#endif
