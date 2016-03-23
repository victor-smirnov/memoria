
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/ctr_wrapper/iterator.hpp>
#include <memoria/v1/containers/table/table_names.hpp>

namespace memoria {

template <typename Types, typename Value>
void AssignToItem(Iter<TableIterTypes<Types>>& iter, Value&& value)
{
    iter.setValue(value);
}

}
