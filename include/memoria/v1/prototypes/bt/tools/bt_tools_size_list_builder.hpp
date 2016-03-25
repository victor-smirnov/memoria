
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

namespace memoria {
namespace v1 {
namespace bt        {

template <typename Tree>
using FlattenBranchTree = Linearize<Tree, 1>;

template <typename Tree>
using FlattenLeafTree = Linearize<Tree, 2>;

template <typename SizeList>
struct StreamStartTag {
    using Type = SizeList;
};


template <typename StructList> struct BuildEmptyRangeList;

template <typename Head, typename... Tail>
struct BuildEmptyRangeList<TL<Head, Tail...>> {
    using Type = MergeLists<
        TL<TL<>>,
        typename BuildEmptyRangeList<TL<Tail...>>::Type
    >;
};


template <typename... List, typename... Tail>
struct BuildEmptyRangeList<TL<TL<List...>, Tail...>> {
    static_assert(
        sizeof...(List) > 0,
        "PackedStruct sub-list must not be empty"
    );

    using Type = MergeLists<
        TL<typename BuildEmptyRangeList<TL<List...>>::Type>,
        typename BuildEmptyRangeList<TL<Tail...>>::Type
    >;
};

template <>
struct BuildEmptyRangeList<TL<>> {
    using Type = TL<>;
};


template <typename T> struct DefaultBranchStructTF;

template <
    typename LeafTypeT,
    template <typename T> class BranchStructTF = DefaultBranchStructTF,
    typename IndexRangeListT = typename BuildEmptyRangeList<LeafTypeT>::Type
>
struct StreamTF {};


namespace {

    template <
        typename OffsetList,
        typename List,
        Int Idx                 = 0,
        Int Max                 = ListSize<List>::Value
    >
    class TagStreamsStartT {
        static constexpr Int StreamOffset = list_tree::LeafCount<List, IntList<Idx>, 2>::Value;

        using StreamStart = Select<StreamOffset, OffsetList>;

        using FixedList = Replace<
                OffsetList,
                StreamStartTag<StreamStart>,
                StreamOffset
        >;

    public:
        using Type = typename TagStreamsStartT<FixedList, List, Idx + 1, Max>::Type;
    };

    template <typename OffsetList, typename List, Int Idx>
    class TagStreamsStartT<OffsetList, List, Idx, Idx> {
    public:
        using Type = OffsetList;
    };

    template <typename OffsetList, typename List>
    using TagStreamsStart = typename TagStreamsStartT<OffsetList, List>::Type;

    template <typename List> struct OffsetBuilderT;
    template <typename List> using OffsetBuilder = typename OffsetBuilderT<List>::Type;

    template <typename List, Int Offset> struct InternalOffsetBuilder;

    template <
        typename Head,
        typename... Tail
    >
    struct OffsetBuilderT<TypeList<Head, Tail...>> {
        using Type = MergeLists<
                IntList<0>,
                OffsetBuilder<TypeList<Tail...>>
        >;
    };

    template <
        typename... Head,
        typename... Tail
    >
    struct OffsetBuilderT<TypeList<TypeList<Head...>, Tail...>> {
        using Type = MergeLists<
                typename InternalOffsetBuilder<TypeList<Head...>, 0>::Type,
                OffsetBuilder<TypeList<Tail...>>
        >;
    };


    template <>
    struct OffsetBuilderT<TypeList<>> {
        using Type = TypeList<>;
    };


    template <
        typename Head,
        typename... Tail,
        Int Offset
    >
    struct InternalOffsetBuilder<TypeList<Head, Tail...>, Offset>
    {
        using Type = MergeValueListsT<
                IntList<Offset>,
                typename InternalOffsetBuilder<
                TypeList<Tail...>,
                Offset + StructSizeProvider<Head>::Value
            >::Type
        >;
    };

    template <Int Offset>
    struct InternalOffsetBuilder<TypeList<>, Offset>
    {
        using Type = IntList<>;
    };


    template <typename List, Int Offset = 0, Int Idx = 0, Int Max = ListSize<List>::Value>
    class ForAllTopLevelElements;

    template <typename List, Int Offset, Int Idx, Int Max>
    class ForAllTopLevelElements {
        static const Int LeafOffset = v1::list_tree::LeafCountInf<List, IntList<Idx>>::Value;
    public:
        using Type = AppendItemToList<
                IntValue<LeafOffset + Offset>,
                typename ForAllTopLevelElements<
                    List,
                    Offset,
                    Idx + 1,
                    Max
                >::Type
            >;
    };


    template <typename List, Int Offset, Int Max>
    class ForAllTopLevelElements<List, Offset, Max, Max> {
    public:
        using Type = IntList<>;
    };
}


template <typename LeafTree>
class LeafOffsetListBuilder {
    using LinearLeafList = FlattenLeafTree<LeafTree>;
    using OffsetList = FailIf<OffsetBuilder<LinearLeafList>, false>;
public:
    using Type = TagStreamsStart<OffsetList, LeafTree>;
};




template <typename List, typename Path>
using LeafSubsetInf = typename ForAllTopLevelElements<
        typename v1::list_tree::Subtree<List, Path>::Type,
        v1::list_tree::LeafCount<List, Path>::Value
>::Type;


template <typename List>
using StreamsStartSubset = LeafSubsetInf<List, IntList<>>;



template <typename List, Int Idx, Int Pos = 0> struct FindTopLevelIdx;

template <
    typename Head,
    typename... Tail,
    Int Idx,
    Int Pos
>
struct FindTopLevelIdx<TypeList<Head, Tail...>, Idx, Pos>
{
    static const Int Children = v1::list_tree::SubtreeLeafCount<TypeList<Head>, IntList<>>::Value;

    static const Int Value = Idx < Children ?
            Pos :
            FindTopLevelIdx<
                TypeList<Tail...>,
                Idx - Children,
                Pos + 1
            >::Value;
};

template <Int Idx, Int Pos>
struct FindTopLevelIdx<TypeList<>, Idx, Pos> {
    static const Int Value = -1;
};





template <typename List, Int Idx, Int Pos = 0> struct FindLocalLeafOffsetV;
template <typename List, Int Idx, Int Pos = 0> struct FindLocalLeafOffsetT;

namespace {

    template <typename List, Int Idx, Int Pos, bool Condition> struct FindLocalLeafOffsetHelperV;
    template <typename List, Int Idx, Int Pos, bool Condition> struct FindLocalLeafOffsetHelperT;

    template <typename List>
    struct ListSizeHelper {
        static const Int Value = ListSize<List>::Value;
    };

    template <typename List>
    struct ListSizeHelper<StreamStartTag<List>> {
        static const Int Value = ListSize<List>::Value;
    };



    template <
        typename Head,
        typename... Tail,
        Int Idx, Int Pos
    >
    struct FindLocalLeafOffsetHelperV<TypeList<Head, Tail...>, Idx, Pos, true> {
        static const Int Value = Idx - Pos;
    };

    template <
        typename Head,
        typename... Tail,
        Int Idx, Int Pos
    >
    struct FindLocalLeafOffsetHelperT<TypeList<Head, Tail...>, Idx, Pos, true> {
        using Type = Head;
    };


    template <
        typename Head,
        typename... Tail,
        Int Idx, Int Pos
    >
    struct FindLocalLeafOffsetHelperV<TypeList<Head, Tail...>, Idx, Pos, false> {
        static const Int Value = FindLocalLeafOffsetV<TypeList<Tail...>, Idx, Pos + ListSizeHelper<Head>::Value>::Value;
    };


    template <
        typename Head,
        typename... Tail,
        Int Idx, Int Pos
    >
    struct FindLocalLeafOffsetHelperT<TypeList<Head, Tail...>, Idx, Pos, false> {
        using Type = typename FindLocalLeafOffsetT<TypeList<Tail...>, Idx, Pos + ListSizeHelper<Head>::Value>::Type;
    };
}


template <
    typename Head,
    typename... Tail,
    Int Idx,
    Int Pos
>
struct FindLocalLeafOffsetV<TypeList<Head, Tail...>, Idx, Pos> {
    static const Int Value = FindLocalLeafOffsetV<TypeList<Tail...>, Idx, Pos + 1>::Value;
};


template <
    typename Head,
    typename... Tail,
    Int Idx,
    Int Pos
>
struct FindLocalLeafOffsetT<TypeList<Head, Tail...>, Idx, Pos> {
    using Type = typename FindLocalLeafOffsetT<TypeList<Tail...>, Idx, Pos + 1>::Type;
};




template <
    typename Head,
    typename... Tail,
    Int Idx
>
struct FindLocalLeafOffsetV<TypeList<Head, Tail...>, Idx, Idx> {
    static const Int Value = 0;
};


template <
    typename Head,
    typename... Tail,
    Int Idx
>
struct FindLocalLeafOffsetT<TypeList<Head, Tail...>, Idx, Idx> {
    using Type = Head;
};




template <
    typename... List,
    typename... Tail,
    Int Idx,
    Int Pos
>
struct FindLocalLeafOffsetV<TypeList<TypeList<List...>, Tail...>, Idx, Pos> {
private:
    using LocalList = TypeList<List...>;
    static const Int LocalSize = ListSize<LocalList>::Value;
public:

    static const Int Value = FindLocalLeafOffsetHelperV<
        TypeList<TypeList<List...>, Tail...>,
        Idx,
        Pos,
        Idx < Pos + LocalSize
    >::Value;
};



template <
    typename... Tail,
    Int... List,
    Int Idx,
    Int Pos
>
struct FindLocalLeafOffsetT<TypeList<IntList<List...>, Tail...>, Idx, Pos> {
private:
    static const Int LocalSize = ListSize<IntList<List...>>::Value;
public:
    using Type = typename FindLocalLeafOffsetHelperT<
            TypeList<IntList<List...>, Tail...>,
            Idx,
            Pos,
            Idx < Pos + LocalSize
    >::Type;
};



template <
    typename... Tail,
    Int... List,
    Int Idx,
    Int Pos
>
struct FindLocalLeafOffsetT<TypeList<StreamStartTag<IntList<List...>>, Tail...>, Idx, Pos> {
private:
    static const Int LocalSize = ListSize<IntList<List...>>::Value;
public:
    using Type = typename FindLocalLeafOffsetHelperT<
            TypeList<StreamStartTag<IntList<List...>>, Tail...>,
            Idx,
            Pos,
            Idx < Pos + LocalSize
    >::Type;
};



template <
    typename... Tail,
    typename... List,
    Int Idx
>
struct FindLocalLeafOffsetV<TypeList<TypeList<List...>, Tail...>, Idx, Idx> {
    static const Int Value = 0;
};

template <
    typename... Tail,
    Int... List,
    Int Idx
>
struct FindLocalLeafOffsetT<TypeList<IntList<List...>, Tail...>, Idx, Idx> {
    using Type = IntList<List...>;
};

template <
    typename... Tail,
    Int... List,
    Int Idx
>
struct FindLocalLeafOffsetT<TypeList<StreamStartTag<IntList<List...>>, Tail...>, Idx, Idx> {
    using Type = StreamStartTag<IntList<List...>>;
};


template <
    Int Idx,
    Int Pos
>
struct FindLocalLeafOffsetV<TypeList<>, Idx, Pos>;

template <
    Int Idx,
    Int Pos
>
struct FindLocalLeafOffsetT<TypeList<>, Idx, Pos>;






template <typename List, Int Idx> struct GetLeafPrefix;


template <typename List, Int Idx>
struct GetLeafPrefix {
    static const Int Value = SelectV<Idx, List>::Value;
};

template <typename List, Int Idx>
struct GetLeafPrefix<StreamStartTag<List>, Idx> {
    static const Int Value = SelectV<Idx, List>::Value;
};


template <typename Path, Int Depth = 0> struct IsStreamStart;

template <Int Head, Int... Tail>
struct IsStreamStart<IntList<Head, Tail...>, 0> {
    static const bool Value = IsStreamStart<IntList<Tail...>, 1>::Value;
};

template <Int Head, Int... Tail, Int Idx>
struct IsStreamStart<IntList<Head, Tail...>, Idx> {
    static const bool Value = Head == 0 && IsStreamStart<IntList<Tail...>, Idx + 1>::Value;
};

template <Int Idx>
struct IsStreamStart<IntList<>, Idx> {
    static const bool Value = true;
};




namespace {
    template <typename List> struct IsStreamStartTag;

    template <Int... List>
    struct IsStreamStartTag<IntList<List...>> {
        static const bool Value = false;
    };


    template <Int... List>
    struct IsStreamStartTag<StreamStartTag<IntList<List...>>> {
        static const bool Value = true;
    };
}



template <typename LeafStructList, Int LeafIdx>
struct LeafToBranchIndexByValueTranslator {
protected:
    using LeafOffsets = typename LeafOffsetListBuilder<LeafStructList>::Type;

    using Leafs = FlattenLeafTree<LeafStructList>;

    static const Int LocalLeafOffset = FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;

    using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

    using LeafPath = typename v1::list_tree::BuildTreePath<LeafStructList, LeafIdx>::Type;

public:
    static const Int LeafOffset = GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;

    static const Int BranchStructIdx = v1::list_tree::LeafCount<LeafStructList, LeafPath, 2>::Value - LocalLeafOffset;

    static const bool IsStreamStart = LocalLeafOffset == 0 && IsStreamStartTag<LocalLeafGroup>::Value;
};




template <typename LeafStructList, typename LeafPath, Int LeafIndex = 0>
struct LeafToBranchIndexTranslator {
protected:
    using LeafOffsets = typename LeafOffsetListBuilder<LeafStructList>::Type;

    using Leafs = FlattenLeafTree<LeafStructList>;

    static const Int LeafIdx            = v1::list_tree::LeafCount<LeafStructList, LeafPath>::Value;
    static const Int LocalLeafOffset    = FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;

    using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

public:
    static const Int BranchIndex = LeafIndex
                                    + GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;


    static Int translate(Int leaf_index)
    {
        const Int LeafIdx           = v1::list_tree::LeafCount<LeafStructList, LeafPath>::Value;
        const Int LocalLeafOffset   = FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;

        using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

        const Int LeafPrefix = GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;

        const Int BranchIndex = leaf_index + LeafPrefix;

        return BranchIndex;
    }
};



template <typename LeafStructList, typename BranchStructList, typename LeafPath>
using BuildBranchPath = typename v1::list_tree::BuildTreePath<
            BranchStructList,
            v1::list_tree::LeafCountInf<LeafStructList, LeafPath, 2>::Value -
                FindLocalLeafOffsetV<
                    FlattenLeafTree<LeafStructList>,
                    v1::list_tree::LeafCount<LeafStructList, LeafPath>::Value
                >::Value
>::Type;



template <
    typename LeafStructList,
    typename BranchStructList,
    typename LeafPath
>
using BrachStructAccessorTool = Select<
        v1::list_tree::LeafCountInf<LeafStructList, LeafPath, 2>::Value -
            FindLocalLeafOffsetV<
                FlattenLeafTree<LeafStructList>,
                v1::list_tree::LeafCount<LeafStructList, LeafPath>::Value
        >::Value,
        Linearize<BranchStructList>
>;




}
}}