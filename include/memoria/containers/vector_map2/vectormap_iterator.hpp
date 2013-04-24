
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_VECTORMAP2_ITERATOR_HPP_
#define MEMORIA_CONTAINERS_VECTORMAP2_ITERATOR_HPP_

#include <memoria/prototypes/ctr_wrapper/iterator.hpp>
#include <memoria/containers/vector_map2/vectormap_names.hpp>

namespace memoria {

template <typename Types, typename Value>
void AssignToItem(Iter<VectorMap2IterTypes<Types>>& iter, const Value& value)
{
    iter.setValue(value);
}

}


#endif
