
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


#include <ostream>
#include <tuple>

namespace memoria   {
namespace bt        {


template <Int From, Int To = From +1>
struct IndexRange {};

template <typename T> struct IndexesH;

template <typename T>
using Indexes = typename IndexesH<T>::Type;

template <Int Head, Int... Tail>
struct IndexesH<IntList<Head, Tail...>>:
	MergeListsMF<IndexRange<Head>, typename IndexesH<IntList<Tail...>>::Type>
{};


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
};


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
struct ShiftRangeList<TypeList<IndexRange<From, To>, Tail...>, Offset> {
	using Type = MergeLists<
			IndexRange<From + Offset, To + Offset>,
			typename ShiftRangeList<TypeList<Tail...>, Offset>::Type
	>;
};


template <Int Offset>
struct ShiftRangeList<TypeList<>, Offset> {
	using Type = TypeList<>;
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
	using Type = MergeLists<
			typename memoria::bt::detail::ShiftRangeList<
				typename ListHead<RangeList>::Type,
				Offset
			>::Type,
			typename RangeListBuilder<
				BranchStruct,
				TypeList<LTail...>,
				typename ListTail<RangeList>::Type,
				Offset + IndexesSize<LeafStruct>::Value
			>::Type
	>;
};


template <typename BranchStruct, typename LeafStruct, typename RangeList, Int Offset>
struct RangeListBuilder {
	using Type = typename memoria::bt::detail::ShiftRangeList<RangeList, Offset>::Type;
};

template <
	typename BranchStruct,
	typename RangeList,
	Int Offset
>
struct RangeListBuilder<BranchStruct, TypeList<>, RangeList, Offset> {
	using Type = TypeList<>;
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

	static_assert(memoria::bt::detail::CheckRangeList<IndexesSize<BranchStruct>::Value, List>::Value, "Invalid RangeList");

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
};


template <Int Offset>
struct BranchNodeRangeListBuilder<TypeList<>, TypeList<>, TypeList<>, Offset>
{
	using Type = TypeList<>;
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

template <typename RangeList, Int Idx = 0, typename RtnT = void> struct IndexRangeProc;

template <typename T, Int From, Int To, typename... Tail, Int Idx, typename RtnT>
struct IndexRangeProc<std::tuple<IndexVector<T, From, To>, Tail...>, Idx, RtnT> {

	using RtnType = T;

	template <typename RangeList>
	static T& value(Int index, RangeList&& accum)
	{
		if (index >= From && index < To)
		{
			return std::get<Idx>(accum)[index - From];
		}
		else {
			return IndexRangeProc<std::tuple<Tail...>, Idx + 1, T>::value(index, std::forward<RangeList>(accum));
		}
	}
};



template <Int Idx, typename RtnT>
struct IndexRangeProc<std::tuple<>, Idx, RtnT> {

	using RtnType = RtnT;

	template <typename RangeList>
	static RtnT& value(Int index, RangeList&& accum)
	{
		throw vapi::BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<index);
	}
};

template <typename T, Int Idx, typename RtnT>
struct IndexRangeProc<std::tuple<EmptyVector<T>>, Idx, RtnT> {

	using RtnType = RtnT;

	template <typename RangeList>
	static RtnT& value(Int index, RangeList&& accum)
	{
		throw vapi::BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid index value: "<<index);
	}
};

template <typename T> struct JustDumpT;

}




template <
	typename LeafStructList,
	typename LeafRangeList,
	typename LeafPath,
	typename AccumType
>
struct AccumItem {
private:
	using LeafOffsets 	= typename LeafOffsetListBuilder<LeafStructList>::Type;

	using Leafs = Linearize<LeafStructList, 2>;

	static constexpr Int LeafIdx 			= memoria::list_tree::LeafCount<LeafStructList, LeafPath>::Value;
	static constexpr Int BranchIdx 			= memoria::list_tree::LeafCount<LeafStructList, LeafPath, 2>::Value;
	static constexpr Int LocalLeafOffset 	= FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;

	using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

	static constexpr Int LeafPrefix = GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;

	using RangeList = typename std::tuple_element<BranchIdx, AccumType>::type;

public:

	template <typename AccumTypeT>
	static typename detail::IndexRangeProc<RangeList>::RtnType& value(Int index, AccumTypeT&& accum)
	{
		return detail::IndexRangeProc<RangeList>::value(index + LeafPrefix + 1, std::forward<RangeList>(std::get<BranchIdx>(std::forward<AccumTypeT>(accum))));
	}
};







}
}

#endif

