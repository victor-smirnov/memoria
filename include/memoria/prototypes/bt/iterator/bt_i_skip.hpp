
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_SKIP_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_SKIP_H

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bt::IteratorSkipName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::Accumulator                                         Accumulator;
    typedef typename Container::Iterator                                            Iterator;

    using CtrSizeT = typename Container::Types::CtrSizeT;



    template <Int Stream>
    auto _skipFw(CtrSizeT amount) ->
    WalkerResultFnType<typename Types::template SkipForwardWalker<Types, IntList<Stream>>>
    {
    	MEMORIA_ASSERT(amount, >=, 0);

    	typename Types::template SkipForwardWalker<Types, IntList<Stream>> walker(amount);

        return self()._findFw2(walker);
    }

    template <Int Stream>
    auto _skipBw(CtrSizeT amount) ->
    WalkerResultFnType<typename Types::template SkipForwardWalker<Types, IntList<Stream>>>
    {
    	MEMORIA_ASSERT(amount, >=, 0);

    	typename Types::template SkipBackwardWalker<Types, IntList<Stream>> walker(amount);

    	return self()._findBw2(walker);
    }

    template <Int Stream>
    auto _skip(CtrSizeT amount) ->
    WalkerResultFnType<typename Types::template SkipForwardWalker<Types, IntList<Stream>>>
    {
        auto& self = this->self();

        if (amount >= 0)
        {
            return self.template _skipFw<Stream>(amount);
        }
        else {
            return self.template _skipBw<Stream>(-amount);
        }
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bt::IteratorSkipName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




#undef M_PARAMS
#undef M_TYPE

}


#endif
