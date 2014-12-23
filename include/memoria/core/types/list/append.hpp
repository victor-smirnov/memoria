
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
    using Type = TypeList<Item, List...>;
};

template <typename ... List1, typename ... List2>
struct AppendTool<TypeList<List1...>, TypeList<List2...> > {
	using Type = TypeList<List1..., List2...>;
};

template <typename ... List, typename Item>
struct AppendTool<TypeList<List...>, Item > {
	using Type = TypeList<List..., Item>;
};



// ValueList
template <typename T, T Item, T ... List>
struct AppendValueTool<T, Item, ValueList<T, List...> > {
	using Type = ValueList<T, Item, List...>;
};

template <typename T, T ... List1, T ... List2>
struct AppendTool<ValueList<T, List1...>, ValueList<T, List2...> > {
	using Type = ValueList<T, List1..., List2...>;
};


template <typename ... List, typename ... Items>
struct AppendToList<TypeList<List...>, Items...> {
	using Type = TypeList<List..., Items...>;
};

template <typename ... List, typename ... Items>
struct PrependToList<TypeList<List...>, Items...> {
	using Type = TypeList<Items..., List...>;
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
    using R0 = typename AppendTool<Accumulator, TypeList<List...> >::Type;
public:
    using Type = typename MergeTypeListsHelper<R0, Tail...>::Type;
};

template <typename Accumulator, typename Item, typename ... Tail>
class MergeTypeListsHelper<Accumulator, Item, Tail...> {
    using R0 = typename AppendTool<Accumulator, TypeList<Item> >::Type;
public:
    using Type = typename MergeTypeListsHelper<R0, Tail...>::Type;
};


template <typename Accumulator>
class MergeTypeListsHelper<Accumulator> {
public:
	using Type = Accumulator;
};



template <typename Accumulator, typename ... Lists> class MergeValueListsHelper;

template <typename Accumulator, typename T, T... List, typename ... Tail>
class MergeValueListsHelper<Accumulator, ValueList<T, List...>, Tail...> {
	using R0 = typename AppendTool<Accumulator, ValueList<T, List...> >::Type;
public:
	using Type = typename MergeValueListsHelper<R0, Tail...>::Type;
};

template <typename Accumulator, typename T, T Value, typename ... Tail>
class MergeValueListsHelper<Accumulator, ConstValue<T, Value>, Tail...> {
	using R0 = typename AppendTool<Accumulator, ValueList<T, Value> >::Type;
public:
	using Type = typename MergeValueListsHelper<R0, Tail...>::Type;
};

template <typename Accumulator, typename ... Tail>
class MergeValueListsHelper {
public:
	using Type = typename AppendTool<Accumulator, Tail... >::Type;
};



template <typename Accumulator>
class MergeValueListsHelper<Accumulator> {
public:
	using Type = Accumulator;
};

}




template <typename ... Lists> struct MergeListsMF {
	using Type = typename internal0::MergeTypeListsHelper<TypeList<>, Lists...>::Type;
};

template <typename ... Lists>
using MergeLists = typename MergeListsMF<Lists...>::Type;


template <typename ... Lists> struct MergeValueLists;

template <typename ... Lists>
using MergeValueListsT = typename MergeValueLists<Lists...>::Type;

template <
    typename T,
    T... List,
    typename... Tail
>
struct MergeValueLists<ValueList<T, List...>, Tail...> {
	using Type = typename internal0::MergeValueListsHelper<ValueList<T, List...>, Tail...>::Type;
};


template <
    typename T,
    T Value,
    T... List,
    typename... Tail
>
struct MergeValueLists<ValueList<T, List...>, ConstValue<T, Value>, Tail...> {
private:
	using R0 = typename internal0::MergeValueListsHelper<ValueList<T, List...>, ValueList<T, Value>>::Type;

public:
	using Type = typename MergeValueLists<R0, Tail...>::Type;
};


template <
    typename T,
    T Value,
    typename... Tail
>
struct MergeValueLists<ConstValue<T, Value>, Tail...> {
	using Type = typename MergeValueLists<ValueList<T, Value>, Tail...>::Type;
};




template <typename List, typename Item, Int Idx> struct ReplaceH;

template <typename List, typename Item, Int Idx>
using Replace = typename ReplaceH<List, Item, Idx>::Type;

template <typename Head, typename... Tail, typename Item>
struct ReplaceH<TypeList<Head, Tail...>, Item, 0> {
	using Type = TypeList<Item, Tail...>;
};

template <typename Head, typename... Tail, typename Item, Int Idx>
struct ReplaceH<TypeList<Head, Tail...>, Item, Idx> {
	using Type = MergeLists<
			TypeList<Head>,
			typename ReplaceH<TypeList<Tail...>, Item, Idx - 1>::Type
	>;
};



}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP */
