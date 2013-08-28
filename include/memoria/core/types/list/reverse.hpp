
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_REVERSE_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_REVERSE_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/list/append.hpp>

namespace memoria    {

template <typename List> struct RevertList;

template <typename Head, typename ... Tail>
struct RevertList<TypeList<Head, Tail...>> {
    typedef typename AppendTool<
                typename RevertList<
                            TypeList<Tail...>
                         >::Type,
                Head
            >::Result                                                           Type;
};

template <>
struct RevertList<TypeList<>> {
    typedef TypeList<>                                                          Type;
};


}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP */
