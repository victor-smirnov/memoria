
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
namespace detail  {

template <typename Accumulator, typename IdxList> struct BranchAccumWaker1;
template <typename Accumulator, typename IdxList> struct BranchAccumWalker2;

template <
	typename Accumulator,
	Int Idx,
	Int... Tail
>
struct BranchAccumWalker2<Accumulator, IntList<Idx, Tail...>> {

	template <typename Walker, typename Node, typename... Args>
	static void process(Walker& walker, const Node* node, Accumulator& accum, Args&&... args)
	{
		BranchAccumWalker2<Accumulator, IntList<Idx, Tail...>> w;

		node->template processStreamByIdx<Idx>(w, std::get<Idx>(accum), walker, std::forward<Args>(args)...);

		BranchAccumWalker2<Accumulator, IntList<Tail...>>::process(walker, node, accum, std::forward<Args>(args)...);
	}

	template <
		typename StreamObj,
		typename T,
		typename Walker,
		typename... Args
	>
	void stream(const StreamObj* obj, T& item, Walker& walker, Args&&... args)
	{
		walker.branch_iterator_accumulator(obj, item, std::forward<Args>(args)...);
	}
};


template <
	typename Accumulator
>
struct BranchAccumWalker2<Accumulator, IntList<>> {

	template <typename Walker, typename Node, typename... Args>
	static void process(Walker& walker, const Node* node, Accumulator& accum, Args&&... args)
	{}
};



template <
	typename Accumulator,
	Int Idx,
	Int... Tail
>
struct BranchAccumWaker1<Accumulator, IntList<Idx, Tail...>> {

	template <typename Walker, typename Node, typename... Args>
	static void process(Walker& walker, const Node* node, Accumulator& accum, Args&&... args)
	{
		using ItemType 		= typename std::tuple_element<Idx, Accumulator>::type;
		using RangeIdxList 	= memoria::list_tree::MakeValueList<Int, 0, std::tuple_size<ItemType>::value>;

		BranchAccumWalker2<ItemType, RangeIdxList>::process(walker, node, std::get<Idx>(accum), std::forward<Args>(args)...);

		BranchAccumWaker1<Accumulator, IntList<Tail...>>::process(walker, node, accum, std::forward<Args>(args)...);
	}
};


template <
	typename Accumulator
>
struct BranchAccumWaker1<Accumulator, IntList<>> {

	template <typename Walker, typename Node, typename... Args>
	static void process(Walker& walker, const Node* node, Accumulator& accum, Args&&... args)
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

	template <typename StreamObj, typename Walker, typename Accum, typename... Args>
	static void process(const StreamObj* obj, Walker& walker, Accum& accum, Args&&... args)
	{
		auto& item = AccumItemH::template item<Offset>(accum);

		walker.template leaf_iterator_accumulator<Offset, From, To - From>(obj, item, std::forward<Args>(args)...);

		LeafIndexRangeWalker<AccumItemH, TL<Tail...>, IntList<RTail...>>::process(obj, walker, accum, std::forward<Args>(args)...);
	}
};


template <
	typename AccumItemH
>
struct LeafIndexRangeWalker<AccumItemH, TL<>, IntList<>>{
	template <typename StreamObj, typename Walker, typename Accum, typename... Args>
	static void process(const StreamObj* obj, Walker& walker, Accum& accum, Args&&... args)
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
	template <Int Idx, typename StreamObj, typename Walker, typename Accum, typename... Args>
	void stream(const StreamObj* obj, Walker& walker, Accum& accum, Args&&... args)
	{
		constexpr Int SubstreamIdx = StreamIdx + Idx;

		using LeafPath   = typename memoria::list_tree::BuildTreePath<LeafStructList, SubstreamIdx>::Type;
		using AccumItemH = memoria::bt::AccumItem<LeafStructList, LeafPath, Accum>;

		using RangeList = typename Select<SubstreamIdx, Linearize<LeafRangeList, 2>>::Result;
		using RangeOffsetList = typename Select<SubstreamIdx, Linearize<LeafRangeOffsetList>>::Result;

		LeafIndexRangeWalker<AccumItemH, RangeList, RangeOffsetList>::process(obj, walker, accum, std::forward<Args>(args)...);
	}
};

}

}
}

#endif
