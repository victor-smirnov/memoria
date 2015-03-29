
#ifndef MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_
#define MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/tools/bt_accumulators.hpp>

#include <memoria/core/types/list/linearize.hpp>

namespace memoria   {
namespace bt        {


namespace detail  {

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

}





template <typename List>
class PackedLeafStructListBuilder;

template <typename List>
class PackedBranchStructListBuilder;

template <typename List>
class IteratorAccumulatorListBuilder;

template <
    typename StructsTF,
    typename... Tail
>
class PackedLeafStructListBuilder<TypeList<StructsTF, Tail...>> {

    using BranchType 	= typename StructsTF::NonLeafType;
    using LeafType 		= typename StructsTF::LeafType;

    static_assert(
            detail::ValidateSubstreams<BranchType, LeafType>::Value,
            "Invalid substream structure"
    );

public:
    using StructList = AppendItemToList<
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

    using BranchType = typename StructsTF::NonLeafType;

public:
    using StructList = AppendItemToList<
                BranchType,
                typename PackedBranchStructListBuilder<
                    TypeList<Tail...>
                >::StructList
    >;
};


template <
    typename StructsTF,
    typename... Tail
>
class IteratorAccumulatorListBuilder<TypeList<StructsTF, Tail...>> {

	using LeafStructList 	= typename StructsTF::LeafType;
    using BranchStructList 	= typename StructsTF::NonLeafType;

    using IdxRangeList 		= typename StructsTF::IdxRangeList;

    using RangeListType = typename BranchNodeRangeListBuilder<
    		BranchStructList,
    		LeafStructList,
    		IdxRangeList
    >::Type;

    using AccType = typename IteratorAccumulatorBuilder<
    		BranchStructList,
    		RangeListType
    >::Type;

public:
    using StructList = AppendItemToList<
    			AccType,
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

template <>
class IteratorAccumulatorListBuilder<TypeList<>> {
public:
    using StructList = TypeList<>;
};


}
}



#endif /* MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_ */
