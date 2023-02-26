
// Copyright 2014-2022 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/core/types.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_size_list_builder.hpp>

#include <memoria/core/tools/result.hpp>

#include <tuple>

namespace memoria {
namespace bt {

template <typename List, int32_t Offset = 0, int32_t Idx = 0, int32_t Max = ListSize<List>>
class BuildTopLevelLeafSubsetsH;

template <typename List, int32_t Offset = 0>
using BuildTopLevelLeafSubsets = typename BuildTopLevelLeafSubsetsH<List, Offset>::Type;



template <typename List, int32_t Offset, int32_t Idx, int32_t Max>
class BuildTopLevelLeafSubsetsH {
    static const int32_t LeafOffsetInf = list_tree::LeafCountInf<List, IntList<Idx>>;
    static const int32_t LeafOffsetSup = list_tree::LeafCountSup<List, IntList<Idx>>;
public:
    using Type = MergeLists<
            list_tree::MakeValueList<
                int32_t,
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


template <typename List, int32_t Offset, int32_t Max>
class BuildTopLevelLeafSubsetsH<List, Offset, Max, Max> {
public:
    using Type = TL<>;
};





template <typename Dispatcher, typename GroupList, int32_t GroupIdx = 0> struct GroupDispatcher;

template <
    typename Dispatcher,
    typename Group,
    typename... Tail,
    int32_t GroupIdx
>
struct GroupDispatcher<Dispatcher, TypeList<Group, Tail...>, GroupIdx>
{
    template <typename Allocator, typename Fn, typename... Args>
    static void dispatchGroups(Allocator* allocator, Fn&& fn, Args&&... args)
    {
        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<Group, GroupIdx>;

        SubgroupDispatcher::dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return GroupDispatcher<Dispatcher, TypeList<Tail...>, GroupIdx + 1>::dispatchGroups(
                allocator,
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename State, typename Allocator, typename Fn, typename... Args>
    static void dispatchGroups(State&& state, Allocator* allocator, Fn&& fn, Args&&... args)
    {
        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<Group, GroupIdx>;

        SubgroupDispatcher(state).dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return GroupDispatcher<Dispatcher, TypeList<Tail...>, GroupIdx + 1>::dispatchGroups(
                std::forward<State>(state),
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

        return GroupDispatcher<Dispatcher, TypeList<Tail...>, GroupIdx + 1>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <int32_t GroupIdx_, typename Allocator, typename Fn, typename... Args>
    static void dispatchGroup(Allocator* allocator, Fn&& fn, Args&&... args)
    {
        using TargetGroup = Select<GroupIdx_, TypeList<Group, Tail...>>;

        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<TargetGroup, GroupIdx_>;

        return SubgroupDispatcher::dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename State, int32_t GroupIdx_, typename Allocator, typename Fn, typename... Args>
    static void dispatchGroup(State&& state, Allocator* allocator, Fn&& fn, Args&&... args)
    {
        using TargetGroup = Select<GroupIdx_, TypeList<Group, Tail...>>;

        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<TargetGroup, GroupIdx_>;

        return SubgroupDispatcher(state).dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t GroupIdx_, typename Fn, typename... Args>
    static void dispatchGroupStatic(Fn&& fn, Args&&... args)
    {
        using TargetGroup = Select<GroupIdx_, TypeList<Group, Tail...>>;

        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<TargetGroup, GroupIdx_>;

        return SubgroupDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};


template <
    typename Dispatcher,
    int32_t GroupIdx
>
struct GroupDispatcher<Dispatcher, TypeList<>, GroupIdx>
{
    template <typename Allocator, typename Fn, typename... Args>
    static void dispatchGroups(Allocator* allocator, Fn&& fn, Args&&... args)  {
    }

    template <typename State, typename Allocator, typename Fn, typename... Args>
    static void dispatchGroups(State&&, Allocator* allocator, Fn&& fn, Args&&... args)  {
    }


    template <typename Fn, typename... Args>
    static void dispatchGroupsStatic(Fn&& fn, Args&&... args)  {
    }
};




template <typename Dispatcher, typename GroupList, int32_t GroupIdx = 0> struct GroupDispatcherWithResult;

template <
    typename Dispatcher,
    typename Group,
    typename... Tail,
    int32_t GroupIdx
>
struct GroupDispatcherWithResult<Dispatcher, TypeList<Group, Tail...>, GroupIdx>
{
    template <typename Allocator, typename Fn, typename... Args>
    static VoidResult dispatchGroups(Allocator* allocator, Fn&& fn, Args&&... args)
    {
        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<Group, GroupIdx>;

        MEMORIA_TRY_VOID(SubgroupDispatcher::dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return GroupDispatcherWithResult<Dispatcher, TypeList<Tail...>, GroupIdx + 1>::dispatchGroups(
                allocator,
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename State, typename Allocator, typename Fn, typename... Args>
    static VoidResult dispatchGroups(State&& state, Allocator* allocator, Fn&& fn, Args&&... args)
    {
        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<Group, GroupIdx>;

        MEMORIA_TRY_VOID(SubgroupDispatcher(state).dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return GroupDispatcherWithResult<Dispatcher, TypeList<Tail...>, GroupIdx + 1>::dispatchGroups(
                std::forward<State>(state),
                allocator,
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    static VoidResult dispatchGroupsStatic(Fn&& fn, Args&&... args)
    {
        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<Group, GroupIdx>;

        MEMORIA_TRY_VOID(SubgroupDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...));

        return GroupDispatcherWithResult<Dispatcher, TypeList<Tail...>, GroupIdx + 1>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <int32_t GroupIdx_, typename Allocator, typename Fn, typename... Args>
    static VoidResult dispatchGroup(Allocator* allocator, Fn&& fn, Args&&... args)
    {
        using TargetGroup = Select<GroupIdx_, TypeList<Group, Tail...>>;

        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<TargetGroup, GroupIdx_>;

        return SubgroupDispatcher::dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename State, int32_t GroupIdx_, typename Allocator, typename Fn, typename... Args>
    static VoidResult dispatchGroup(State&& state, Allocator* allocator, Fn&& fn, Args&&... args)
    {
        using TargetGroup = Select<GroupIdx_, TypeList<Group, Tail...>>;

        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<TargetGroup, GroupIdx_>;

        return SubgroupDispatcher(state).dispatchAll(allocator, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t GroupIdx_, typename Fn, typename... Args>
    static VoidResult dispatchGroupStatic(Fn&& fn, Args&&... args)
    {
        using TargetGroup = Select<GroupIdx_, TypeList<Group, Tail...>>;

        using SubgroupDispatcher = typename Dispatcher::template SubsetDispatcher<TargetGroup, GroupIdx_>;

        return SubgroupDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};


template <
    typename Dispatcher,
    int32_t GroupIdx
>
struct GroupDispatcherWithResult<Dispatcher, TypeList<>, GroupIdx>
{
    template <typename Allocator, typename Fn, typename... Args>
    static VoidResult dispatchGroups(Allocator* allocator, Fn&& fn, Args&&... args)  {
        return VoidResult::of();
    }

    template <typename State, typename Allocator, typename Fn, typename... Args>
    static VoidResult dispatchGroups(State&&, Allocator* allocator, Fn&& fn, Args&&... args)  {
        return VoidResult::of();
    }


    template <typename Fn, typename... Args>
    static VoidResult dispatchGroupsStatic(Fn&& fn, Args&&... args)  {
        return VoidResult::of();
    }
};





}}
