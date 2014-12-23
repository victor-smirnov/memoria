
#ifndef MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_INTERNAL_HPP_
#define MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_INTERNAL_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

namespace memoria   {
namespace bt        {
namespace internal  {

// Determine total leaf count of of Substreams Tree
template <typename List> struct SubstreamsTreeSize;

// Main case. We are at the the Leaf node. Count leaf size and proceed further.
template <
    Int Substreams,
    typename... Tail
>
struct SubstreamsTreeSize<TypeList<IntValue<Substreams>, Tail...>> {
    static const Int Size = Substreams + SubstreamsTreeSize<TypeList<Tail...>>::Size;
};

// Main case. We are at the the NonLeaf node. Determine subtree size and proceed further.
template <
    typename... List,
    typename... Tail
>
struct SubstreamsTreeSize<TypeList<TypeList<List...>, Tail...>> {
    static const Int Size = SubstreamsTreeSize<TypeList<List...>>::Size
                            + SubstreamsTreeSize<TypeList<Tail...>>::Size;
};

// Main case. We are at the the end of the level. Stop proceeding.
template <>
struct SubstreamsTreeSize<TypeList<>> {
    static const Int Size = 0;
};




template <typename T, Int Acc = 0>
struct SubstreamSizeListBuilder {
    using Type = TypeList<IntValue<1>>;
};


template <typename T, typename... List, Int Acc>
struct SubstreamSizeListBuilder<TypeList<T, List...>, Acc> {
    using Type = typename SubstreamSizeListBuilder<
                TypeList<List...>, Acc + 1
    >::Type;
};

template <typename T, typename... List, Int Acc, typename R, typename... Tail>
struct SubstreamSizeListBuilder<TypeList<T, TypeList<List...>, R, Tail...>, Acc> {
    using Type = MergeLists<
            IntValue<Acc + 1>,
            TypeList<typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type>,
            typename SubstreamSizeListBuilder<TypeList<R, Tail...>, 0>::Type
    >;
};

template <typename T, typename... List, Int Acc>
struct SubstreamSizeListBuilder<TypeList<T, TypeList<List...>>, Acc> {
    using Type = MergeLists<
            IntValue<Acc + 1>,
            TypeList<typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type>
    >;
};

template <typename... List, typename R, typename... Tail, Int Acc>
struct SubstreamSizeListBuilder<TypeList<TypeList<List...>, R, Tail...>, Acc> {
    using Type = MergeLists<
            TypeList<typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type>,
            typename SubstreamSizeListBuilder<TypeList<R, Tail...>, 0>::Type
    >;
};

template <typename... List, Int Acc>
struct SubstreamSizeListBuilder<TypeList<TypeList<List...>>, Acc> {
    using Type = TypeList<
            typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type
    >;
};

template <typename T, Int Acc>
struct SubstreamSizeListBuilder<TypeList<T>, Acc> {
	using Type = TypeList<IntValue<Acc + 1>>;
};




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



template <typename T>
struct NormalizeSingleElementList {
	using Type = T;
};

template <typename T>
struct NormalizeSingleElementList<TypeList<T>> {
	using Type = T;
};

template <typename... List>
struct NormalizeSingleElementList<TypeList<List...>> {
	using Type = TypeList<List...>;
};


template <typename T>
struct IsTypeOrPlainList {
	static const bool Value = true;
};

template <typename... T>
struct IsTypeOrPlainList<TypeList<T...>> {
	static const bool Value = IsPlainList<TypeList<T...>>::Value;
};



template <typename T> struct SublinearizeT {
	using Type = TypeList<T>;
};


template <typename Head, typename... Tail>
struct SublinearizeT<TypeList<Head, Tail...>> {
//	using Type = typename IfThenElse<
//					IsTypeOrPlainList<TypeList<Head, Tail...>>::Value,
//					TypeList<Head, Tail...>,
//					typename IfThenElse<
//						IsTypeOrPlainList<Head>::Value,
//						MergeLists<
//							Head,
//							typename SublinearizeT<TypeList<Tail...>>::Type
//						>,
//
//					>::Type
//
//	>::Result;
};


}
}
}



#endif /* MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_ */
