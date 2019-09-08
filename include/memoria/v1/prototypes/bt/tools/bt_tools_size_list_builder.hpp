
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/list/list_tree.hpp>
#include <memoria/v1/core/types/algo/select.hpp>

namespace memoria {
namespace v1 {
namespace bt {

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
struct BuildEmptyRangeList<TL<Head, Tail...>>: HasType< 
    MergeLists<
        TL<TL<>>,
        typename BuildEmptyRangeList<TL<Tail...>>::Type
    >
>{};


template <typename... List, typename... Tail>
struct BuildEmptyRangeList<TL<TL<List...>, Tail...>>: HasType<
    MergeLists<
        TL<typename BuildEmptyRangeList<TL<List...>>::Type>,
        typename BuildEmptyRangeList<TL<Tail...>>::Type
    >
> 
{
    static_assert(
        sizeof...(List) > 0,
        "PackedStruct sub-list must not be empty"
    );
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


namespace _ {

    template <
        typename OffsetList,
        typename List,
        int32_t Idx = 0,
        int32_t Max = ListSize<List>
    >
    class TagStreamsStartT {
        static constexpr int32_t StreamOffset = list_tree::LeafCount<List, IntList<Idx>, 2>;

        using StreamStart = Select<StreamOffset, OffsetList>;

        using FixedList = Replace<
                OffsetList,
                StreamStartTag<StreamStart>,
                StreamOffset
        >;

    public:
        using Type = typename TagStreamsStartT<FixedList, List, Idx + 1, Max>::Type;
    };

    template <typename OffsetList, typename List, int32_t Idx>
    class TagStreamsStartT<OffsetList, List, Idx, Idx> {
    public:
        using Type = OffsetList;
    };

    template <typename OffsetList, typename List>
    using TagStreamsStart = typename TagStreamsStartT<OffsetList, List>::Type;

    template <typename List> struct OffsetBuilderT;
    template <typename List> using OffsetBuilder = typename OffsetBuilderT<List>::Type;

    template <typename List, int32_t Offset> struct InternalOffsetBuilder;

    template <
        typename Head,
        typename... Tail
    >
    struct OffsetBuilderT<TypeList<Head, Tail...>>: HasType<
        MergeLists<
            IntList<0>,
            OffsetBuilder<TypeList<Tail...>>
        >
    >{};

    template <
        typename... Head,
        typename... Tail
    >
    struct OffsetBuilderT<TypeList<TypeList<Head...>, Tail...>>: HasType< 
        MergeLists<
            typename InternalOffsetBuilder<TypeList<Head...>, 0>::Type,
            OffsetBuilder<TypeList<Tail...>>
        >
    >{};


    template <>
    struct OffsetBuilderT<TypeList<>>: HasType<TL<>> {};


    template <
        typename Head,
        typename... Tail,
        int32_t Offset
    >
    struct InternalOffsetBuilder<TypeList<Head, Tail...>, Offset>: HasType<
        MergeValueLists<
                IntList<Offset>,
                typename InternalOffsetBuilder<
                TypeList<Tail...>,
                Offset + PkdStructIndexes<Head>
            >::Type
        >
    >{};

    template <int32_t Offset>
    struct InternalOffsetBuilder<TypeList<>, Offset>: HasType<IntList<>>{};


    template <typename List, int32_t Offset = 0, int32_t Idx = 0, int32_t Max = ListSize<List>>
    class ForAllTopLevelElements;

    template <typename List, int32_t Offset, int32_t Idx, int32_t Max>
    class ForAllTopLevelElements {
        static const int32_t LeafOffset = list_tree::LeafCountInf<List, IntList<Idx>>;
    public:
        using Type = MergeValueLists<
            IntValue<LeafOffset + Offset>,
            typename ForAllTopLevelElements<
                List,
                Offset,
                Idx + 1,
                Max
            >::Type
        >;
    };


    template <typename List, int32_t Offset, int32_t Max>
    class ForAllTopLevelElements<List, Offset, Max, Max>: public HasType<IntList<>> {};
}


template <typename LeafTree>
class LeafOffsetListBuilder {
    using LinearLeafList = FlattenLeafTree<LeafTree>;
    using OffsetList = bt::_::OffsetBuilder<LinearLeafList>;
public:
    using Type = bt::_::TagStreamsStart<OffsetList, LeafTree>;
};




template <typename List, typename Path>
using LeafSubsetInf = typename bt::_::ForAllTopLevelElements<
        list_tree::Subtree<List, Path>,
        list_tree::LeafCount<List, Path>
>::Type;


template <typename List>
using StreamsStartSubset = LeafSubsetInf<List, IntList<>>;



template <typename List, int32_t Idx, int32_t Pos = 0> struct FindTopLevelIdx;

template <
    typename Head,
    typename... Tail,
    int32_t Idx,
    int32_t Pos
>
struct FindTopLevelIdx<TypeList<Head, Tail...>, Idx, Pos>
{
    static const int32_t Children = list_tree::SubtreeLeafCount<TypeList<Head>, IntList<>>;

    static const int32_t Value = Idx < Children ?
            Pos :
            FindTopLevelIdx<
                TypeList<Tail...>,
                Idx - Children,
                Pos + 1
            >::Value;
};

template <int32_t Idx, int32_t Pos>
struct FindTopLevelIdx<TypeList<>, Idx, Pos>: HasValue<int32_t, -1> {};





template <typename List, int32_t Idx, int32_t Pos = 0> struct FindLocalLeafOffsetV;
template <typename List, int32_t Idx, int32_t Pos = 0> struct FindLocalLeafOffsetT;

namespace _ {

    template <typename List, int32_t Idx, int32_t Pos, bool Condition> struct FindLocalLeafOffsetHelperV;
    template <typename List, int32_t Idx, int32_t Pos, bool Condition> struct FindLocalLeafOffsetHelperT;

    template <typename List>
    struct ListSizeHelper {
        static const int32_t Value = ListSize<List>;
    };

    template <typename List>
    struct ListSizeHelper<StreamStartTag<List>> {
        static const int32_t Value = ListSize<List>;
    };



    template <
        typename Head,
        typename... Tail,
        int32_t Idx, int32_t Pos
    >
    struct FindLocalLeafOffsetHelperV<TypeList<Head, Tail...>, Idx, Pos, true>: HasValue<int32_t, Idx - Pos> {};

    template <
        typename Head,
        typename... Tail,
        int32_t Idx, int32_t Pos
    >
    struct FindLocalLeafOffsetHelperT<TypeList<Head, Tail...>, Idx, Pos, true>: HasType<Head> {};


    template <
        typename Head,
        typename... Tail,
        int32_t Idx, int32_t Pos
    >
    struct FindLocalLeafOffsetHelperV<TypeList<Head, Tail...>, Idx, Pos, false>: HasValue<
        int32_t,
        FindLocalLeafOffsetV<TypeList<Tail...>, Idx, Pos + ListSizeHelper<Head>::Value>::Value
    > {};


    template <
        typename Head,
        typename... Tail,
        int32_t Idx, int32_t Pos
    >
    struct FindLocalLeafOffsetHelperT<TypeList<Head, Tail...>, Idx, Pos, false>: HasType<
        typename FindLocalLeafOffsetT<TypeList<Tail...>, Idx, Pos + ListSizeHelper<Head>::Value>::Type
    > {};
}


template <
    typename Head,
    typename... Tail,
    int32_t Idx,
    int32_t Pos
>
struct FindLocalLeafOffsetV<TypeList<Head, Tail...>, Idx, Pos>: HasValue<
    int32_t,
    FindLocalLeafOffsetV<TypeList<Tail...>, Idx, Pos + 1>::Value
>{};


template <
    typename Head,
    typename... Tail,
    int32_t Idx,
    int32_t Pos
>
struct FindLocalLeafOffsetT<TypeList<Head, Tail...>, Idx, Pos>: HasType<
    typename FindLocalLeafOffsetT<TypeList<Tail...>, Idx, Pos + 1>::Type
>{};




template <
    typename Head,
    typename... Tail,
    int32_t Idx
>
struct FindLocalLeafOffsetV<TypeList<Head, Tail...>, Idx, Idx>: HasValue<int32_t, 0> {};


template <
    typename Head,
    typename... Tail,
    int32_t Idx
>
struct FindLocalLeafOffsetT<TypeList<Head, Tail...>, Idx, Idx>: HasType<Head> {};




template <
    typename... List,
    typename... Tail,
    int32_t Idx,
    int32_t Pos
>
struct FindLocalLeafOffsetV<TypeList<TypeList<List...>, Tail...>, Idx, Pos> {
private:
    using LocalList = TypeList<List...>;
    static const int32_t LocalSize = ListSize<LocalList>;
public:

    static const int32_t Value = bt::_::FindLocalLeafOffsetHelperV<
        FailIf<false, TypeList<TypeList<List...>, Tail...>>,
        Idx,
        Pos,
        Idx < Pos + LocalSize
    >::Value;
};



template <
    typename... Tail,
    int32_t... List,
    int32_t Idx,
    int32_t Pos
>
struct FindLocalLeafOffsetT<TypeList<IntList<List...>, Tail...>, Idx, Pos> {
private:
    static const int32_t LocalSize = ListSize<IntList<List...>>;
public:
    using Type = typename bt::_::FindLocalLeafOffsetHelperT<
            TypeList<IntList<List...>, Tail...>,
            Idx,
            Pos,
            Idx < Pos + LocalSize
    >::Type;
};



template <
    typename... Tail,
    int32_t... List,
    int32_t Idx,
    int32_t Pos
>
struct FindLocalLeafOffsetT<TypeList<StreamStartTag<IntList<List...>>, Tail...>, Idx, Pos> {
private:
    static const int32_t LocalSize = ListSize<IntList<List...>>;
public:
    using Type = typename bt::_::FindLocalLeafOffsetHelperT<
            TypeList<StreamStartTag<IntList<List...>>, Tail...>,
            Idx,
            Pos,
            Idx < Pos + LocalSize
    >::Type;
};



template <
    typename... Tail,
    typename... List,
    int32_t Idx
>
struct FindLocalLeafOffsetV<TypeList<TypeList<List...>, Tail...>, Idx, Idx>: HasValue<int32_t, 0> {};

template <
    typename... Tail,
    int32_t... List,
    int32_t Idx
>
struct FindLocalLeafOffsetT<TypeList<IntList<List...>, Tail...>, Idx, Idx>: HasType<IntList<List...>> {};

template <
    typename... Tail,
    int32_t... List,
    int32_t Idx
>
struct FindLocalLeafOffsetT<TypeList<StreamStartTag<IntList<List...>>, Tail...>, Idx, Idx>: HasType< 
    StreamStartTag<IntList<List...>>
>{};


template <
    int32_t Idx,
    int32_t Pos
>
struct FindLocalLeafOffsetV<TypeList<>, Idx, Pos>;

template <
    int32_t Idx,
    int32_t Pos
>
struct FindLocalLeafOffsetT<TypeList<>, Idx, Pos>;






template <typename List, int32_t Idx> struct GetLeafPrefix;


template <typename List, int32_t Idx>
struct GetLeafPrefix: HasValue<int32_t, SelectV<Idx, List>> {};

template <typename List, int32_t Idx>
struct GetLeafPrefix<StreamStartTag<List>, Idx>: HasValue<int32_t, SelectV<Idx, List>> {};


template <typename Path, int32_t Depth = 0> struct IsStreamStart;

template <int32_t Head, int32_t... Tail>
struct IsStreamStart<IntList<Head, Tail...>, 0>: HasValue<
    bool,
    IsStreamStart<IntList<Tail...>, 1>::Value
>{};

template <int32_t Head, int32_t... Tail, int32_t Idx>
struct IsStreamStart<IntList<Head, Tail...>, Idx>: HasValue<
    bool,
    Head == 0 && IsStreamStart<IntList<Tail...>, Idx + 1>::Value
>{};

template <int32_t Idx>
struct IsStreamStart<IntList<>, Idx>: HasValue<bool, true> {};




namespace _ {
    template <typename List> struct IsStreamStartTag;

    template <int32_t... List>
    struct IsStreamStartTag<IntList<List...>>: HasValue<bool, false> {};


    template <int32_t... List>
    struct IsStreamStartTag<StreamStartTag<IntList<List...>>>: HasValue<bool, true> {};
}



template <typename LeafStructList, int32_t LeafIdx>
struct LeafToBranchIndexByValueTranslator {
protected:
    using LeafOffsets = typename LeafOffsetListBuilder<LeafStructList>::Type;

    using Leafs = FlattenLeafTree<LeafStructList>;

    static constexpr int32_t LocalLeafOffset = FindLocalLeafOffsetV<FailIf<LeafIdx == 100, Leafs>, LeafIdx>::Value;

    using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

    using LeafPath = FailIf<LeafIdx == 100, typename list_tree::BuildTreePath<LeafStructList, LeafIdx>::Type>;

public:
    static constexpr int32_t LeafOffset = GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;

    static constexpr int32_t BranchStructIdx = list_tree::LeafCount<LeafStructList, LeafPath, 2> - FailIfV<LeafIdx == 10, LocalLeafOffset>;

    static constexpr bool IsStreamStart = LocalLeafOffset == 0 && bt::_::IsStreamStartTag<LocalLeafGroup>::Value;
};




template <typename LeafStructList, typename LeafPath, int32_t LeafIndex = 0>
struct LeafToBranchIndexTranslator {
protected:
    using LeafOffsets = typename LeafOffsetListBuilder<LeafStructList>::Type;

    using Leafs = FlattenLeafTree<LeafStructList>;

    static const int32_t LeafIdx            = list_tree::LeafCount<LeafStructList, LeafPath>;
    static const int32_t LocalLeafOffset    = FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;

    using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

public:
    static const int32_t BranchIndex = LeafIndex
                                    + GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;


    static int32_t translate(int32_t leaf_index)
    {
        const int32_t LeafIdx           = list_tree::LeafCount<LeafStructList, LeafPath>;
        const int32_t LocalLeafOffset   = FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;

        using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

        const int32_t LeafPrefix = GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;

        const int32_t BranchIndex = leaf_index + LeafPrefix;

        return BranchIndex;
    }
};



template <typename LeafStructList, typename BranchStructList, typename LeafPath>
using BuildBranchPath = typename list_tree::BuildTreePath<
            BranchStructList,
            list_tree::LeafCountInf<LeafStructList, LeafPath, 2> -
            FindLocalLeafOffsetV<
                    FlattenLeafTree<LeafStructList>,
                    list_tree::LeafCount<LeafStructList, LeafPath>
            >::Value
>::Type;



template <
    typename LeafStructList,
    typename BranchStructList,
    typename LeafPath
>
using BrachStructAccessorTool = Select<
        list_tree::LeafCountInf<LeafStructList, LeafPath, 2> -
        FindLocalLeafOffsetV<
                FlattenLeafTree<LeafStructList>,
                list_tree::LeafCount<LeafStructList, LeafPath>
        >::Value,
        Linearize<BranchStructList>
>;




}
}}
