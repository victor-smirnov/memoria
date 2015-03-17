
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


}


template <typename PkdStruct>
struct AccumType {
	using Type = BigInt;
};

template <typename PkdStruct>
struct IndexesSize {
	static const Int Value = PkdStruct::Indexes;
};

/**
 * Builds type list for accumulator
 *
 */

template <typename PkdStructList, typename IdxList> struct AccumListBuilderH;

template <typename PkdStruct, typename... STail, typename IHead, typename... ITail>
struct AccumListBuilderH<TypeList<PkdStruct, STail...>, TypeList<IHead, ITail...>>:
	MergeListsMF<
			typename detail::AccumBuilderH<
				typename AccumType<PkdStruct>::Type,
				IHead,
				IndexesSize<PkdStruct>::Value
			>::Type,
			typename AccumListBuilderH<TypeList<STail...>, TypeList<ITail...>>::Type
	>
{};


template <>
struct AccumListBuilderH<TypeList<>, TypeList<>>:
	TypeP<TypeList<>>
{};





}
}

#endif

