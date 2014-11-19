
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


template <typename LeafType, Int Idx = 0>
struct LinearLeafListHelper {
    typedef TypeList<
                StreamDescr<LeafType, Idx>
    >                                                                           Type;
};


template <typename LeafType, typename... Tail, Int Idx>
struct LinearLeafListHelper<TypeList<LeafType, Tail...>, Idx> {
    typedef typename MergeLists<
                StreamDescr<LeafType, Idx>,
                LinearLeafListHelper<Tail..., Idx + 1>
    >::Result                                                                   Type;
};


template <typename Head, typename... SubTail, typename... Tail, Int Idx>
struct LinearLeafListHelper<TypeList<TypeList<Head, SubTail...>, Tail...>, Idx> {
private:
    typedef typename LinearLeafListHelper<TypeList<Head, SubTail...>, Idx>::Type SubList;
public:
    typedef typename MergeLists<
                SubList,
                typename LinearLeafListHelper<
                            Tail...,
                            Idx + ListSize<SubList>::Value
                >::Type
    >::Result                                                                   Type;
};

template <Int Idx>
struct LinearLeafListHelper<TypeList<>, Idx> {
    typedef TypeList<>                                                          Type;
};





template <typename T, Int Acc = 0>
struct SubstreamSizeListBuilder {
    typedef TypeList<IntValue<1>>                                               Type;
};


template <typename T, typename... List, Int Acc>
struct SubstreamSizeListBuilder<TypeList<T, List...>, Acc> {
    typedef typename SubstreamSizeListBuilder<
                TypeList<List...>, Acc + 1
    >::Type                                                                     Type;
};

template <typename T, typename... List, Int Acc, typename R, typename... Tail>
struct SubstreamSizeListBuilder<TypeList<T, TypeList<List...>, R, Tail...>, Acc> {
    typedef typename MergeLists<
            IntValue<Acc + 1>,
            TypeList<typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type>,
            typename SubstreamSizeListBuilder<TypeList<R, Tail...>, 0>::Type
    >::Result                                                                   Type;
};

template <typename T, typename... List, Int Acc>
struct SubstreamSizeListBuilder<TypeList<T, TypeList<List...>>, Acc> {
    typedef typename MergeLists<
            IntValue<Acc + 1>,
            TypeList<typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type>
    >::Result                                                                   Type;
};

template <typename... List, typename R, typename... Tail, Int Acc>
struct SubstreamSizeListBuilder<TypeList<TypeList<List...>, R, Tail...>, Acc> {
    typedef typename MergeLists<
            TypeList<typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type>,
            typename SubstreamSizeListBuilder<TypeList<R, Tail...>, 0>::Type
    >::Result                                                                   Type;
};

template <typename... List, Int Acc>
struct SubstreamSizeListBuilder<TypeList<TypeList<List...>>, Acc> {
    typedef TypeList<
            typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type
    >                                                                           Type;
};

template <typename T, Int Acc>
struct SubstreamSizeListBuilder<TypeList<T>, Acc> {
    typedef TypeList<IntValue<Acc + 1>>                                         Type;
};


}
}
}



#endif /* MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_ */
