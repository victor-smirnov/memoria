
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


#include <memoria/v1/core/tools/static_array.hpp>

#include <memoria/v1/core/types/list/append.hpp>
#include <memoria/v1/core/types/list/linearize.hpp>
#include <memoria/v1/core/types/list/list_tree.hpp>
#include <memoria/v1/core/exceptions/bounds.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_size_list_builder.hpp>

#include <iostream>
#include <tuple>
#include <utility>

namespace memoria {
namespace v1 {
namespace bt        {


template <int32_t From_, int32_t To_ = From_ +1>
struct SumRange {
    static const int32_t From = From_;
    static const int32_t To = To_;
};

template <int32_t From_, int32_t To_ = From_ +1>
struct MaxRange {
    static const int32_t From = From_;
    static const int32_t To = To_;
};

template <typename T> struct IndexesH;

template <typename T>
using Indexes = typename IndexesH<T>::Type;

template <int32_t Head, int32_t... Tail>
struct IndexesH<IntList<Head, Tail...>>:
    detail::MergeListsH<SumRange<Head>, typename IndexesH<IntList<Tail...>>::Type>
{};


template <typename IndexRangeTree>
using FlattenIndexRangeTree = Linearize<IndexRangeTree, 3>;

template <int32_t Head>
struct IndexesH<IntList<Head>>:
    TypeP<TypeList<SumRange<Head>>>
{};




template <typename T, typename RangesList>
struct IndexDescr {};

template <typename T, int32_t From_, int32_t To_>
struct SumVector: public v1::core::StaticVector<T, To_ - From_> {
    static const int32_t To     = To_;
    static const int32_t From   = From_;
};

template <typename T, int32_t From_, int32_t To_>
struct MaxVector: public v1::core::StaticVector<T, To_ - From_> {
    static const int32_t To     = To_;
    static const int32_t From   = From_;
};

template <typename T>
struct EmptyVector {
    static const int32_t To     = 0;
    static const int32_t From   = 0;

    bool operator==(const EmptyVector<T>&) const {
        return true;
    }

    bool operator!=(const EmptyVector<T>&) const {
        return false;
    }
};

template <typename T>
std::ostream& operator<<(std::ostream& out, const EmptyVector<T>& v) {
    out<<"EmptyVector<>";
    return out;
}


namespace detail {

    template <typename T, typename List, int32_t Max> struct AccumBuilderH;

    template <typename T, int32_t From, int32_t To, typename... Tail, int32_t Max>
    struct AccumBuilderH<T, TypeList<SumRange<From, To>, Tail...>, Max>: HasType<
        MergeLists<
            SumVector<T, From, To>,
            typename AccumBuilderH<T, TypeList<Tail...>, Max>::Type
        >
    >
    {
        static_assert(To <= Max, "Index range must not exceed the limit");
    };

    template <typename T, int32_t From, int32_t To, int32_t Max>
    struct AccumBuilderH<T, TypeList<SumRange<From, To>>, Max>
    {
        using Type = TL<SumVector<T, From, To>>;
        static_assert(To <= Max, "Index range must not exceed the limit");
    };


    template <typename T, int32_t From, int32_t To, typename... Tail, int32_t Max>
    struct AccumBuilderH<T, TypeList<MaxRange<From, To>, Tail...>, Max>:
        memoria::v1::detail::MergeListsH<
            MaxVector<T, From, To>,
            typename AccumBuilderH<T, TypeList<Tail...>, Max>::Type
        >
    {
        static_assert(To <= Max, "Max range must not exceed the limit");
    };

    template <typename T, int32_t From, int32_t To, int32_t Max>
    struct AccumBuilderH<T, TypeList<MaxRange<From, To>>, Max>
    {
        using Type = TL<MaxVector<T, From, To>>;
        static_assert(To <= Max, "Max range must not exceed the limit");
    };


    template <typename T, int32_t Max>
    struct AccumBuilderH<T, TypeList<>, Max>: TypeP<TypeList<EmptyVector<T>>>
    {};



    template <typename List, int32_t Offset> struct ShiftRangeList;

    template <int32_t From, int32_t To, typename... Tail, int32_t Offset>
    struct ShiftRangeList<TypeList<SumRange<From, To>, Tail...>, Offset>
    {
        using Type = MergeLists<
                SumRange<From + Offset, To + Offset>,
                typename ShiftRangeList<TypeList<Tail...>, Offset>::Type
        >;

        using OffsetList = MergeValueLists<
                IntList<From + Offset>,
                typename ShiftRangeList<TypeList<Tail...>, Offset>::OffsetList
        >;
    };


    template <int32_t From, int32_t To, int32_t Offset>
    struct ShiftRangeList<SumRange<From, To>, Offset>
    {
        using Type = TL<
                SumRange<From + Offset, To + Offset>
        >;

        using OffsetList = IntList<From + Offset>;
    };

    template <int32_t From, int32_t To, typename... Tail, int32_t Offset>
    struct ShiftRangeList<TypeList<MaxRange<From, To>, Tail...>, Offset>
    {
        using Type = MergeLists<
                MaxRange<From + Offset, To + Offset>,
                typename ShiftRangeList<TypeList<Tail...>, Offset>::Type
        >;

        using OffsetList = typename MergeValueLists<
                IntList<From + Offset>,
                typename ShiftRangeList<TypeList<Tail...>, Offset>::OffsetList
        >::Type;
    };


    template <int32_t From, int32_t To, int32_t Offset>
    struct ShiftRangeList<MaxRange<From, To>, Offset>
    {
        using Type = TL<
                MaxRange<From + Offset, To + Offset>
        >;

        using OffsetList = IntList<From + Offset>;
    };



    template <int32_t Offset>
    struct ShiftRangeList<TypeList<>, Offset> {
        using Type       = TypeList<>;
        using OffsetList = IntList<>;
    };


    template <typename RangeList> struct GlueRanges;

    template <int32_t From, int32_t Middle, int32_t To, typename... Tail>
    struct GlueRanges<TypeList<SumRange<From, Middle>, SumRange<Middle, To>, Tail...>> {
        static_assert(From >= 0, "From must be >= 0");
        static_assert(Middle > From, "To must be > From");
        static_assert(To > Middle, "To must be > From");

        using Type = typename GlueRanges<TypeList<SumRange<From, To>, Tail...>>::Type;
    };

    template <int32_t From, int32_t To, typename... Tail>
    struct GlueRanges<TypeList<SumRange<From, To>, Tail...>> {
        static_assert(From >= 0, "From must be >= 0");
        static_assert(To > From, "To must be > From");

        using Type = MergeLists<
                SumRange<From, To>,
                typename GlueRanges<TypeList<Tail...>>::Type
        >;
    };


    template <int32_t From, int32_t Middle, int32_t To, typename... Tail>
    struct GlueRanges<TypeList<MaxRange<From, Middle>, MaxRange<Middle, To>, Tail...>> {
        static_assert(From >= 0, "From must be >= 0");
        static_assert(Middle > From, "To must be > From");
        static_assert(To > Middle, "To must be > From");

        using Type = typename GlueRanges<TypeList<MaxRange<From, To>, Tail...>>::Type;
    };

    template <int32_t From, int32_t To, typename... Tail>
    struct GlueRanges<TypeList<MaxRange<From, To>, Tail...>> {
        static_assert(From >= 0, "From must be >= 0");
        static_assert(To > From, "To must be > From");

        using Type = MergeLists<
                MaxRange<From, To>,
                typename GlueRanges<TypeList<Tail...>>::Type
        >;
    };

    template <>
    struct GlueRanges<TL<>> {
        using Type = TL<>;
    };


    template <int32_t Max, typename List>
    struct CheckRangeList;

    template <int32_t Max, typename Head, typename... Tail>
    struct CheckRangeList<Max, TL<Head, Tail...>> {
        static const bool Value = CheckRangeList<Max, TL<Tail...>>::Value;
    };

    template <int32_t Max, int32_t From, int32_t To>
    struct CheckRangeList<Max, TL<SumRange<From, To>>> {
        static const bool Value = To <= Max;
    };

    template <int32_t Max, int32_t From, int32_t To>
    struct CheckRangeList<Max, TL<MaxRange<From, To>>> {
        static const bool Value = To <= Max;
    };

    template <int32_t Max>
    struct CheckRangeList<Max, TL<>> {
        static const bool Value = true;
    };


    template <typename List> struct MakeTuple;

    template <typename... Types>
    struct MakeTuple<TypeList<Types...>> {
        using Type = std::tuple<Types...>;
    };

    template <typename... Types>
    struct MakeTuple<std::tuple<Types...>> {
        using type = std::tuple<Types...>;
    };
}








template <typename BranchStruct, typename LeafStructList, typename RangeList, int32_t Offset> struct RangeListBuilder;

template <
    typename BranchStruct,
    typename LeafStruct, typename... LTail,
    typename RangeList,
    int32_t Offset
>
struct RangeListBuilder<BranchStruct, TypeList<LeafStruct, LTail...>, RangeList, Offset> {
private:
    using ShiftedRangeList = typename detail::ShiftRangeList<
            typename ListHead<RangeList>::Type,
            Offset
    >::Type;

    using RangeOffsetList = typename detail::ShiftRangeList<
            typename ListHead<RangeList>::Type,
            Offset
    >::OffsetList;

public:
    using Type = MergeLists<
            ShiftedRangeList,
            typename RangeListBuilder<
                BranchStruct,
                TypeList<LTail...>,
                typename ListTail<RangeList>::Type,
                Offset + IndexesSize<LeafStruct>::Value
            >::Type
    >;

    using OffsetList = MergeLists<
            TL<RangeOffsetList>,
            typename RangeListBuilder<
                BranchStruct,
                TypeList<LTail...>,
                typename ListTail<RangeList>::Type,
                Offset + IndexesSize<LeafStruct>::Value
            >::OffsetList
    >;
};


template <typename BranchStruct, typename LeafStruct, typename RangeList, int32_t Offset>
struct RangeListBuilder {
    using Type       = typename detail::ShiftRangeList<RangeList, Offset>::Type;
    using OffsetList = typename detail::ShiftRangeList<RangeList, Offset>::OffsetList;
};

template <
    typename BranchStruct,
    typename RangeList,
    int32_t Offset
>
struct RangeListBuilder<BranchStruct, TypeList<>, RangeList, Offset> {
    using Type       = TypeList<>;
    using OffsetList = TypeList<>;
};


/**
 * Converts range list from leaf node format to branch node format, that has different layout.
 * Several leaf structs may belong to one branch struct.
 */

template <typename BranchStructList, typename LeafStructLists, typename RangeList, int32_t Offset = 0> struct BranchNodeRangeListBuilder;

template <
    typename BranchStruct, typename... BTail,
    typename LeafStruct, typename... LTail,
    typename RangeList, typename... RTail,
    int32_t Offset
>
struct BranchNodeRangeListBuilder<TypeList<BranchStruct, BTail...>, TypeList<LeafStruct, LTail...>, TypeList<RangeList, RTail...>, Offset>
{
    using List = typename RangeListBuilder<
            BranchStruct,
            LeafStruct,
            RangeList,
            Offset
    >::Type;

    using RangeOffsetList = typename RangeListBuilder<
            BranchStruct,
            LeafStruct,
            RangeList,
            Offset
    >::OffsetList;

    static_assert(v1::bt::detail::CheckRangeList<IndexesSize<BranchStruct>::Value, List>::Value, "RangeList exceeds PackedStruct size");

    using Type = MergeLists<
            TL<
                typename v1::bt::detail::GlueRanges<List>::Type
            >,
            typename BranchNodeRangeListBuilder<
                TypeList<BTail...>,
                TypeList<LTail...>,
                TypeList<RTail...>,
                0
            >::Type
    >;

    using OffsetList = MergeLists<
            TL<RangeOffsetList>,
            typename BranchNodeRangeListBuilder<
                TypeList<BTail...>,
                TypeList<LTail...>,
                TypeList<RTail...>,
                0
            >::OffsetList
    >;
};


template <int32_t Offset>
struct BranchNodeRangeListBuilder<TypeList<>, TypeList<>, TypeList<>, Offset>
{
    using Type = TypeList<>;
    using OffsetList = TypeList<>;
};



/**
 * Converts branch node range list to list of IndexVector types
 */
template <typename BranchStructList, typename RangeLists> struct IteratorBranchNodeEntryBuilder;

template <typename BranchStruct, typename... BTail, typename RangeList, typename... RTail>
struct IteratorBranchNodeEntryBuilder<TL<BranchStruct, BTail...>, TL<RangeList, RTail...>> {
    using Type = MergeLists<
            typename detail::MakeTuple<
                typename detail::AccumBuilderH<
                    typename v1::AccumType<BranchStruct>::Type,
                    RangeList,
                    IndexesSize<BranchStruct>::Value
                >::Type
            >::Type,
            typename IteratorBranchNodeEntryBuilder<TL<BTail...>, TL<RTail...>>::Type
    >;
};

template <>
struct IteratorBranchNodeEntryBuilder<TL<>, TL<>> {
    using Type = TL<>;
};







namespace {

    template <typename RangeList, int32_t Idx = 0> struct IndexRangeProc;

    template <typename T, int32_t From, int32_t To, typename... Tail, int32_t Idx, template <typename, int32_t, int32_t> class IndexVector>
    struct IndexRangeProc<std::tuple<IndexVector<T, From, To>, Tail...>, Idx> {

        using RtnType = T;

        template <typename RangeList>
        static T& value(int32_t index, RangeList&& accum)
        {
            if (index >= From && index < To)
            {
                return std::get<Idx>(accum)[index - From];
            }
            else {
                return IndexRangeProc<std::tuple<Tail...>, Idx + 1>::value(index, std::forward<RangeList>(accum));
            }
        }

        template <int32_t Index, typename RangeList>
        static T& value(RangeList&& accum)
        {
            if (Index >= From && Index < To)
            {
                return std::get<Idx>(accum)[Index - From];
            }
            else {
                return IndexRangeProc<std::tuple<Tail...>, Idx + 1>::template value<Index>(std::forward<RangeList>(accum));
            }
        }
    };





    template <typename T, int32_t From, int32_t To, int32_t Idx, template <typename, int32_t, int32_t> class IndexVector>
    struct IndexRangeProc<std::tuple<IndexVector<T, From, To>>, Idx> {

        using RtnType = T;

        template <typename RangeList>
        static auto value(int32_t index, RangeList&& accum)
        {
            if (index >= From && index < To)
            {
                return std::get<Idx>(accum)[index - From];
            }
            else {
                throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<index);
            }
        }

        template <int32_t Index, typename RangeList>
        static auto value(RangeList&& accum)
        {
            if (Index >= From && Index < To)
            {
                return std::get<Idx>(accum)[Index - From];
            }
            else {
                throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<Index);
            }
        }
    };



    template <typename T, int32_t Idx>
    struct IndexRangeProc<std::tuple<EmptyVector<T>>, Idx> {

        using RtnType = T;

        template <typename RangeList>
        static RtnType& value(int32_t index, RangeList&& accum)
        {
            throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<index);
        }

        template <int32_t Index, typename RangeList>
        static RtnType& value(RangeList&& accum)
        {
            throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<Index);
        }
    };



    template <typename AccumTuple, int32_t Offset, int32_t Tmp = 0> struct SearchForAccumItem;

    template <typename T, int32_t From, int32_t To, int32_t Offset, int32_t Tmp, typename... Tail>
    struct SearchForAccumItem<std::tuple<SumVector<T, From, To>, Tail...>, Offset, Tmp>
    {
        static constexpr int32_t Idx = (Offset >= From && Offset < To) ?
                Tmp :
                SearchForAccumItem<std::tuple<Tail...>, Offset, Tmp + 1>::Idx;
    };


    template <typename T, int32_t From, int32_t To, int32_t Offset, int32_t Tmp, typename... Tail>
    struct SearchForAccumItem<std::tuple<MaxVector<T, From, To>, Tail...>, Offset, Tmp>
    {
        static constexpr int32_t Idx = (Offset >= From && Offset < To) ?
                Tmp :
                SearchForAccumItem<std::tuple<Tail...>, Offset, Tmp + 1>::Idx;
    };




    template <int32_t Offset, int32_t Tmp>
    struct SearchForAccumItem<std::tuple<>, Offset, Tmp> {
        static constexpr int32_t Idx = -1;
    };

}


template <
    typename LeafStructList,
    typename LeafPath,
    typename AccumType
>
struct AccumItem: public LeafToBranchIndexTranslator<LeafStructList, LeafPath, 0> {
public:
    using Base = LeafToBranchIndexTranslator<LeafStructList, LeafPath, 0>;

    static constexpr int32_t BranchIdx  = v1::list_tree::LeafCountInf<LeafStructList, LeafPath, 2>::Value - Base::LocalLeafOffset;

    static constexpr int32_t LeafPrefix = Base::BranchIndex;

    using AccumRangeList = typename std::tuple_element<BranchIdx, AccumType>::type;

    template <int32_t Offset>
    using Vector = typename std::tuple_element<
        SearchForAccumItem<AccumRangeList, Offset>::Idx,
        AccumRangeList
    >::type;

public:

    template <int32_t Offset>
    static
    Vector<Offset>& item(AccumType& accum)
    {
        return std::get<SearchForAccumItem<AccumRangeList, Offset>::Idx>(std::get<BranchIdx>(accum));
    }

    template <int32_t Offset>
    static
    const Vector<Offset>& item(const AccumType& accum)
    {
        return std::get<SearchForAccumItem<AccumRangeList, Offset>::Idx>(std::get<BranchIdx>(accum));
    }

    template <typename AccumTypeT>
    static auto value(int32_t index, AccumTypeT&& accum)
    {
        return IndexRangeProc<AccumRangeList>::value(
                index + LeafPrefix,
                std::get<BranchIdx>(std::forward<AccumTypeT>(accum))
        );
    }

    template <int32_t Index, typename AccumTypeT>
    static auto value(AccumTypeT&& accum)
    {
        return IndexRangeProc<AccumRangeList>::template value<Index + LeafPrefix>(std::forward<AccumRangeList>(std::get<BranchIdx>(std::forward<AccumTypeT>(accum))));
    }
};




template <typename LeafStructList, typename LeafPath>
struct PackedStructValueTypeH {
    static const int32_t LeafIdx = v1::list_tree::LeafCount<LeafStructList, LeafPath>::Value;
    using PkdStruct = Select<LeafIdx, Linearize<LeafStructList>>;

    using Type = typename AccumType<PkdStruct>::Type;
};







template <typename Tuple> struct StreamTupleHelper;


namespace {

    template <typename T, T A, T B>
    struct min {
        static const T Value = A < B ? A : B;
    };


    template <
        typename T1,
        typename T2,
        int32_t Idx = 0,
        int32_t Max = min<int32_t, std::tuple_size<T1>::value, std::tuple_size<T2>::value>::Value
    >
    struct StreamT2TCvtHelper {

        static void _convert(T1& t1, const T2& t2)
        {
            std::get<Idx>(t1) = std::get<Idx>(t2);

            StreamT2TCvtHelper<T1, T2, Idx + 1, Max>::_convert(t1, t2);
        }
    };


    template <typename T1, typename T2, int32_t Idx>
    struct StreamT2TCvtHelper<T1, T2, Idx, Idx> {

        static void _convert(T1& t1, const T2& t2){}
    };
}



template <typename Tuple>
struct StreamTupleHelper {

    template <typename... Args>
    static Tuple convert(Args&&... args)
    {
        Tuple tuple;

        _convert<0>(tuple, std::forward<Args>(args)...);

        return tuple;
    }

    template <typename... Args>
    static Tuple convertAll(Args&&... args)
    {
        static_assert(sizeof...(args) == std::tuple_size<Tuple>::value, "Number of arguments does not match target tuple size");

        return convert(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static Tuple convertTuple(const std::tuple<Args...>& other)
    {
        Tuple tuple;

        StreamT2TCvtHelper<Tuple, std::tuple<Args...>>::_convert(tuple, other);

        return tuple;
    }

    template <typename... Args>
    static Tuple convertTupleAll(const std::tuple<Args...>& other)
    {
        static_assert(sizeof...(Args) == std::tuple_size<Tuple>::value, "Source tuple size does not match target tuple size");

        return convertTuple(other);
    }

private:

    template <int32_t Idx2>
    static void _convert(Tuple& tuple) {}

    template <int32_t Idx2, typename Arg, typename... Args>
    static void _convert(Tuple& tuple, Arg&& head, Args&&... tail)
    {
        std::get<Idx2>(tuple) = head;

        _convert<Idx2 + 1>(tuple, std::forward<Args>(tail)...);
    }
};





}
}}
