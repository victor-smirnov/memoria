
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_BASE_DETAIL_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_BASE_DETAIL_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/list_tree.hpp>


#include <tuple>

namespace memoria {
namespace bt      {
namespace detail  {

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
            using RangesIdxList = memoria::list_tree::MakeValueList<Int, 0, std::tuple_size<RangesTuple>::value>;

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

        using StreamIdxList = memoria::list_tree::MakeValueList<Int, SubstreamsStartIdx, SubstreamsStartIdx + StreamSize>;

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
struct LeafIndexRangeWalker<AccumItemH, TL<memoria::bt::SumRange<From, To>, Tail...>, IntList<Offset, RTail...>> {

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
struct LeafIndexRangeWalker<AccumItemH, TL<memoria::bt::MaxRange<From, To>, Tail...>, IntList<Offset, RTail...>> {

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

        using LeafPath   = typename memoria::list_tree::BuildTreePath<LeafStructList, SubstreamIdx>::Type;
        using AccumItemH = memoria::bt::AccumItem<LeafStructList, LeafPath, Accum>;

        using RangeList = Select<SubstreamIdx, Linearize<LeafRangeList, 2>>;
        using RangeOffsetList = Select<SubstreamIdx, Linearize<LeafRangeOffsetList>>;

        LeafIndexRangeWalker<AccumItemH, RangeList, RangeOffsetList>::process(obj, walker, accum, std::forward<Args>(args)...);
    }
};

}

}
}

#endif
