
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_TABLE_ITERATOR_HPP_
#define MEMORIA_CONTAINERS_TABLE_ITERATOR_HPP_

#include <memoria/prototypes/ctr_wrapper/iterator.hpp>
#include <memoria/containers/table/table_names.hpp>

namespace memoria {

template <typename Types, typename Value>
void AssignToItem(Iter<TableIterTypes<Types>>& iter, Value&& value)
{
    iter.setValue(value);
}

}


#endif
