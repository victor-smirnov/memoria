
// Copyright 2015 Victor Smirnov
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


#include <tuple>

namespace memoria {
namespace v1 {
namespace bt      {
namespace details  {

template <typename StreamIdxList> struct IteratorStreamRangesListWalker;

namespace {

    template <typename SubstreamIdxList> struct IteratorSubstreamsRangesListWalker;
    template <typename RangesIdxList> struct SubstreamRangesTupleWalker;

    template <Int Idx, Int... Tail>
    struct SubstreamRangesTupleWalker<IntList<Idx, Tail...>> {

        template <typename Walker, typename StreamObj, typename RangesListTuple, typename... Args>
        static void process(Walker& walker, const StreamObj* obj, RangesListTuple& ranges_tuple, Args&&... args)
        {
            walker.branch_iterator_BranchNodeEntry(obj, std::get<Idx>(ranges_tuple), std::forward<Args>(args)...);

            SubstreamRangesTupleWalker<IntList<Tail...>>::process(walker, obj, ranges_tuple, std::forward<Args>(args)...);
        }
    };


    template <>
    struct SubstreamRangesTupleWalker<IntList<>> {

        template <typename Walker, typename Node, typename RangesListTuple, typename... Args>
        static void process(Walker& walker, const Node* node, RangesListTuple& accum, Args&&... args)
        {}
    };





    template <Int Idx, Int... Tail>
    struct IteratorSubstreamsRangesListWalker<IntList<Idx, Tail...>> {

        template <typename Walker, typename Node, typename BranchNodeEntry, typename... Args>
        static void process(Walker& walker, const Node* node, BranchNodeEntry& accum, Args&&... args)
        {
            using RangesTuple   = typename std::tuple_element<Idx, BranchNodeEntry>::type;
            using RangesIdxList = v1::list_tree::MakeValueList<Int, 0, std::tuple_size<RangesTuple>::value>;

            IteratorSubstreamsRangesListWalker<IntList<Idx, Tail...>> w;

            node->template processStreamByIdx<Idx>(w, RangesIdxList(), std::get<Idx>(accum), walker, std::forward<Args>(args)...);

            IteratorSubstreamsRangesListWalker<IntList<Tail...>>::process(walker, node, accum, std::forward<Args>(args)...);
        }


        template <
            typename StreamObj,
            typename RangesIdxList,
            typename T,
            typename Walker,
            typename... Args
        >
        void stream(const StreamObj* obj, RangesIdxList, T& ranges_tuple, Walker& walker, Args&&... args)
        {
            SubstreamRangesTupleWalker<RangesIdxList>::process(walker, obj, ranges_tuple, std::forward<Args>(args)...);
        }
    };


    template <>
    struct IteratorSubstreamsRangesListWalker<IntList<>> {

        template <typename Walker, typename Node, typename BranchNodeEntry, typename... Args>
        static void process(Walker& walker, const Node* node, BranchNodeEntry& accum, Args&&... args)
        {}
    };

}





template <
    Int StreamIdx,
    Int... Tail
>
struct IteratorStreamRangesListWalker<IntList<StreamIdx, Tail...>> {

    template <typename Walker, typename Node, typename BranchNodeEntry, typename... Args>
    static void process(Walker& walker, const Node* node, BranchNodeEntry& accum, Args&&... args)
    {
        const Int SubstreamsStartIdx = Node::template StreamStartIdx<StreamIdx>::Value;
        const Int StreamSize         = Node::template StreamSize<StreamIdx>::Value;

        using StreamIdxList = v1::list_tree::MakeValueList<Int, SubstreamsStartIdx, SubstreamsStartIdx + StreamSize>;

        IteratorSubstreamsRangesListWalker<StreamIdxList>::process(walker, node, accum, std::forward<Args>(args)...);

        IteratorStreamRangesListWalker<IntList<Tail...>>::process(walker, node, accum, std::forward<Args>(args)...);
    }
};


template <>
struct IteratorStreamRangesListWalker<IntList<>> {

    template <typename Walker, typename Node, typename BranchNodeEntry, typename... Args>
    static void process(Walker& walker, const Node* node, BranchNodeEntry& accum, Args&&... args)
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
struct LeafIndexRangeWalker<AccumItemH, TL<v1::bt::SumRange<From, To>, Tail...>, IntList<Offset, RTail...>> {

    template <typename StreamObj, typename Walker, typename Accum, typename... Args>
    static void process(const StreamObj* obj, Walker& walker, Accum& accum, Args&&... args)
    {
        auto& item = AccumItemH::template item<Offset>(accum);

        walker.template leaf_iterator_BranchNodeEntry<Offset, From, To - From>(obj, item, std::forward<Args>(args)...);

        LeafIndexRangeWalker<AccumItemH, TL<Tail...>, IntList<RTail...>>::process(obj, walker, accum, std::forward<Args>(args)...);
    }
};

template <
    typename AccumItemH,
    Int From,
    Int To,
    Int Offset,
    typename... Tail,
    Int... RTail
>
struct LeafIndexRangeWalker<AccumItemH, TL<v1::bt::MaxRange<From, To>, Tail...>, IntList<Offset, RTail...>> {

    template <typename StreamObj, typename Walker, typename Accum, typename... Args>
    static void process(const StreamObj* obj, Walker& walker, Accum& accum, Args&&... args)
    {
        auto& item = AccumItemH::template item<Offset>(accum);

        walker.template leaf_iterator_BranchNodeEntry<Offset, From, To - From>(obj, item, std::forward<Args>(args)...);

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

        using LeafPath   = typename v1::list_tree::BuildTreePath<LeafStructList, SubstreamIdx>::Type;
        using AccumItemH = v1::bt::AccumItem<LeafStructList, LeafPath, Accum>;

        using RangeList = Select<SubstreamIdx, Linearize<LeafRangeList, 2>>;
        using RangeOffsetList = Select<SubstreamIdx, Linearize<LeafRangeOffsetList>>;

        LeafIndexRangeWalker<AccumItemH, RangeList, RangeOffsetList>::process(obj, walker, accum, std::forward<Args>(args)...);
    }
};

}

}
}}