
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_BASE_DETAIL_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_BASE_DETAIL_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/prototypes/bt/walkers/bt_walker_tools.hpp>


#include <tuple>

namespace memoria {
namespace bt1     {
namespace detail {

template <typename Accumulator, typename IdxList> struct BranchAccumWaker1;
template <typename Accumulator, typename IdxList> struct BranchAccumWalker2;

template <
	typename Accumulator,
	Int Idx,
	Int... Tail
>
struct BranchAccumWalker2<Accumulator, IntList<Idx, Tail...>> {

	template <typename Node>
	static void process(const Node* node, Accumulator& accum, Int start, Int end)
	{
		BranchAccumWalker2<Accumulator, IntList<Idx, Tail...>> w;

		node->template processStreamByIdx<Idx>(w, std::get<Idx>(accum), start, end);

		BranchAccumWalker2<Accumulator, IntList<Tail...>>::process(node, accum, start, end);
	}

	template <
		typename StreamObj,
		typename T,
		Int From,
		Int To,
		template <typename, Int, Int> class IterAccumItem
	>
	void stream(const StreamObj* obj, IterAccumItem<T, From, To>& item, Int start, Int end)
	{
		static_assert(To <= StreamObj::Indexes, "Invalid BTree structure");

		for (Int c = 0; c < StreamObj::Indexes; c++)
		{
			item[c + From] += obj->sum(c, start, end);
		}
	}

	template <
		typename StreamObj,
		typename T,
		template <typename> class AccumItem
	>
	void stream(const StreamObj* obj, AccumItem<T>& item, Int start, Int end)
	{
	}
};


template <
	typename Accumulator
>
struct BranchAccumWalker2<Accumulator, IntList<>> {

	template <typename Node>
	static void process(const Node* node, Accumulator& accum, Int start, Int end)
	{}
};



template <
	typename Accumulator,
	Int Idx,
	Int... Tail
>
struct BranchAccumWaker1<Accumulator, IntList<Idx, Tail...>> {

	template <typename Node>
	static void process(const Node* node, Accumulator& accum, Int start, Int end)
	{
		using ItemType 		= typename std::tuple_element<Idx, Accumulator>::type;
		using RangeIdxList 	= memoria::list_tree::MakeValueList<Int, 0, std::tuple_size<ItemType>::value>;

		BranchAccumWalker2<ItemType, RangeIdxList>::process(node, std::get<Idx>(accum), start, end);

		BranchAccumWaker1<Accumulator, IntList<Tail...>>::process(node, accum, start, end);
	}
};


template <
	typename Accumulator
>
struct BranchAccumWaker1<Accumulator, IntList<>> {

	template <typename Node>
	static void process(const Node* node, Accumulator& accum, Int start, Int end)
	{}
};


template <typename AccumItemH, typename RangeList, typename RangeOffsetList> struct LeafIndexRangeWalker;

template <
	typename AccumItemH,
	Int From,
	Int To,
	Int Offset,
	typename... Tail,
	Int... RTail
>
struct LeafIndexRangeWalker<AccumItemH, TL<memoria::bt::IndexRange<From, To>, Tail...>, IntList<Offset, RTail...>> {

	template <typename StreamObj, typename Accum>
	static void process(const StreamObj* obj, Accum& accum, Int start, Int end)
	{
		auto& item = AccumItemH::template item<Offset>(accum);

		for (Int c = 0; c < To - From; c++)
		{
			item[Offset - std::remove_reference<decltype(item)>::type::From + c] += obj->sum(c + From, start, end);
		}

		LeafIndexRangeWalker<AccumItemH, TL<Tail...>, IntList<RTail...>>::process(obj, accum, start, end);
	}
};


template <
	typename AccumItemH
>
struct LeafIndexRangeWalker<AccumItemH, TL<>, IntList<>>{
	template <typename StreamObj, typename Accum>
	static void process(const StreamObj* obj, Accum& accum, Int start, Int end)
	{
	}
};


template <
	typename LeafStructList,
	typename LeafRangeList,
	typename LeafRangeOffsetList,
	Int StreamIdx
>
struct LeafAccumWalker {
	template <Int Idx, typename StreamObj, typename Accum>
	void stream(const StreamObj* obj, Accum& accum, Int start, Int end)
	{
		constexpr Int SubstreamIdx = StreamIdx + Idx;

		using LeafPath   = typename memoria::list_tree::BuildTreePath<LeafStructList, SubstreamIdx>::Type;
		using AccumItemH = memoria::bt::AccumItem<LeafStructList, LeafPath, Accum>;

		using RangeList = typename Select<SubstreamIdx, Linearize<LeafRangeList, 2>>::Result;
		using RangeOffsetList = typename Select<SubstreamIdx, Linearize<LeafRangeOffsetList>>::Result;

		LeafIndexRangeWalker<AccumItemH, RangeList, RangeOffsetList>::process(obj, accum, start, end);
	}
};

}

}
}

#endif
