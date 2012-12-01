
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_INDEX_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_INDEX_HPP

#include <memoria/core/types/typelist.hpp>

namespace memoria    {

template <typename Item_> struct DefaultItemProvider {
    typedef Item_ Item;
};

template <typename Item, typename List, Int Idx = 0> struct IndexOfTool;

template <typename Item, typename ... Tail, Int Idx>
struct IndexOfTool<Item, TypeList<Item, Tail...>, Idx> {
    static const Int Value = Idx;
};

template <typename Item, typename Head, typename ... Tail, Int Idx>
struct IndexOfTool<Item, TypeList<Head, Tail...>, Idx> {
    static const Int Value = IndexOfTool<Item, TypeList<Tail...>, Idx + 1>::Value;
};

template <typename Item, Int Idx>
struct IndexOfTool<Item, TypeList<>, Idx> {
    static const Int Value = -1; //not found
};




template <Int Idx, typename List, Int Counter = 0> struct SelectByIndexTool;

template <Int Value> struct ListIndexOutOfRange {};

template <Int Idx, typename Head, typename ... Tail>
struct SelectByIndexTool<Idx, TypeList<Head, Tail...>, Idx> {
    typedef Head                                                                Result;
};

template <Int Idx, typename Head, typename ... Tail, Int Counter>
struct SelectByIndexTool<Idx, TypeList<Head, Tail...>, Counter> {
    typedef typename SelectByIndexTool<Idx, TypeList<Tail...>, Counter + 1>::Result  Result;
};

template <Int Idx, Int Counter>
struct SelectByIndexTool<Idx, TypeList<>, Counter> {
    typedef ListIndexOutOfRange<Idx>                                            Result;
};

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_INDEX_HPP */
