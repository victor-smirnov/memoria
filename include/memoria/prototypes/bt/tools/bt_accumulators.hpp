
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_ACCUMULATORS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_ACCUMULATORS_HPP

#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/types/list/append.hpp>

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


template <typename T, Int From, Int To, typename... Tail, Int Max>
struct AccumBuilderH<T, TypeList<IndexRange<From, To>, Tail...>, Max>:
	MergeListsMF<
		IndexVector<T, From, To>,
		typename AccumBuilderH<T, TypeList<Tail...>, Max>::Type
	>
{
	static_assert(To <= Max, "Index range must not exceed the limit");
};


template <typename T, Int Max>
struct AccumBuilderH<T, TypeList<>, Max>:
	TypeP<TypeList<>>
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




template <typename BranchStructList, typename RangeLists> struct IteratorAccumulatorBuilder;

template <typename BranchStruct, typename... BTail, typename RangeList, typename... RTail>
struct IteratorAccumulatorBuilder<TL<BranchStruct, BTail...>, TL<RangeList, RTail...>> {
	using Type = MergeLists<
			TL<
				typename memoria::bt::detail::AccumBuilderH<
					typename memoria::bt::AccumType<BranchStruct>::Type,
					RangeList,
					IndexesSize<BranchStruct>::Value
				>::Type
			>,
			typename IteratorAccumulatorBuilder<TL<BTail...>, TL<RTail...>>::Type
	>;
};

template <>
struct IteratorAccumulatorBuilder<TL<>, TL<>> {
	using Type = TL<>;
};




}
}

#endif

