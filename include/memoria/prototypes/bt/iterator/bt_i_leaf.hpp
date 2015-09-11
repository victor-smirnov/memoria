
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_LEAF_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_LEAF_H

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bt::IteratorLeafName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::Accumulator                                         Accumulator;
    typedef typename Container::Iterator                                            Iterator;

    using CtrSizeT = typename Container::Types::CtrSizeT;





    bool nextLeaf()
    {
        auto& self = this->self();

        auto id = self.leaf()->id();

        typename Types::template NextLeafWalker<Types> walker;

        self._findFw2(walker);

        return id != self.leaf()->id();
    }


    bool prevLeaf()
    {
        auto& self = this->self();

        auto id = self.leaf()->id();

        typename Types::template PrevLeafWalker<Types> walker;

        self._findBw2(walker);

        return id != self.leaf()->id();
    }



MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bt::IteratorLeafName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}


#endif
