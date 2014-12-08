
// Copyright Victor Smirnov 2011-2014.
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




template <Int Idx, typename List, bool ReturnDefault = false, Int Counter = 0> struct SelectByIndexTool;

template <Int Idx, typename List, bool ReturnDefault = false>
using SelectByIndex = typename SelectByIndexTool<Idx, List, ReturnDefault>::Type;

template <Int idx> class ListIndexOutOfRange {};


template <Int Idx, bool ReturnDefault, typename Head, typename ... Tail>
struct SelectByIndexTool<Idx, TypeList<Head, Tail...>, ReturnDefault, Idx> {
    using Type = Head;
};

template <Int Idx, bool ReturnDefault, typename Head, typename ... Tail, Int Counter>
struct SelectByIndexTool<Idx, TypeList<Head, Tail...>, ReturnDefault, Counter> {
	using Type = typename SelectByIndexTool<
                        Idx,
                        TypeList<Tail...>,
                        ReturnDefault,
                        Counter + 1
                     >::Type;
};

template <Int Idx, Int Counter>
struct SelectByIndexTool<Idx, TypeList<>, false, Counter> {
	using Type = ListIndexOutOfRange<Idx>;
};

template <Int Idx, Int Counter>
struct SelectByIndexTool<Idx, TypeList<>, true, Counter> {
	using Type = NullType;
};


template <Int Idx, bool ReturnDefault, typename T, T Head, T ... Tail>
struct SelectByIndexTool<Idx, ValueList<T, Head, Tail...>, ReturnDefault, Idx> {
    static const T Value = Head;
};

template <Int Idx, bool ReturnDefault, typename T, T Head, T ... Tail, Int Counter>
struct SelectByIndexTool<Idx, ValueList<T, Head, Tail...>, ReturnDefault, Counter> {
    static const T Value = SelectByIndexTool<
                        Idx,
                        ValueList<T, Tail...>,
                        ReturnDefault,
                        Counter + 1
                     >::Value;
};

template <Int Idx, Int Counter, typename T>
struct SelectByIndexTool<Idx, ValueList<T>, false, Counter>;

template <Int Idx, Int Counter, typename T>
struct SelectByIndexTool<Idx, ValueList<T>, true, Counter> {
    static const T Value = 0;
};

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_INDEX_HPP */
