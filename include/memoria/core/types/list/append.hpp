
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP
#define	_MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP

#include <memoria/core/types/typelist.hpp>

namespace memoria {

template <typename Item, typename List> struct AppendTool;

template <typename Head, typename Tail, typename Item>
struct AppendTool<Item, TL<Head, Tail> > {
    typedef TL<Head, typename AppendTool<Item, Tail>::Result> Result;
};

template<>
struct AppendTool<NullType, NullType> {
    typedef NullType                                          Result;
};

template <typename Item>
struct AppendTool<Item, NullType> {
    typedef TL<Item, NullType>                                Result;
};

template <typename Head, typename Tail>
struct AppendTool<TL<Head, Tail>, NullType> {
    typedef TL<Head, Tail>                                    Result;
};

template <typename List, typename Item>
struct AppendLists {
	typedef typename AppendTool<Item, List>::Result			  Result;
};


}

#endif	/* _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP */
