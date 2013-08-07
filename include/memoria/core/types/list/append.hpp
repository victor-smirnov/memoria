
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
    typedef TypeList<Item, List...>                                     Result;
};

template <typename ... List1, typename ... List2>
struct AppendTool<TypeList<List1...>, TypeList<List2...> > {
    typedef TypeList<List1..., List2...>                                Result;
};

template <typename ... List, typename Item>
struct AppendTool<TypeList<List...>, Item > {
    typedef TypeList<List..., Item>                                     Result;
};



// ValueList
template <typename T, T Item, T ... List>
struct AppendValueTool<T, Item, ValueList<T, List...> > {
    typedef ValueList<T, Item, List...>                                 Result;
};

template <typename T, T ... List1, T ... List2>
struct AppendTool<ValueList<T, List1...>, ValueList<T, List2...> > {
    typedef ValueList<T, List1..., List2...>                            Result;
};


template <typename ... List, typename ... Items>
struct AppendToList<TypeList<List...>, Items...> {
    typedef TypeList<List..., Items...>                                 Result;
};

template <typename ... List, typename ... Items>
struct PrependToList<TypeList<List...>, Items...> {
    typedef TypeList<Items..., List...>                                 Result;
};



// Errors:
template <typename ... List>
struct AppendTool<TypeList<List...>, NullType>;

template <typename ... List>
struct AppendTool<NullType, TypeList<List...>>;

template <>
struct AppendTool<NullType, NullType>;





namespace internal0 {
template <typename Accumulator, typename ... Lists> class MergeListsHelper;

template <typename Accumulator, typename ... List, typename ... Tail>
class MergeListsHelper<Accumulator, TypeList<List...>, Tail...> {
    typedef typename AppendTool<Accumulator, TypeList<List...> >::Result        R0;
public:
    typedef typename MergeListsHelper<R0, Tail...>::Result                      Result;
};

template <typename Accumulator, typename Item, typename ... Tail>
class MergeListsHelper<Accumulator, Item, Tail...> {
    typedef typename AppendTool<Accumulator, TypeList<Item> >::Result           R0;
public:
    typedef typename MergeListsHelper<R0, Tail...>::Result                      Result;
};

template <typename Accumulator>
class MergeListsHelper<Accumulator> {
public:
    typedef Accumulator                                                         Result;
};

}

template <typename ... Lists> struct MergeLists {
    typedef typename internal0::MergeListsHelper<TypeList<>, Lists...>::Result   Result;
};


}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP */
