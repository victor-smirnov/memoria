
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <memoria/v1/prototypes/bt_tl/bttl_names.hpp>

namespace memoria {
namespace v1 {

template <typename Types, typename Value>
void AssignToItem(Iter<BTTLIterTypes<Types>>& iter, const Value& value)
{
    iter.setValue(value);
}

}}