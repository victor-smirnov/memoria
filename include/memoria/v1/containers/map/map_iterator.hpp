
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/containers/map/map_names.hpp>
#include <memoria/v1/prototypes/ctr_wrapper/iterator.hpp>

namespace memoria {
namespace v1 {

template <typename Types, typename Value>
void AssignToItem(Iter<MapIterTypes<Types>>& iter, Value&& value)
{
    iter.setValue(value);
}

}}