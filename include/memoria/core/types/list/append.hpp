
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/list/typelist.hpp>

namespace memoria {

template <typename Item1, typename Item2> struct AppendTool;
template <typename List, typename ... Items> struct AppendToList;
template <typename List, typename ... Items> struct PrependToList;
template <typename T, T Item1, typename List> struct AppendValueTool;



template <typename Item, typename ... List>
struct AppendTool<Item, TypeList<List...> > {
    typedef TypeList<Item, List...>                                             Result;
};

template <typename ... List1, typename ... List2>
struct AppendTool<TypeList<List1...>, TypeList<List2...> > {
    typedef TypeList<List1..., List2...>                                        Result;
};

template <typename ... List, typename Item>
struct AppendTool<TypeList<List...>, Item > {
    typedef TypeList<List..., Item>                                             Result;
};



// ValueList
template <typename T, T Item, T ... List>
struct AppendValueTool<T, Item, ValueList<T, List...> > {
    typedef ValueList<T, Item, List...>                                         Result;
};

template <typename T, T ... List1, T ... List2>
struct AppendTool<ValueList<T, List1...>, ValueList<T, List2...> > {
    typedef ValueList<T, List1..., List2...>                                    Result;
};


template <typename ... List, typename ... Items>
struct AppendToList<TypeList<List...>, Items...> {
    typedef TypeList<List..., Items...>                                         Result;
};

template <typename ... List, typename ... Items>
struct PrependToList<TypeList<List...>, Items...> {
    typedef TypeList<Items..., List...>                                         Result;
};



// Errors:
template <typename ... List>
struct AppendTool<TypeList<List...>, NullType>;

template <typename ... List>
struct AppendTool<NullType, TypeList<List...>>;

template <>
struct AppendTool<NullType, NullType>;





namespace internal0 {

template <typename Accumulator, typename ... Lists> class MergeTypeListsHelper;

template <typename Accumulator, typename ... List, typename ... Tail>
class MergeTypeListsHelper<Accumulator, TypeList<List...>, Tail...> {
    typedef typename AppendTool<Accumulator, TypeList<List...> >::Result        R0;
public:
    typedef typename MergeTypeListsHelper<R0, Tail...>::Result                  Result;
};

template <typename Accumulator, typename Item, typename ... Tail>
class MergeTypeListsHelper<Accumulator, Item, Tail...> {
    typedef typename AppendTool<Accumulator, TypeList<Item> >::Result           R0;
public:
    typedef typename MergeTypeListsHelper<R0, Tail...>::Result                  Result;
};


template <typename Accumulator>
class MergeTypeListsHelper<Accumulator> {
public:
    typedef Accumulator                                                         Result;
};



template <typename Accumulator, typename ... Lists> class MergeValueListsHelper;

template <typename Accumulator, typename T, T... List, typename ... Tail>
class MergeValueListsHelper<Accumulator, ValueList<T, List...>, Tail...> {
    typedef typename AppendTool<Accumulator, ValueList<T, List...> >::Result    R0;
public:
    typedef typename MergeValueListsHelper<R0, Tail...>::Result                 Result;
};

template <typename Accumulator, typename T, T Value, typename ... Tail>
class MergeValueListsHelper<Accumulator, ConstValue<T, Value>, Tail...> {
    typedef typename AppendTool<Accumulator, ValueList<T, Value> >::Result    R0;
public:
    typedef typename MergeValueListsHelper<R0, Tail...>::Result                 Result;
};

template <typename Accumulator, typename ... Tail>
class MergeValueListsHelper {
public:
    typedef typename AppendTool<Accumulator, Tail... >::Result                 Result;
};



template <typename Accumulator>
class MergeValueListsHelper<Accumulator> {
public:
    typedef Accumulator                                                         Result;
};

}




template <typename ... Lists> struct MergeLists {
    typedef typename internal0::MergeTypeListsHelper<TypeList<>, Lists...>::Result   Result;
};


template <typename ... Lists> struct MergeValueLists;

template <
    typename T,
    T... List,
    typename... Tail
>
struct MergeValueLists<ValueList<T, List...>, Tail...> {
    typedef typename internal0::MergeValueListsHelper<ValueList<T, List...>, Tail...>::Result   Result;
};


template <
    typename T,
    T Value,
    T... List,
    typename... Tail
>
struct MergeValueLists<ValueList<T, List...>, ConstValue<T, Value>, Tail...> {
private:
    typedef typename internal0::MergeValueListsHelper<ValueList<T, List...>, ValueList<T, Value>>::Result R0;

public:
    typedef typename MergeValueLists<R0, Tail...>::Result   Result;
};


template <
    typename T,
    T Value,
    typename... Tail
>
struct MergeValueLists<ConstValue<T, Value>, Tail...> {
    typedef typename MergeValueLists<ValueList<T, Value>, Tail...>::Result   Result;
};


}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP */
