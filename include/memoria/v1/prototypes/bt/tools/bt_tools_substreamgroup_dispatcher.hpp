
// Copyright 2014 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/list/list_tree.hpp>
#include <memoria/v1/core/types/algo/select.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <tuple>

namespace memoria {
namespace v1 {
namespace bt        {

namespace detail {



}

template <typename List, Int Offset = 0, Int Idx = 0, Int Max = ListSize<List>::Value>
class BuildTopLevelLeafSubsetsH;

template <typename List, Int Offset = 0>
using BuildTopLevelLeafSubsets = typename BuildTopLevelLeafSubsetsH<List, Offset>::Type;



template <typename List, Int Offset, Int Idx, Int Max>
class BuildTopLevelLeafSubsetsH {
    static const Int LeafOffsetInf = v1::list_tree::LeafCountInf<List, IntList<Idx>>::Value;
    static const Int LeafOffsetSup = v1::list_tree::LeafCountSup<List, IntList<Idx>>::Value;
public:
    using Type = MergeLists<
            v1::list_tree::MakeValueList<
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
}}