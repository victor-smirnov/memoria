
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SRTREAMDSCR_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SRTREAMDSCR_FACTORY_HPP

#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/list/linearize.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/core/types/algo/fold.hpp>
#include <memoria/core/exceptions/bounds.hpp>



#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/fse_max/packed_fse_max_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_dense_tree.hpp>

#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/exint_codec.hpp>

#include <iostream>
#include <tuple>
#include <utility>

namespace memoria   {
namespace bt        {


template <PkdSearchType SearchType, typename KeyType_, Int Indexes_>
struct IdxSearchType {
	static constexpr PkdSearchType 	Value 		= SearchType;
	static constexpr Int 			Indexes 	= Indexes_;

	using KeyType = KeyType_;
};

template <typename T> struct FSEBranchStructTF;


template <typename KeyType, Int Indexes>
struct FSEBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>> {
	using Type = PkdFQTreeT<KeyType, Indexes>;
};

template <typename KeyType, Int Indexes>
struct FSEBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {
	using Type = PkdFMTreeT<KeyType, Indexes>;
};


template <typename T> struct VLQBranchStructTF;

template <typename KeyType, Int Indexes>
struct VLQBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>> {
	using Type = PkdVQTreeT<KeyType, Indexes, UByteI7Codec>;
};

template <typename KeyType, Int Indexes>
struct VLQBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {
	using Type = PkdFMTreeT<KeyType, Indexes>;
};


template <typename T> struct VLDBranchStructTF;

template <typename KeyType, Int Indexes>
struct VLDBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>> {
	using Type = PkdVDTreeT<KeyType, Indexes, UByteI7Codec>;
};

template <typename KeyType, Int Indexes>
struct VLDBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {
	using Type = PkdFMTreeT<KeyType, Indexes>;
};





namespace {

	template <typename State, typename Element>
	struct PackedStructsIndexesSum
	{
		static constexpr Int Value = StructSizeProvider<Element>::Value + State::Value;
	};

	template <typename Element>
	struct PackedStructsIndexesSum<EmptyType, Element>
	{
		static constexpr Int Value = StructSizeProvider<Element>::Value;
	};

	template <>
	struct PackedStructsIndexesSum<EmptyType, EmptyType>
	{
		static constexpr Int Value = 0;
	};



	template <typename LeafStruct, template <typename, Int> class BranchStructTF>
	struct BTBuildBranchStruct
	{
		using Type = typename BranchStructTF<
				BigInt,
				StructSizeProvider<LeafStruct>::Value
		>::Type;
	};

	template <typename... LeafStructs, template <typename, Int> class BranchStructTF>
	struct BTBuildBranchStruct<TL<LeafStructs...>, BranchStructTF>
	{
		using Type = typename BranchStructTF<
				BigInt,
				FoldTLRight<TL<LeafStructs...>, PackedStructsIndexesSum>::Type::Value
		>::Type;
	};



	template <typename LeafStruct, template <typename, Int> class BranchStructTF>
	struct BTBuildBranchStructSS: HasType<
		typename BranchStructTF<
				BigInt,
				StructSizeProvider<LeafStruct>::Value + 1
		>::Type
	>{};

	template <typename... LeafStructs, template <typename, Int> class BranchStructTF>
	struct BTBuildBranchStructSS<TL<LeafStructs...>, BranchStructTF>: HasType<
		typename BranchStructTF<
				BigInt,
				FoldTLRight<TL<LeafStructs...>, PackedStructsIndexesSum>::Type::Value + 1
		>::Type
	>{};



	template <typename List> struct CheckKeysHaveSameType;

	template <typename PkdStruct1, typename PkdStruct2, typename... Tail>
	struct CheckKeysHaveSameType<TL<PkdStruct1, PkdStruct2, Tail...>>
	{
		static constexpr PkdSearchType SearchType1 = PkdSearchTypeProvider<PkdStruct1>::Value;
		static constexpr PkdSearchType SearchType2 = PkdSearchTypeProvider<PkdStruct2>::Value;

		static constexpr bool Value = SearchType1 == SearchType2 && CheckKeysHaveSameType<
				TL<PkdStruct2, Tail...>
		>::Value;
	};

	template <typename PkdStruct1, typename PkdStruct2>
	struct CheckKeysHaveSameType<TL<PkdStruct1, PkdStruct2>>
	{
		static constexpr PkdSearchType SearchType1 = PkdSearchTypeProvider<PkdStruct1>::Value;
		static constexpr PkdSearchType SearchType2 = PkdSearchTypeProvider<PkdStruct2>::Value;

		static constexpr bool Value = SearchType1 == SearchType2;
	};

	template <typename PkdStruct>
	struct CheckKeysHaveSameType<TL<PkdStruct>>: HasValue<bool, true> {};


	template <>
	struct CheckKeysHaveSameType<TL<>>: HasValue<bool, true> {};


	template <typename List> struct SumIndexes;

	template <PkdSearchType SearchType, typename KeyType, Int Indexes, typename... Tail>
	struct SumIndexes<TL<IdxSearchType<SearchType, KeyType, Indexes>, Tail...>> {
		static constexpr Int Value = Indexes + SumIndexes<TL<Tail...>>::Value;
	};

	template <>
	struct SumIndexes<TL<>> {
		static constexpr Int Value = 0;
	};


	template <typename T1, typename T2, bool LT = sizeof(T1) < sizeof(T2) > struct SeletMaxTypeT;

	template <typename T1, typename T2>
	using SeletMaxType = typename SeletMaxTypeT<T1, T2>::Type;

	template <typename T1, typename T2>
	struct SeletMaxTypeT<T1, T2, true>: HasType<T2> {};

	template <typename T1, typename T2>
	struct SeletMaxTypeT<T1, T2, false>: HasType<T1> {};


	template <typename List> struct GetSUPSearchKeyTypeT;
	template <typename List> using GetSUPSearchKeyType = typename GetSUPSearchKeyTypeT<List>::Type;

	template <PkdSearchType SearchType, typename KeyType, Int Indexes, typename... Tail>
	struct GetSUPSearchKeyTypeT<TL<IdxSearchType<SearchType, KeyType, Indexes>, Tail...>>: HasType <
		SeletMaxType<
				KeyType,
				GetSUPSearchKeyType<TL<Tail...>>
		>
	> {};

	template <PkdSearchType SearchType, typename KeyType, Int Indexes>
	struct GetSUPSearchKeyTypeT<TL<IdxSearchType<SearchType, KeyType, Indexes>>>: HasType <KeyType> {};




	template <typename List> struct BuildKeyMetadataListT;

	template <typename List>
	using BuildKeyMetadataList = typename BuildKeyMetadataListT<List>::Type;


	template <typename PkdStruct, typename... Tail>
	struct BuildKeyMetadataListT<TL<PkdStruct, Tail...>>: HasType<
		MergeLists<
				IdxSearchType<
					PkdSearchTypeProvider<PkdStruct>::Value,
					typename PkdSearchKeyTypeProvider<PkdStruct>::Type,
					IndexesSize<PkdStruct>::Value
				>,
				BuildKeyMetadataList<TL<Tail...>>
		>
	>{};

	template <typename PkdStruct, typename... Tail1, typename... Tail2>
	struct BuildKeyMetadataListT<TL<TL<PkdStruct, Tail1...>, Tail2...>>
	{
		using List = TL<PkdStruct, Tail1...>;

		static_assert(
				CheckKeysHaveSameType<List>::Value,
				"All grouped together leaf packed structs must be of the same search type"
		);

		using LeafStructGroupKeyMetadataList = BuildKeyMetadataList<List>;

		static constexpr PkdSearchType GroupSearchType = PkdSearchTypeProvider<PkdStruct>::Value;

		static constexpr Int TotalIndexes 	= SumIndexes<LeafStructGroupKeyMetadataList>::Value;
		using LargestKeyType 				= GetSUPSearchKeyType<LeafStructGroupKeyMetadataList>;

		using Type = MergeLists<
				IdxSearchType<
					GroupSearchType,
					LargestKeyType,
					TotalIndexes
				>,
				BuildKeyMetadataList<TL<Tail2...>>
		>;
	};

	template <>
	struct BuildKeyMetadataListT<TL<>> {
		using Type = TL<>;
	};



	template <typename List, template <typename> class BranchStructTF> struct BranchStructListBuilderT;

	template <typename List, template <typename> class BranchStructTF>
	using BranchStructListBuilder = typename BranchStructListBuilderT<List, BranchStructTF>::Type;

	template <typename IdxSearchTypeT, typename... Tail, template <typename> class BranchStructTF>
	struct BranchStructListBuilderT<TL<IdxSearchTypeT, Tail...>, BranchStructTF>: HasType<
		MergeLists<
			typename BranchStructTF<IdxSearchTypeT>::Type,
			BranchStructListBuilder<TL<Tail...>, BranchStructTF>
		>
	> {};

	template <template <typename> class BranchStructTF>
	struct BranchStructListBuilderT<TL<>, BranchStructTF>: HasType<TL<>>{};



	template <typename List> struct IncStreamStartStructsIndexSizeT;
	template <typename List> using IncStreamStartStructsIndexSize = typename IncStreamStartStructsIndexSizeT<List>::Type;

	template <PkdSearchType SearchType, typename KeyType, Int Indexes, typename... Tail>
	struct IncStreamStartStructsIndexSizeT<TL<IdxSearchType<SearchType, KeyType, Indexes>, Tail...>>: HasType<
		MergeLists<
			IdxSearchType<SearchType, KeyType, Indexes + 1>,
			Tail...
		>
	> {};
}




template <typename LeafStructList, template <typename> class BranchStructTF> struct BTStreamDescritorsBuilder;

template <typename LeafStruct, typename... Tail, template <typename> class BranchStructTF>
struct BTStreamDescritorsBuilder<TL<LeafStruct, Tail...>, BranchStructTF>
{
	static_assert(
			PkdSearchTypeProvider<LeafStruct>::Value == PkdSearchType::SUM,
			"First packed struct in each stream must has PkdSearchType::SUM search type. Consider prepending PackedSizedStruct"
	);

	using StructList = TL<LeafStruct, Tail...>;

	using RawKeyMetadataList = BuildKeyMetadataList<StructList>;

	using KeyMetadataList = IncStreamStartStructsIndexSize<RawKeyMetadataList>;

	using Type = BranchStructListBuilder<KeyMetadataList, BranchStructTF>;
};

template <typename LeafStruct, typename... Tail1, typename... Tail2, template <typename> class BranchStructTF>
struct BTStreamDescritorsBuilder<TL<TL<LeafStruct, Tail1...>, Tail2...>, BranchStructTF>
{
	static_assert(
			PkdSearchTypeProvider<LeafStruct>::Value == PkdSearchType::SUM,
			"First packed struct in each stream must has PkdSearchType::SUM search type. Consider prepending PackedSizedStruct"
	);

	using StructList = TL<TL<LeafStruct, Tail1...>, Tail2...>;

	using RawKeyMetadataList = BuildKeyMetadataList<StructList>;

	using KeyMetadataList = IncStreamStartStructsIndexSize<RawKeyMetadataList>;

	using Type = BranchStructListBuilder<KeyMetadataList, BranchStructTF>;
};





}
}

#endif

