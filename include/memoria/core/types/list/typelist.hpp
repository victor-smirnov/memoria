
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_TYPELIST_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_TYPELIST_HPP

#include <memoria/core/types/types.hpp>

namespace memoria    {

template <typename ... List> struct ListHead;

template <typename Head, typename ... Tail>
struct ListHead<TypeList<Head, Tail...>> {
	typedef Head Type;
};

template <typename ... List> struct ListTail;

template <typename Head, typename ... Tail>
struct ListTail<Head, Tail...> {
	typedef TypeList<Tail...> Type;
};

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_TYPELIST_HPP */
