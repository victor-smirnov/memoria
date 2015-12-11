
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_ALGO_FOLD_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_ALGO_FOLD_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/typelist.hpp>

namespace memoria    {


template <typename List, template <typename, typename> class FoldingFn, typename State = EmptyType> struct FoldTLRight;

template <typename Head, typename... Tail, template <typename, typename> class FoldingFn, typename State>
struct FoldTLRight<TL<Head, Tail...>, FoldingFn, State>
{
	using NewState 	= FoldingFn<State, Head>;
	using Type 		= typename FoldTLRight<TL<Tail...>, FoldingFn, NewState>::Type;
};

template <typename Element, template <typename, typename> class FoldingFn, typename State>
struct FoldTLRight<TL<Element>, FoldingFn, State>
{
	using Type = FoldingFn<State, Element>;
};

template <template <typename, typename> class FoldingFn, typename State>
struct FoldTLRight<TL<>, FoldingFn, State>
{
	using Type = FoldingFn<State, EmptyType>;
};



template <typename List, template <typename, typename> class FoldingFn> struct FoldTLLeft;

template <typename Head, typename... Tail, template <typename, typename> class FoldingFn>
struct FoldTLLeft<TL<Head, Tail...>, FoldingFn>
{
	using Type = FoldingFn<typename FoldTLLeft<TL<Tail...>, FoldingFn>::Type, Head>;
};

template <typename Head, template <typename, typename> class FoldingFn>
struct FoldTLLeft<TL<Head>, FoldingFn>
{
	using Type = FoldingFn<EmptyType, Head>;
};

template <template <typename, typename> class FoldingFn>
struct FoldTLLeft<TL<>, FoldingFn>
{
	using Type = FoldingFn<EmptyType, EmptyType>;
};


}

#endif
