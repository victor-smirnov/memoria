
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_REMOVE_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_REMOVE_HPP

#include <memoria/core/types/list/typelist.hpp>

namespace memoria    {

template <typename Item, typename List, bool All = false> struct RemoveTool;

template <typename Item, typename Head, typename ... Tail, bool All>
struct RemoveTool<Item, VTL<Head, Tail...>, All> {
    typedef typename AppendTool<
    					Head,
    					typename RemoveTool<
    								Item,
    								VTL<Tail...>,
    								All
    					>::Result
    				>::Result 													Result;
};

template <typename Item, typename ... Tail>
struct RemoveTool<Item, VTL<Item, Tail...>, false> {
    typedef VTL<Tail...>														Result;
};

template <typename Item, typename ... Tail>
struct RemoveTool<Item, VTL<Item, Tail... >, true> {
    typedef typename RemoveTool<
    					Item,
    					VTL<Tail...>,
    					true
    				>::Result                       							Result;
};


template <typename Item, bool All>
struct RemoveTool<Item, VTL<>, All> {
    typedef VTL<>                                                            	Result;
};




template <typename List> struct RemoveDuplicatesTool;


template<>
struct RemoveDuplicatesTool<VTL<>> {
    typedef VTL<>                                                            	Result;
};



template <typename Head, typename ... Tail>
struct RemoveDuplicatesTool<VTL<Head, Tail...> > {
private:
    typedef typename RemoveDuplicatesTool<VTL<Tail...>>::Result	TailResult;
    typedef typename RemoveTool<Head, TailResult>::Result		HeadResult;
public:
    typedef typename AppendTool<Head, HeadResult>::Result                       Result;
};





}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_REMOVE_HPP */
