
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_ACCUMULATORS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_ACCUMULATORS_HPP

#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/list/linearize.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/core/exceptions/bounds.hpp>

#include <memoria/prototypes/bt/tools/bt_size_list_builder.hpp>


#include <iostream>
#include <tuple>

namespace memoria   {
namespace bt        {


template <Int From_, Int To_ = From_ +1>
struct IndexRange {
	static const Int From = From_;
	static const Int To = To_;
};

template <typename T> struct IndexesH;

template <typename T>
using Indexes = typename IndexesH<T>::Type;

template <Int Head, Int... Tail>
struct IndexesH<IntList<Head, Tail...>>:
	MergeListsMF<IndexRange<Head>, typename IndexesH<IntList<Tail...>>::Type>
{};


template <typename IndexRangeTree>
using FlattenIndexRangeTree = Linearize<IndexRangeTree, 3>;

template <Int Head>
struct IndexesH<IntList<Head>>:
	TypeP<TypeList<IndexRange<Head>>>
{};




template <typename T, typename RangesList>
struct IndexDescr {};

namespace detail {

template <typename T, typename List, Int Max> struct AccumBuilderH;

template <typename T, Int From_, Int To_>
struct IndexVector: public memoria::core::StaticVector<T, To_ - From_> {
	static const Int To 	= To_;
	static const Int From 	= From_;
};

template <typename T>
struct EmptyVector {
	static const Int To 	= 0;
	static const Int From 	= 0;

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


template <typename T, Int From, Int To, typename... Tail, Int Max>
struct AccumBuilderH<T, TypeList<IndexRange<From, To>, Tail...>, Max>:
	MergeListsMF<
		IndexVector<T, From, To>,
		typename AccumBuilderH<T, TypeList<Tail...>, Max>::Type
	>
{
	static_assert(To <= Max, "Index range must not exceed the limit");
};

template <typename T, Int From, Int To, Int Max>
struct AccumBuilderH<T, TypeList<IndexRange<From, To>>, Max>
{
	using Type = TL<IndexVector<T, From, To>>;
	static_assert(To <= Max, "Index range must not exceed the limit");
};


template <typename T, Int Max>
struct AccumBuilderH<T, TypeList<>, Max>:
	TypeP<TypeList<EmptyVector<T>>>
{};



template <typename List, Int Offset> struct ShiftRangeList;

template <Int From, Int To, typename... Tail, Int Offset>
struct ShiftRangeList<TypeList<IndexRange<From, To>, Tail...>, Offset>
{
	using Type = MergeLists<
			IndexRange<From + Offset, To + Offset>,
			typename ShiftRangeList<TypeList<Tail...>, Offset>::Type
	>;

	using OffsetList = typename MergeValueLists<
			IntList<From + Offset>,
			typename ShiftRangeList<TypeList<Tail...>, Offset>::OffsetList
	>::Type;
};


template <Int Offset>
struct ShiftRangeList<TypeList<>, Offset> {
	using Type 		 = TypeList<>;
	using OffsetList = IntList<>;
};


template <typename RangeList> struct GlueRanges;

template <Int From, Int Middle, Int To, typename... Tail>
struct GlueRanges<TypeList<IndexRange<From, Middle>, IndexRange<Middle, To>, Tail...>> {
	static_assert(From >= 0, "From must be >= 0");
	static_assert(Middle > From, "To must be > From");
	static_assert(To > Middle, "To must be > From");

	using Type = typename GlueRanges<TypeList<IndexRange<From, To>, Tail...>>::Type;
};

template <Int From, Int To, typename... Tail>
struct GlueRanges<TypeList<IndexRange<From, To>, Tail...>> {
	static_assert(From >= 0, "From must be >= 0");
	static_assert(To > From, "To must be > From");

	using Type = MergeLists<
			IndexRange<From, To>,
			typename GlueRanges<TypeList<Tail...>>::Type
	>;
};

template <>
struct GlueRanges<TL<>> {
	using Type = TL<>;
};


template <Int Max, typename List>
struct CheckRangeList;

template <Int Max, typename Head, typename... Tail>
struct CheckRangeList<Max, TL<Head, Tail...>> {
	static const bool Value = CheckRangeList<Max, TL<Tail...>>::Value;
};

template <Int Max, Int From, Int To>
struct CheckRangeList<Max, TL<IndexRange<From, To>>> {
	static const bool Value = To <= Max;
};

template <Int Max>
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





template <typename PkdStruct>
struct AccumType {
	using Type = BigInt;
};

template <typename PkdStruct>
struct IndexesSize {
	static const Int Value = PkdStruct::Indexes;
};



template <typename BranchStruct, typename LeafStructList, typename RangeList, Int Offset> struct RangeListBuilder;

template <
	typename BranchStruct,
	typename LeafStruct, typename... LTail,
	typename RangeList,
	Int Offset
>
struct RangeListBuilder<BranchStruct, TypeList<LeafStruct, LTail...>, RangeList, Offset> {
private:
	using ShiftedRangeList = typename memoria::bt::detail::ShiftRangeList<
			typename ListHead<RangeList>::Type,
			Offset
	>::Type;

	using RangeOffsetList = typename memoria::bt::detail::ShiftRangeList<
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


template <typename BranchStruct, typename LeafStruct, typename RangeList, Int Offset>
struct RangeListBuilder {
	using Type 		 = typename memoria::bt::detail::ShiftRangeList<RangeList, Offset>::Type;
	using OffsetList = typename memoria::bt::detail::ShiftRangeList<RangeList, Offset>::OffsetList;
};

template <
	typename BranchStruct,
	typename RangeList,
	Int Offset
>
struct RangeListBuilder<BranchStruct, TypeList<>, RangeList, Offset> {
	using Type 		 = TypeList<>;
	using OffsetList = TypeList<>;
};


/**
 * Converts range list from leaf node format to branch node format, that has different layout.
 * Several leaf structs may belong to one branch struct.
 */

template <typename BranchStructList, typename LeafStructLists, typename RangeList, Int Offset = 1> struct BranchNodeRangeListBuilder;

template <
	typename BranchStruct, typename... BTail,
	typename LeafStruct, typename... LTail,
	typename RangeList, typename... RTail,
	Int Offset
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

	static_assert(memoria::bt::detail::CheckRangeList<IndexesSize<BranchStruct>::Value, List>::Value, "RangeList exceeds PackedStruct size");

	using Type = MergeLists<
			TL<
				typename memoria::bt::detail::GlueRanges<List>::Type
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


template <Int Offset>
struct BranchNodeRangeListBuilder<TypeList<>, TypeList<>, TypeList<>, Offset>
{
	using Type = TypeList<>;
	using OffsetList = TypeList<>;
};



/**
 * Converts branch node range list to list of IndexVector types
 */
template <typename BranchStructList, typename RangeLists> struct IteratorAccumulatorBuilder;

template <typename BranchStruct, typename... BTail, typename RangeList, typename... RTail>
struct IteratorAccumulatorBuilder<TL<BranchStruct, BTail...>, TL<RangeList, RTail...>> {
	using Type = MergeLists<
			typename memoria::bt::detail::MakeTuple<
				typename memoria::bt::detail::AccumBuilderH<
					typename memoria::bt::AccumType<BranchStruct>::Type,
					RangeList,
					IndexesSize<BranchStruct>::Value
				>::Type
			>::Type,
			typename IteratorAccumulatorBuilder<TL<BTail...>, TL<RTail...>>::Type
	>;
};

template <>
struct IteratorAccumulatorBuilder<TL<>, TL<>> {
	using Type = TL<>;
};





namespace detail {

template <typename RangeList, Int Idx = 0> struct IndexRangeProc;

template <typename T, Int From, Int To, typename... Tail, Int Idx>
struct IndexRangeProc<std::tuple<IndexVector<T, From, To>, Tail...>, Idx> {

	using RtnType = T;

	template <typename RangeList>
	static T& value(Int index, RangeList&& accum)
	{
		if (index >= From && index < To)
		{
			return std::get<Idx>(accum)[index - From];
		}
		else {
			return IndexRangeProc<std::tuple<Tail...>, Idx + 1>::value(index, std::forward<RangeList>(accum));
		}
	}

	template <Int Index, typename RangeList>
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


template <typename T, Int From, Int To, Int Idx>
struct IndexRangeProc<std::tuple<IndexVector<T, From, To>>, Idx> {

	using RtnType = T;

	template <typename RangeList>
	static T& value(Int index, RangeList&& accum)
	{
		if (index >= From && index < To)
		{
			return std::get<Idx>(accum)[index - From];
		}
		else {
			throw vapi::BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<index);
		}
	}

	template <Int Index, typename RangeList>
	static T& value(RangeList&& accum)
	{
		if (Index >= From && Index < To)
		{
			return std::get<Idx>(accum)[Index - From];
		}
		else {
			throw vapi::BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<Index);
		}
	}
};



template <typename T, Int Idx>
struct IndexRangeProc<std::tuple<EmptyVector<T>>, Idx> {

	using RtnType = T;

	template <typename RangeList>
	static RtnType& value(Int index, RangeList&& accum)
	{
		throw vapi::BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<index);
	}

	template <Int Index, typename RangeList>
	static RtnType& value(RangeList&& accum)
	{
		throw vapi::BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<Index);
	}
};



template <typename AccumTuple, Int Offset, Int Tmp = 0> struct SearchForAccumItem;

template <typename T, Int From, Int To, Int Offset, Int Tmp, typename... Tail>
struct SearchForAccumItem<std::tuple<IndexVector<T, From, To>, Tail...>, Offset, Tmp>
{
	static constexpr Int Idx = (Offset >= From && Offset < To) ?
					Tmp :
					SearchForAccumItem<std::tuple<Tail...>, Offset, Tmp + 1>::Idx;
};






template <Int Offset, Int Tmp>
struct SearchForAccumItem<std::tuple<>, Offset, Tmp> {
	static constexpr Int Idx = -1;
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

	static constexpr Int BranchIdx  = memoria::list_tree::LeafCountInf<LeafStructList, LeafPath, 2>::Value - Base::LocalLeafOffset;

	static constexpr Int LeafPrefix = Base::BranchIndex;

	using AccumRangeList = typename std::tuple_element<BranchIdx, AccumType>::type;

	template <Int Offset>
	using Vector = typename std::tuple_element<
		detail::SearchForAccumItem<AccumRangeList, Offset>::Idx,
		AccumRangeList
	>::type;

public:

	template <Int Offset>
	static
	Vector<Offset>& item(AccumType& accum)
	{
		return std::get<
				detail::SearchForAccumItem<AccumRangeList, Offset>::Idx
		>(std::get<BranchIdx>(accum));
	}

	template <typename AccumTypeT>
	static typename detail::IndexRangeProc<AccumRangeList>::RtnType& value(Int index, AccumTypeT&& accum)
	{
		return detail::IndexRangeProc<AccumRangeList>::value(index + LeafPrefix, std::forward<AccumRangeList>(std::get<BranchIdx>(std::forward<AccumTypeT>(accum))));
	}

	template <typename AccumTypeT>
	static const typename detail::IndexRangeProc<AccumRangeList>::RtnType& cvalue(Int index, AccumTypeT&& accum)
	{
		return detail::IndexRangeProc<AccumRangeList>::value(index + LeafPrefix, std::forward<AccumRangeList>(std::get<BranchIdx>(std::forward<AccumTypeT>(accum))));
	}


	template <Int Index, typename AccumTypeT>
	static typename detail::IndexRangeProc<AccumRangeList>::RtnType& value(AccumTypeT&& accum)
	{
		return detail::IndexRangeProc<AccumRangeList>::template value<Index + LeafPrefix>(std::forward<AccumRangeList>(std::get<BranchIdx>(std::forward<AccumTypeT>(accum))));
	}
};









}
}

#endif

