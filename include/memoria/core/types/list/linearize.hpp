
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TYPES_LIST_LINEARIZE_HPP_
#define MEMORIA_CORE_TYPES_LIST_LINEARIZE_HPP_

#include <memoria/core/types/list/misc.hpp>
#include <memoria/core/types/algo/select.hpp>

namespace memoria {

template <typename List, Int Depth = 0>
struct ListDepth {
	static const Int Value = Depth;
};


template <typename Head, typename... Tail, Int Depth>
struct ListDepth<TypeList<Head, Tail...>, Depth> {
	static const Int Value = ListDepth<TypeList<Tail...>, Depth>::Value;
};

template <typename... Head, typename... Tail, Int Depth>
struct ListDepth<TypeList<TypeList<Head...>, Tail...>, Depth> {
	static const Int Value = Max<
			Int,
			ListDepth<TypeList<Head...>, Depth + 1>::Value,
			ListDepth<TypeList<Tail...>, Depth>::Value
	>::Value;
};


template <Int Depth>
struct ListDepth<TypeList<>, Depth> {
	static const Int Value = Depth + 1;
};



template <typename List> struct IsPlainList {
	static const bool Value = false;
};

template <typename... List>
struct IsPlainList<TypeList<List...>> {
	static const bool Value = ListDepth<TypeList<List...>>::Value == 1;
};






template <typename T, Int MaxDepth = 1>
struct LinearizeT {
    using Type = TypeList<T>;
};

template <typename T, Int MaxDepth = 1>
using Linearize = typename LinearizeT<T, MaxDepth>::Type;

// This case is not defined
template <typename T>
struct LinearizeT<T, 0>;


template <typename T, typename... Tail, Int MaxDepth>
struct LinearizeT<TypeList<T, Tail...>, MaxDepth> {
	using Type = typename IfThenElse<
    		ListDepth<T>::Value < MaxDepth,
    		MergeLists<
    			TypeList<T>,
    			typename LinearizeT<TypeList<Tail...>, MaxDepth>::Type
    		>,
    		MergeLists<
    			typename LinearizeT<T, MaxDepth>::Type,
    			typename LinearizeT<TypeList<Tail...>, MaxDepth>::Type
    		>
    >::Result;
};


template <Int MaxDepth>
struct LinearizeT<TypeList<>, MaxDepth> {
    using Type = TypeList<>;
};




}

#endif
