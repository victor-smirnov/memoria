
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BT_ACCUMULATOR_HANDLERS_HPP_
#define MEMORIA_BT_ACCUMULATOR_HANDLERS_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/prototypes/bt/tools/bt_size_list_builder.hpp>

#include <tuple>

namespace memoria 	{
namespace bt 		{

namespace detail {

template <typename T, Int Idx> struct AccumulatorLeafItemHandler;

template <
	Int Head,
	Int... Tail,
	Int Idx
>
struct AccumulatorLeafItemHandler<IntList<Head, Tail...>, Idx>
{
	template <typename TupleItem, typename Fn, typename... Args>
	static void process(TupleItem&& tuple_item, Fn&& fn, Args&&... args)
	{
		fn.template substream<Idx, Head>(
				std::forward<TupleItem>(tuple_item),
				std::forward<Args>(args)...
		);

		AccumulatorLeafItemHandler<
			IntList<Tail...>,
			Idx + 1
		>
		::process(
				std::forward<TupleItem>(tuple_item),
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);
	}
};


template <
	Int Head,
	Int... Tail,
	Int Idx
>
struct AccumulatorLeafItemHandler<::memoria::bt::StreamStartTag<IntList<Head, Tail...>>, Idx>
{
	template <typename TupleItem, typename Fn, typename... Args>
	static void process(TupleItem&& tuple_item, Fn&& fn, Args&&... args)
	{
		fn.template streamStart<Idx, Head>(
				std::forward<TupleItem>(tuple_item),
				std::forward<Args>(args)...
		);

		AccumulatorLeafItemHandler<
			IntList<Tail...>,
			Idx + 1
		>
		::process(
				std::forward<TupleItem>(tuple_item),
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);
	}
};


template <
	Int Idx
>
struct AccumulatorLeafItemHandler<IntList<>, Idx>
{
	template <typename TupleItem, typename Fn, typename... Args>
	static void process(TupleItem&& tuple_item, Fn&& fn, Args&&... args){}
};


template <typename List>
struct ItemSize {
	static const Int Value = ListSize<List>::Value;
};

template <typename List>
struct ItemSize<StreamStartTag<List>> {
	static const Int Value = ListSize<List>::Value;
};

}


template <
	typename Tuple,
	typename List,
	Int AccumulatorIdx 	= 0,
	Int TotalIdx 		= 0
>
struct AccumulatorLeafHandler;

template <
	typename Head,
	typename... Tail,
	typename List,
	Int AccumulatorIdx,
	Int TotalIdx
>
struct AccumulatorLeafHandler<std::tuple<Head, Tail...>, List, AccumulatorIdx, TotalIdx>
{
	template <typename Tuple, typename Fn, typename... Args>
	static void process(Tuple&& tuple, Fn&& fn, Args&&... args)
	{
		using ListElement = typename Select<AccumulatorIdx, List>::Result;

		detail::AccumulatorLeafItemHandler<ListElement, TotalIdx>::process(
				std::get<AccumulatorIdx>(tuple),
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);

		AccumulatorLeafHandler<
			std::tuple<Tail...>,
			List,
			AccumulatorIdx + 1,
			TotalIdx + detail::ItemSize<ListElement>::Value
		>
		::process(
				std::forward<Tuple>(tuple),
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);
	}
};


template <Int AccumulatorIdx, Int TotalIdx, typename List>
struct AccumulatorLeafHandler<std::tuple<>, List, AccumulatorIdx, TotalIdx>
{
	template <typename Fn, typename... Args>
	static void process(Fn&& fn, Args&&... args){}
};



}
}



#endif
