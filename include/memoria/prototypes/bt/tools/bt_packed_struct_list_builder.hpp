
#ifndef MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_
#define MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder_internal.hpp>


namespace memoria 	{
namespace bt 		{


template <typename List>
class PackedLeafStructListBuilder;

template <typename List>
class PackedBranchStructListBuilder;


template <
    typename StructsTF,
    typename... Tail
>
class PackedLeafStructListBuilder<TypeList<StructsTF, Tail...>> {

	using BranchType = typename internal::NormalizeSingleElementList<typename StructsTF::NonLeafType>::Type;
	using LeafType = typename internal::NormalizeSingleElementList<typename StructsTF::LeafType>::Type;

	static_assert(
			internal::ValidateSubstreams<BranchType, LeafType>::Value,
			"Invalid substream structure"
	);

public:
    using StructList = MergeLists<
    			LeafType,
                typename PackedLeafStructListBuilder<
                    TypeList<Tail...>
                >::StructList
    >;
};


template <
    typename StructsTF,
    typename... Tail
>
class PackedBranchStructListBuilder<TypeList<StructsTF, Tail...>> {

	using BranchType = typename internal::NormalizeSingleElementList<typename StructsTF::NonLeafType>::Type;

public:
    using StructList = MergeLists<
                BranchType,
                typename PackedBranchStructListBuilder<
                    TypeList<Tail...>
                >::StructList
    >;
};

template <>
class PackedLeafStructListBuilder<TypeList<>> {
public:
    using StructList = TypeList<>;
};


template <>
class PackedBranchStructListBuilder<TypeList<>> {
public:
    using StructList = TypeList<>;
};



}
}



#endif /* MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_ */
