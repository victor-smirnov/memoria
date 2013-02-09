
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BSTREE_ITERATOR_FIND_H
#define __MEMORIA_PROTOTYPES_BSTREE_ITERATOR_FIND_H

#include <iostream>

#include <memoria/core/types/types.hpp>

#include <memoria/containers/map/names.hpp>
#include <memoria/core/container/iterator.hpp>



namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bstree::IterFindName)

    typedef typename Base::NodeBase                                                 NodeBase;
    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;
    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::Accumulator                                         Accumulator;
    typedef typename Container::TreePath                                            TreePath;


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bstree::IterFindName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE



}


#endif
