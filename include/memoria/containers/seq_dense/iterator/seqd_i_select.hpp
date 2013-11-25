
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


    BigInt select(BigInt rank_delta, Int symbol);
    BigInt selectFw(BigInt rank_delta, Int symbol);
    BigInt selectBw(BigInt rank_delta, Int symbol);
    
MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterSelectName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


M_PARAMS
BigInt M_TYPE::select(BigInt rank_delta, Int symbol)
{
    auto& self  = this->self();

    if (rank_delta > 0)
    {
        return self.selectFw(rank_delta, symbol);
    }
    else if (rank_delta < 0)
    {
        return self.selectBw(-rank_delta, symbol);
    }
    else {
        return 0;
    }
}



M_PARAMS
BigInt M_TYPE::selectFw(BigInt rank_delta, Int symbol)
{
    auto& self  = this->self();
    auto& ctr   = self.ctr();
    Int stream  = self.stream();

    MEMORIA_ASSERT(rank_delta, >=, 0);

    typename Types::template SelectFwWalker<Types> walker(stream, symbol, rank_delta);

    walker.prepare(self);

    Int idx = ctr.findFw(self.leaf(), stream, self.idx(), walker);

    return walker.finish(self, idx);
}



M_PARAMS
BigInt M_TYPE::selectBw(BigInt rank_delta, Int symbol)
{
    auto& self  = this->self();
    auto& ctr   = self.ctr();
    Int stream  = self.stream();

    MEMORIA_ASSERT(rank_delta, >=, 0);

    typename Types::template SelectBwWalker<Types> walker(stream, symbol, rank_delta);

    walker.prepare(self);

    Int idx = ctr.findBw(self.leaf(), stream, self.idx(), walker);

    return walker.finish(self, idx);
}




#undef M_TYPE
#undef M_PARAMS


}



#endif
