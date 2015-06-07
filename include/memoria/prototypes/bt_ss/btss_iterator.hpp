
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_ITERATOR_HPP_
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_ITERATOR_HPP_


#include <memoria/prototypes/bt_ss/btss_names.hpp>

namespace memoria {

template <typename Types, typename Value>
void AssignToItem(Iter<BTSSIterTypes<Types>>& iter, const Value& value)
{
    iter.setValue(value);
}

}


#endif
