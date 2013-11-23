
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_FIND_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_FIND_H

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bt::IteratorFindName)

    typedef typename Base::NodeBase                                                 NodeBase;
    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;
    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::Accumulator                                         Accumulator;
    typedef typename Container::Iterator                                            Iterator;

    template <template <typename CtrTypes> class Walker>
    BigInt _findFw(Int index, Key key);

    template <template <typename CtrTypes> class Walker>
    BigInt _findBw(Int index, Key key);

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bt::IteratorFindName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
template <template <typename CtrTypes> class Walker>
BigInt M_TYPE::_findFw(Int index, Key key)
{
    auto& self = this->self();
    Int stream = self.stream();

    Walker<Types> walker(stream, index, key);

    walker.prepare(self);

    Int idx = self.model().findFw(self.leaf(), stream, self.key_idx(), walker);

    return walker.finish(self, idx);
}

M_PARAMS
template <template <typename CtrTypes> class Walker>
BigInt M_TYPE::_findBw(Int index, Key key)
{
    auto& self = this->self();
    Int stream = self.stream();

    Walker<Types> walker(stream, index, key);

    walker.prepare(self);

    Int idx = self.model().findBw(self.leaf(), stream, self.key_idx(), walker);

    return walker.finish(self, idx);
}

#undef M_PARAMS
#undef M_TYPE



}


#endif
