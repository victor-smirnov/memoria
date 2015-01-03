
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_TYPES_LIST_LIST_TREE_HPP1_
#define MEMORIA_CORE_TYPES_LIST_LIST_TREE_HPP1_

#include <memoria/core/types/list/typelist.hpp>
#include <memoria/core/types/list/linearize.hpp>
#include <memoria/core/types/list/sublist.hpp>
#include <memoria/core/types/algo/select.hpp>

namespace memoria   {
namespace list_tree {


template <typename List, typename Path, Int Depth = 1, Int Ptr = 0> struct LeafCount;

template <
    typename Head,
    typename... Tail,
    Int Idx,
    Int... PathTail,
    Int Depth,
    Int Ptr
>
struct LeafCount<TypeList<Head, Tail...>, IntList<Idx, PathTail...>, Depth, Ptr> {
    static const Int PrefixSize = ListSize<Linearize<TypeList<Head>, Depth>>::Value;
    static const Int Value = PrefixSize + LeafCount<
        TypeList<Tail...>,
        IntList<Idx, PathTail...>,
        Depth,
        Ptr + 1
    >::Value;
};


template <
    typename Head,
    typename... Tail,
    Int Idx,
    Int... PathTail,
    Int Depth
>
struct LeafCount<TypeList<Head, Tail...>, IntList<Idx, PathTail...>, Depth, Idx> {
    static const Int Value = LeafCount<
        Head,
        IntList<PathTail...>, Depth
    >::Value;
};



template <
    typename Head,
    typename... Tail,
    Int Depth,
    Int Idx
>
struct LeafCount<TypeList<Head, Tail...>, IntList<>, Depth, Idx> {
    static const Int Value = LeafCount<
        TypeList<Head, Tail...>,
        IntList<0>, Depth
    >::Value;
};


template <
    typename T,
    Int Depth,
    Int Idx
>
struct LeafCount<T, IntList<>, Depth, Idx> {
    static const Int Value = 0;
};


template <typename List, typename Path, Int Ptr = 0> struct Subtree;

template <
    typename Head,
    typename... Tail,
    Int Idx,
    Int... PathTail,
    Int Ptr
>
struct Subtree<TypeList<Head, Tail...>, IntList<Idx, PathTail...>, Ptr> {
    using Type = typename Subtree<
        TypeList<Tail...>,
        IntList<Idx, PathTail...>,
        Ptr + 1
    >::Type;
};


template <
    typename Head,
    typename... Tail,
    Int Idx,
    Int... PathTail
>
struct Subtree<TypeList<Head, Tail...>, IntList<Idx, PathTail...>, Idx> {
    using Type = typename IfThenElse<
    		(sizeof...(PathTail) > 0),
    		typename Subtree<Head, IntList<PathTail...>>::Type,
    		Head
    >::Result;
};

template <
    typename Head,
    typename... Tail,
    Int Idx
>
struct Subtree<TypeList<Head, Tail...>, IntList<>, Idx> {
    using Type = TypeList<Head, Tail...>;
};


template <
    typename T,
    Int Idx
>
struct Subtree<T, IntList<>, Idx> {
    using Type = T;
};


template <typename List, typename Path = IntList<>, Int Depth = 1>
using SubtreeLeafCount = ListSize<
							Linearize<
							typename Subtree<List, Path>::Type,
							Depth
							>
						  >;



template <typename List, typename Path, Int Depth = 1>
using LeafCountInf = LeafCount<List, Path, Depth>;

template <typename List, typename Path, Int Depth = 1>
using LeafCountSup = IntValue<LeafCount<List, Path, Depth>::Value + SubtreeLeafCount<List, Path, Depth>::Value>;





template <typename T, T From, T To> struct MakeValueListH;

template <typename T, T From, T To>
struct MakeValueListH {
	using Type = AppendItemToList<
			ConstValue<T, From>,
			typename MakeValueListH<T, From + 1, To>::Type
	>;
};

template <typename T, T To>
struct MakeValueListH<T, To, To> {
	using Type = ValueList<T>;
};



template <typename T, T From, T To>
using MakeValueList = typename MakeValueListH<T, From, To>::Type;


template <typename List, Int LeafIdx, typename Path = IntList<>, Int Idx = 0> struct BuildTreePath;

namespace detail {

template <bool Condition, typename List, Int LeafIdx, typename Path, Int Idx> struct BuildTreePathHelper1;
template <bool Condition, typename List, Int LeafIdx, typename Path, Int Idx> struct BuildTreePathHelper2;

template <typename List, Int LeafIdx, Int... Path, Int Idx>
struct BuildTreePathHelper1<true, List, LeafIdx, IntList<Path...>, Idx> {
	using Type = typename MergeValueLists<IntList<Path...>, IntList<Idx>>::Type;
};

template <typename List, Int LeafIdx, Int... Path, Int Idx>
struct BuildTreePathHelper1<false, List, LeafIdx, IntList<Path...>, Idx> {
	using Type = typename BuildTreePath<
			List,
			LeafIdx - 1,
			IntList<Path...>,
			Idx + 1
	>::Type;
};


template <typename... Head, typename... Tail, Int LeafIdx, Int... Path, Int Idx>
struct BuildTreePathHelper2<true, TypeList<TypeList<Head...>, Tail...>, LeafIdx, IntList<Path...>, Idx> {
	using Type = typename BuildTreePath<
			TypeList<Head...>,
			LeafIdx,
			typename MergeValueLists<IntList<Path...>, IntList<Idx>>::Type,
			0
	>::Type;
};

template <typename... Head, typename... Tail, Int LeafIdx, Int... Path, Int Idx>
struct BuildTreePathHelper2<false, TypeList<TypeList<Head...>, Tail...>, LeafIdx, IntList<Path...>, Idx> {
	static const Int SubtreeSize = ListSize<Linearize<TypeList<Head...>>>::Value;

	using Type = typename BuildTreePath<
			TypeList<Tail...>,
			LeafIdx - SubtreeSize,
			IntList<Path...>,
			Idx + 1
	>::Type;
};


}


template <
	typename... Head,
	typename... Tail,
	Int... PathList,
	Int LeafIdx,
	Int Idx
>
struct BuildTreePath<TypeList<TypeList<Head...>, Tail...>, LeafIdx, IntList<PathList...>, Idx> {
	static const Int SubtreeSize = ListSize<Linearize<TypeList<Head...>>>::Value;

	using Type = typename detail::BuildTreePathHelper2<
			(LeafIdx < SubtreeSize),
			TypeList<TypeList<Head...>, Tail...>,
			LeafIdx,
			IntList<PathList...>,
			Idx
	>::Type;
};


template <
	typename Head,
	typename... Tail,
	Int... PathList,
	Int LeafIdx,
	Int Idx
>
struct BuildTreePath<TypeList<Head, Tail...>, LeafIdx, IntList<PathList...>, Idx> {
	using Type = typename detail::BuildTreePathHelper1<
			(LeafIdx < 1),
			TypeList<Tail...>,
			LeafIdx,
			IntList<PathList...>,
			Idx
	>::Type;
};



template <
	Int... PathList,
	Int LeafIdx,
	Int Idx
>
struct BuildTreePath<TypeList<>, LeafIdx, IntList<PathList...>, Idx>;

}
}

#endif
