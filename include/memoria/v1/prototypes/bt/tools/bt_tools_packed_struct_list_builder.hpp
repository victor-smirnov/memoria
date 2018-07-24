// Copyright 2012 Victor Smirnov
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
#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/v1/core/types/list/linearize.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_core.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_index_range.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_streamdescr_factory.hpp>


namespace memoria {
namespace v1 {
namespace bt {

namespace detail {

    template <typename BranchSubstream, typename LeafSubstream>
    struct ValidateSubstreams {
        static const bool Value = true;
    };

    template <typename T, typename... List>
    struct ValidateSubstreams<T, TypeList<List...>> {
        static const bool Value = true;
    };

    template <typename T, typename... List>
    struct ValidateSubstreams<TypeList<T>, TypeList<List...>> {
        static const bool Value = true;
    };

    template <typename T1, typename T2>
    struct ValidateSubstreams<TypeList<T1>, T2> {
        static const bool Value = true;
    };

    template <typename T1, typename... List1, typename T2, typename... List2>
    struct ValidateSubstreams<TypeList<T1, List1...>, TypeList<T2, List2...>> {
        static const bool Value = (sizeof...(List1) == sizeof...(List2)) &&
                                    IsPlainList<TypeList<T1, List1...>>::Value;
    };


    template <typename List> struct InputBufferListBuilder;

    template <typename Head, typename... Tail>
    struct InputBufferListBuilder<TL<Head, Tail...>> {
        using Type = MergeLists<
                typename PkdStructInputBufferType<Head>::Type,
                typename InputBufferListBuilder<TL<Tail...>>::Type
        >;
    };

    template <typename PackedStruct>
    struct InputBufferListBuilder {
        using Type = TL<typename PkdStructInputBufferType<PackedStruct>::Type>;
    };

    template <typename... List, typename... Tail>
    struct InputBufferListBuilder<TL<TL<List...>, Tail...>> {
        using Type = MergeLists<
                TL<typename InputBufferListBuilder<TL<List...>>::Type>,
                typename InputBufferListBuilder<TL<Tail...>>::Type
        >;
    };

    template <>
    struct InputBufferListBuilder<TL<>> {
        using Type = TL<>;
    };
}





template <typename SumType, typename List>
class PackedLeafStructListBuilder;

template <typename SumType, typename List>
class PackedBranchStructListBuilder;

template <typename SumType, typename List>
class IteratorBranchNodeEntryListBuilder;



template <
    typename SumType,
    typename LeafType,
    typename IdxRangeList,
    template <typename> class BranchStructTF,
    typename... Tail
>
class PackedLeafStructListBuilder<SumType, TypeList<StreamTF<LeafType, BranchStructTF, IdxRangeList>, Tail...>> {

    using BranchType = typename BTStreamDescritorsBuilder<FlattenLeafTree<LeafType>, BranchStructTF, SumType>::Type;

    static_assert(
            true,//ValidateSubstreams<BranchType, LeafType>::Value,
            "Invalid substream structure"
    );

    using InputBufferType = typename detail::InputBufferListBuilder<LeafType>::Type;

public:
    using StructList = MergeLists<
                TL<LeafType>,
                typename PackedLeafStructListBuilder<
                    SumType,
                    TypeList<Tail...>
                >::StructList
    >;

    using StreamInputList = MergeLists<
            TL<typename MakeStreamEntryTL<Linearize<LeafType>>::Type>,
            typename PackedLeafStructListBuilder<
                SumType,
                TypeList<Tail...>
            >::StreamInputList
    >;

    using InputBufferList = MergeLists<
            TL<InputBufferType>,
            typename PackedLeafStructListBuilder<
                        SumType,
                        TypeList<Tail...>
            >::InputBufferList
    >;
};




template <
    typename SumType,
    typename LeafType,
    typename IdxRangeList,
    template <typename> class BranchStructTF,
    typename... Tail
>
class PackedBranchStructListBuilder<SumType, TypeList<StreamTF<LeafType, BranchStructTF, IdxRangeList>, Tail...>> {

    using BranchType = typename BTStreamDescritorsBuilder<FlattenLeafTree<LeafType>, BranchStructTF, SumType>::Type;

public:
    using StructList = MergeLists<
                TL<BranchType>,
                typename PackedBranchStructListBuilder<
                    SumType,
                    TypeList<Tail...>
                >::StructList
    >;
};

template <typename... T>
class Undefined;


template <
    typename SumType,
    typename LeafType,
    typename IdxRangeList,
    template <typename> class BranchStructTF,
    typename... Tail
>
class IteratorBranchNodeEntryListBuilder<SumType, TypeList<StreamTF<LeafType, BranchStructTF, IdxRangeList>, Tail...>> {

    using BranchType = typename BTStreamDescritorsBuilder<FlattenLeafTree<LeafType>, BranchStructTF, SumType>::Type;

    using LeafStructList    = FlattenLeafTree<LeafType>;
    using BranchStructList  = FlattenBranchTree<BranchType>;
    using FlatIdxRangeList  = FlattenIndexRangeTree<IdxRangeList>;

    using RangeListType = typename BranchNodeRangeListBuilder<
            BranchStructList,
            LeafStructList,
            FlatIdxRangeList
    >::Type;

    using RangeOffsetListType = typename BranchNodeRangeListBuilder<
            BranchStructList,
            LeafStructList,
            FlatIdxRangeList
    >::OffsetList;

    using AccType = typename IteratorBranchNodeEntryBuilder<
            BranchStructList,
            RangeListType
    >::Type;

public:
    using AccumTuple = MergeLists<
            TL<AccType>,
            typename IteratorBranchNodeEntryListBuilder<SumType, TypeList<Tail...>>::AccumTuple
    >;

    using RangeOffsetList = MergeLists<
            TL<RangeOffsetListType>,
            typename IteratorBranchNodeEntryListBuilder<SumType, TypeList<Tail...>>::RangeOffsetList
    >;

    using IndexRangeList = MergeLists<
            TL<IdxRangeList>,
            typename IteratorBranchNodeEntryListBuilder<SumType, TypeList<Tail...>>::IndexRangeList
    >;
};

template <typename SumType>
class PackedLeafStructListBuilder<SumType, TypeList<>> {
public:
    using StructList        = TypeList<>;
    using StreamInputList   = TypeList<>;
    using InputBufferList   = TypeList<>;
};


template <typename SumType>
class PackedBranchStructListBuilder<SumType, TypeList<>> {
public:
    using StructList = TypeList<>;
};

template <typename SumType>
class IteratorBranchNodeEntryListBuilder<SumType, TypeList<>> {
public:
    using AccumTuple        = TypeList<>;
    using RangeOffsetList   = TypeList<>;
    using IndexRangeList    = TypeList<>;
};







template <typename T> struct BranchNodeEntryBuilder;

template <typename PackedStruct, typename... Tail>
struct BranchNodeEntryBuilder<TL<PackedStruct, Tail...>> {
    using Type = MergeLists<
                    core::StaticVector<
                        typename PkdSearchKeyTypeProvider<PackedStruct>::Type,
                        StructSizeProvider<PackedStruct>::Value
                    >,
                    typename BranchNodeEntryBuilder<TL<Tail...>>::Type
    >;
};

template <>
struct BranchNodeEntryBuilder<TL<>> {
    using Type = TL<>;
};







}
}}
