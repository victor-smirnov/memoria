
#ifndef MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_
#define MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/core/types/list/linearize.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_core.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_index_range.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_streamdescr_factory.hpp>


namespace memoria   {
namespace bt        {

namespace   {

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





template <typename List>
class PackedLeafStructListBuilder;

template <typename List>
class PackedBranchStructListBuilder;

template <typename List>
class IteratorAccumulatorListBuilder;



template <
    typename LeafType,
	typename IdxRangeList,
	template <typename> class BranchStructTF,
    typename... Tail
>
class PackedLeafStructListBuilder<TypeList<StreamTF<LeafType, IdxRangeList, BranchStructTF>, Tail...>> {

	using BranchType = typename BTStreamDescritorsBuilder<FlattenLeafTree<LeafType>, BranchStructTF>::Type;

    static_assert(
            true,//ValidateSubstreams<BranchType, LeafType>::Value,
            "Invalid substream structure"
    );

    using InputBufferType = typename InputBufferListBuilder<LeafType>::Type;

public:
    using StructList = AppendItemToList<
                LeafType,
                typename PackedLeafStructListBuilder<
                    TypeList<Tail...>
                >::StructList
    >;

    using StreamInputList = AppendItemToList<
    		typename MakeStreamEntryTL<Linearize<LeafType>>::Type,
    		typename PackedLeafStructListBuilder<
    			TypeList<Tail...>
    		>::StreamInputList
    >;

    using InputBufferList = AppendItemToList<
    		InputBufferType,
    		typename PackedLeafStructListBuilder<
    					TypeList<Tail...>
    		>::InputBufferList
    >;
};




template <
    typename LeafType,
	typename IdxRangeList,
	template <typename> class BranchStructTF,
    typename... Tail
>
class PackedBranchStructListBuilder<TypeList<StreamTF<LeafType, IdxRangeList, BranchStructTF>, Tail...>> {

    using BranchType = typename BTStreamDescritorsBuilder<FlattenLeafTree<LeafType>, BranchStructTF>::Type;

public:
    using StructList = AppendItemToList<
                BranchType,
                typename PackedBranchStructListBuilder<
                    TypeList<Tail...>
                >::StructList
    >;
};

template <typename... T>
class Undefined;


template <
	typename LeafType,
	typename IdxRangeList,
	template <typename> class BranchStructTF,
    typename... Tail
>
class IteratorAccumulatorListBuilder<TypeList<StreamTF<LeafType, IdxRangeList, BranchStructTF>, Tail...>> {

	using BranchType = typename BTStreamDescritorsBuilder<FlattenLeafTree<LeafType>, BranchStructTF>::Type;

	using LeafStructList 	= FlattenLeafTree<LeafType>;
    using BranchStructList 	= FlattenBranchTree<BranchType>;
    using FlatIdxRangeList 	= FlattenIndexRangeTree<IdxRangeList>;

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

    using AccType = typename IteratorAccumulatorBuilder<
    		BranchStructList,
    		RangeListType
    >::Type;

public:
    using AccumTuple = AppendItemToList<
    		AccType,
    		typename IteratorAccumulatorListBuilder<TypeList<Tail...>>::AccumTuple
    >;

    using RangeOffsetList = AppendItemToList<
    		RangeOffsetListType,
    		typename IteratorAccumulatorListBuilder<TypeList<Tail...>>::RangeOffsetList
    >;

    using IndexRangeList = AppendItemToList<
    		IdxRangeList,
    		typename IteratorAccumulatorListBuilder<TypeList<Tail...>>::IndexRangeList
    >;
};

template <>
class PackedLeafStructListBuilder<TypeList<>> {
public:
    using StructList 		= TypeList<>;
    using StreamInputList 	= TypeList<>;
    using InputBufferList 	= TypeList<>;
};


template <>
class PackedBranchStructListBuilder<TypeList<>> {
public:
    using StructList = TypeList<>;
};

template <>
class IteratorAccumulatorListBuilder<TypeList<>> {
public:
    using AccumTuple 		= TypeList<>;
    using RangeOffsetList 	= TypeList<>;
    using IndexRangeList	= TypeList<>;
};







template <typename T> struct AccumulatorBuilder;

template <typename PackedStruct, typename... Tail>
struct AccumulatorBuilder<TL<PackedStruct, Tail...>> {
	using Type = MergeLists<
					memoria::core::StaticVector<
						typename PkdSearchKeyTypeProvider<PackedStruct>::Type,
						StructSizeProvider<PackedStruct>::Value
					>,
					typename AccumulatorBuilder<TL<Tail...>>::Type
	>;
};

template <>
struct AccumulatorBuilder<TL<>> {
	using Type = TL<>;
};







}
}



#endif /* MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_ */
