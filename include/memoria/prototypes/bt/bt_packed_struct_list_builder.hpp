
#ifndef MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_
#define MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

namespace memoria {
namespace bt {

namespace internal {

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



template <typename List, typename Path, Int Idx = 0, Int Max = ListHead<Path>::Value> struct LeafOffsetCount;
//template <typename List, typename Path, Int Idx, Int Max> struct LeafOffsetCount;

template <Int, Int> struct IncompleteTreePath;
template <Int, Int> struct InvalidTreePath;
template <Int> 		struct InvalidTreeStructure;


// Main case. Current tree element is Leaf and there are more elements in the level to consume.
// Proceed further.
template <
	Int Substreams,
	typename... Tail,
	Int... Path,
	Int Idx,
	Int Max
>
struct LeafOffsetCount<TypeList<IntValue<Substreams>, Tail...>, IntList<Max, Path...>, Idx, Max>
{
	static const Int Size = Substreams + LeafOffsetCount<TypeList<Tail...>, IntList<Max, Path...>, Idx + 1, Max>::Size;
};



// Main case. Current element is Leaf and we are at the target element. Stop.
template <
	Int Substreams,
	typename... Tail,
	Int Max
>
struct LeafOffsetCount<TypeList<IntValue<Substreams>, Tail...>, IntList<Max>, Max, Max>
{
	static const Int Size = 0;
};


// Main case. Current element is NonLeaf and there are more tree elements at this tree level to consume.
// Determine subtree size and proceed further at the current level.
template <
	typename... List,
	typename... Tail,
	Int... Path,
	Int Idx,
	Int Max
>
struct LeafOffsetCount<TypeList<TypeList<List...>, Tail...>, IntList<Max, Path...>, Idx, Max>
{
	static const Int Size = SubstreamsTreeSize<TypeList<List...>>::Size
							+ LeafOffsetCount<TypeList<Tail...>, IntList<Max, Path...>, Idx + 1, Max>::Size;
};

// Main case. Current element is NonLeaf and we are at the target element.
// Dive into subtree.
template <
	typename... Sublist,
	typename... Tail,
	Int PathHead,
	Int... PathTail,
	Int Max
>
struct LeafOffsetCount<TypeList<TypeList<Sublist...>, Tail...>, IntList<Max, PathHead, PathTail...>, Max, Max> {
	static const Int Size = LeafOffsetCount<TypeList<Sublist...>, IntList<PathHead, PathTail...>, 0, PathHead>::Size;
};





// Exceptional case. Tree path is too long. It exceeds some Leaf node.
template <
	Int... Path,
	Int Idx,
	Int Max
>
struct LeafOffsetCount<TypeList<>, IntList<Path...>, Idx, Max> {
	static const Int Size = InvalidTreePath<Idx, Max>::Value;
};


// Exceptional case. Tree path is too short, it doesn't end at a Leaf node.
template <
	typename... List,
	typename... Tail,
	Int Max
>
struct LeafOffsetCount<TypeList<TypeList<List...>, Tail...>, IntList<Max>, Max, Max>
{
	static const Int Size = IncompleteTreePath<Max, Max>::Value;
};






template <typename LeafType, Int Idx = 0>
struct LinearLeafListHelper {
	typedef TypeList<
				StreamDescr<LeafType, Idx>
	>																			Type;
};


template <typename LeafType, typename... Tail, Int Idx>
struct LinearLeafListHelper<TypeList<LeafType, Tail...>, Idx> {
	typedef typename MergeLists<
				StreamDescr<LeafType, Idx>,
				LinearLeafListHelper<Tail..., Idx + 1>
	>::Result 																	Type;
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
	>::Result 																	Type;
};

template <Int Idx>
struct LinearLeafListHelper<TypeList<>, Idx> {
	typedef TypeList<>															Type;
};





template <typename T, Int Acc = 0>
struct SubstreamSizeListBuilder {
	typedef TypeList<IntValue<1>>												Type;
};


template <typename T, typename... List, Int Acc>
struct SubstreamSizeListBuilder<TypeList<T, List...>, Acc> {
	typedef typename SubstreamSizeListBuilder<
				TypeList<List...>, Acc + 1
	>::Type																		Type;
};

template <typename T, typename... List, Int Acc, typename R, typename... Tail>
struct SubstreamSizeListBuilder<TypeList<T, TypeList<List...>, R, Tail...>, Acc> {
	typedef typename MergeLists<
			IntValue<Acc + 1>,
			TypeList<typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type>,
			typename SubstreamSizeListBuilder<TypeList<R, Tail...>, 0>::Type
	>::Result																	Type;
};

template <typename T, typename... List, Int Acc>
struct SubstreamSizeListBuilder<TypeList<T, TypeList<List...>>, Acc> {
	typedef typename MergeLists<
			IntValue<Acc + 1>,
			TypeList<typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type>
	>::Result																	Type;
};

template <typename... List, typename R, typename... Tail, Int Acc>
struct SubstreamSizeListBuilder<TypeList<TypeList<List...>, R, Tail...>, Acc> {
	typedef typename MergeLists<
			TypeList<typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type>,
			typename SubstreamSizeListBuilder<TypeList<R, Tail...>, 0>::Type
	>::Result																	Type;
};

template <typename... List, Int Acc>
struct SubstreamSizeListBuilder<TypeList<TypeList<List...>>, Acc> {
	typedef TypeList<
			typename SubstreamSizeListBuilder<TypeList<List...>, 0>::Type
	>																			Type;
};

template <typename T, Int Acc>
struct SubstreamSizeListBuilder<TypeList<T>, Acc> {
	typedef TypeList<IntValue<Acc + 1>>											Type;
};


}







template <typename List, Int Idx = 0>
class PackedLeafStructListBuilder;

template <typename List, Int Idx = 0>
class PackedNonLeafStructListBuilder;



template <
    typename StructsTF,
    typename... Tail,
    Int Idx
>
class PackedLeafStructListBuilder<TypeList<StructsTF, Tail...>, Idx> {

    typedef TypeList<StructsTF, Tail...> List;

    typedef typename internal::LinearLeafListHelper<
    		typename StructsTF::LeafType,
    		Idx
    >::Type																		LeafList;

public:

    typedef typename MergeLists<
                LeafList,
                typename PackedLeafStructListBuilder<
                    TypeList<Tail...>,
                    Idx + ListSize<LeafList>::Value
                >::StructList
    >::Result                                                                   StructList;

    typedef typename internal::SubstreamSizeListBuilder<
                typename StructsTF::LeafType
    >::Type                                                                   	SubstreamSizeList;
};


template <
    typename StructsTF,
    typename... Tail,
    Int Idx
>
class PackedNonLeafStructListBuilder<TypeList<StructsTF, Tail...>, Idx> {
public:
	typedef typename MergeLists<
            StreamDescr<
                typename StructsTF::NonLeafType,
                Idx
            >,
            typename PackedNonLeafStructListBuilder<
                TypeList<Tail...>,
                Idx + 1
            >::StructList
    >::Result                                                                   StructList;
};





template <Int Idx>
class PackedLeafStructListBuilder<TypeList<>, Idx> {
public:
    typedef TypeList<>                                                          StructList;
    typedef TypeList<>                                                          SubstreamSizeList;
};


template <Int Idx>
class PackedNonLeafStructListBuilder<TypeList<>, Idx> {
public:
    typedef TypeList<>                                                          StructList;
};




}
}



#endif /* MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_ */
