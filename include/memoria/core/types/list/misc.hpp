
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_MISC_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_MISC_HPP

#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/types.hpp>

namespace memoria    {


template <typename List, typename Default> struct SelectHeadIfNotEmpty;

template <typename Head, typename ... Tail, typename Default>
struct SelectHeadIfNotEmpty<VTL<Head, Tail...>, Default> {
    typedef Head                                                                Result;
};

template <typename Default>
struct SelectHeadIfNotEmpty<VTL<>, Default> {
    typedef Default                                                             Result;
};


template <typename List> struct ListSize;

template <typename ... List>
struct ListSize<VTL<List...> > {
    static const Int Value = sizeof...(List);
};


}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_TYPECHAIN_HPP */
