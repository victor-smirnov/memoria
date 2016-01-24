
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BT_SUBSTREAMGROUP_DISPATCHER_HPP_
#define MEMORIA_BT_SUBSTREAMGROUP_DISPATCHER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <tuple>

namespace memoria   {
namespace bt        {

namespace detail {



}

template <typename List, Int Offset = 0, Int Idx = 0, Int Max = ListSize<List>::Value>
class BuildTopLevelLeafSubsetsH;

template <typename List, Int Offset = 0>
using BuildTopLevelLeafSubsets = typename BuildTopLevelLeafSubsetsH<List, Offset>::Type;



template <typename List, Int Offset, Int Idx, Int Max>
class BuildTopLevelLeafSubsetsH {
	static const Int LeafOffsetInf = memoria::list_tree::LeafCountInf<List, IntList<Idx>>::Value;
	static const Int LeafOffsetSup = memoria::list_tree::LeafCountSup<List, IntList<Idx>>::Value;
public:
	using Type = MergeLists<
			memoria::list_tree::MakeValueList<
				Int,
				LeafOffsetInf + Offset,
				LeafOffsetSup + Offset
			>,
			typename BuildTopLevelLeafSubsetsH<
				List,
				Offset,
				Idx + 1,
				Max
			>::Type
	>;
};


template <typename List, Int Offset, Int Max>
class BuildTopLevelLeafSubsetsH<List, Offset, Max, Max> {
public:
	using Type = TypeList<>;
};





template <typename Dispatcher, typename GroupList, Int GroupIdx = 0> struct GroupDispatcher;

template <
	typename Dispatcher,
	typename Group,
	typename... Tail,
	Int GroupIdx
>
struct GroupDispatcher<Dispatcher, TypeList<Group, Tail...>, GroupIdx>
{
	template <typename Allocator, typename Fn, typename... Args>
	static void dispatchGroups(Allocator* allocator, Fn&& fn, Args&&... args)
	{
		using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<Group, GroupIdx>;

		SubgroupDispatcher::dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...);

		GroupDispatcher<Dispatcher, TypeList<Tail...>, GroupIdx + 1>::dispatchGroups(
				allocator,
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);
	}


	template <typename Fn, typename... Args>
	static void dispatchGroupsStatic(Fn&& fn, Args&&... args)
	{
		using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<Group, GroupIdx>;

		SubgroupDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);

		GroupDispatcher<Dispatcher, TypeList<Tail...>, GroupIdx + 1>::dispatchGroupsStatic(
				std::forward<Fn>(fn),
				std::forward<Args>(args)...
		);
	}


	template <Int GroupIdx_, typename Allocator, typename Fn, typename... Args>
	static void dispatchGroup(Allocator* allocator, Fn&& fn, Args&&... args)
	{
		using TargetGroup = Select<GroupIdx_, TypeList<Group, Tail...>>;

		using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<TargetGroup, GroupIdx_>;

		SubgroupDispatcher::dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...);
	}

	template <Int GroupIdx_, typename Fn, typename... Args>
	static void dispatchGroupStatic(Fn&& fn, Args&&... args)
	{
		using TargetGroup = Select<GroupIdx_, TypeList<Group, Tail...>>;

		using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<TargetGroup, GroupIdx_>;

		SubgroupDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);
	}
};


template <
	typename Dispatcher,
	Int GroupIdx
>
struct GroupDispatcher<Dispatcher, TypeList<>, GroupIdx>
{
	template <typename Allocator, typename Fn, typename... Args>
	static void dispatchGroups(Allocator* allocator, Fn&& fn, Args&&... args) {}

	template <typename Fn, typename... Args>
	static void dispatchGroupsStatic(Fn&& fn, Args&&... args) {}
};







}
}
#endif
