
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_REMOVE_HPP
#define	_MEMORIA_CORE_TOOLS_TYPES_LIST_REMOVE_HPP

#include <memoria/core/types/list/typelist.hpp>

namespace memoria    {

template <typename Item, typename List, bool All = false> struct RemoveTool;

template <typename Item, typename Head, typename Tail, bool All>
struct RemoveTool<Item, TL<Head, Tail>, All> {
    typedef TL<Head, typename RemoveTool<Item, Tail>::Result>                 Result;
};

template <typename Item, typename Tail>
struct RemoveTool<Item, TL<Item, Tail>, false> {
    typedef Tail                                                                Result;
};

template <typename Item, typename Tail>
struct RemoveTool<Item, TL<Item, Tail>, true> {
    typedef typename RemoveTool<Item, Tail, true>::Result                           Result;
};


template <typename Item, bool All>
struct RemoveTool<Item, NullType, All> {
    typedef NullType                                                            Result;
};


template <typename List> struct RemoveDuplicatesTool;

template<>
struct RemoveDuplicatesTool<NullType> {
    typedef NullType                                                            Result;
};

template <typename Head, typename Tail>
struct RemoveDuplicatesTool<TL<Head, Tail> > {
private:
    typedef typename RemoveDuplicatesTool<Tail>::Result                         TailResult;
    typedef typename RemoveTool<Head, TailResult>::Result                       HeadResult;
public:
    typedef TL<Head, HeadResult>                                          Result;
};


}

#endif	/* _MEMORIA_CORE_TOOLS_TYPES_LIST_REMOVE_HPP */
