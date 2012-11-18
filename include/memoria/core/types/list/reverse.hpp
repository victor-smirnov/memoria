
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_REVERSE_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_REVERSE_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/list/append.hpp>

namespace memoria    {

template <typename List> struct ReverseTool;

template <typename Head, typename Tail>
struct ReverseTool<TL<Head, Tail> > {
    typedef typename appendTool<Head, typename ReverseTool<Tail>::Result>::Result  Result;
};

template<typename Head>
struct ReverseTool<TL<Head, NullType> > {
    typedef TL<Head>                                          Result;
};

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP */
