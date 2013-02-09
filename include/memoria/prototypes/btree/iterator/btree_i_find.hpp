
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_FIND_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_FIND_H

#include <iostream>

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/btree/names.hpp>
#include <memoria/core/container/iterator.hpp>



namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::btree::IteratorFindName)

    typedef typename Base::NodeBase                                                 NodeBase;
    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;
    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::Accumulator                                         Accumulator;
    typedef typename Container::TreePath                                            TreePath;
    typedef typename Container::Iterator                                            Iterator;

    template <typename Comparator>
    const Iterator _findFW(Key key, Int c);

    template <typename Comparator>
    const Iterator _findBW(Key key, Int c);

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::btree::IteratorFindName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
template <typename Comparator>
const typename M_TYPE::Iterator M_TYPE::_findFW(Key key, Int c)
{
	return Iterator(*me());
}

M_PARAMS
template <typename Comparator>
const typename M_TYPE::Iterator M_TYPE::_findBW(Key key, Int c)
{
	return Iterator(*me());
}

#undef M_PARAMS
#undef M_TYPE



}


#endif
