
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












template <typename StructsTF, Int Idx>
class LeafListTool {
public:
	typedef TypeList<
				StreamDescr<
                    typename StructsTF::LeafType,
                    Idx
                >
	>																			LeafStructList;

    typedef TypeList<IntValue<1>>                                               SubstreamSizeList;
};


template <typename StructsTF, typename... Tail, Int Idx>
class LeafListTool<TypeList<StructsTF, Tail...>, Idx> {

	typedef LeafListTool<StructsTF, Idx> 										HeadListTool;

	typedef typename HeadListTool::LeafStructsList								HeadStructList;
	typedef typename HeadListTool::LeafSubstreamList							HeadSubstreamList;

public:
    typedef typename MergeLists<
                HeadStructList,
                typename LeafListTool<
                    TypeList<Tail...>,
                    Idx + ListSize<HeadStructList>::Value
                >::LeafStructList
    >::Result                                                                   LeafStructList;

    typedef typename MergeLists<
                    HeadSubstreamList,
                    typename LeafListTool<
                        TypeList<Tail...>,
                        Idx + ListSize<HeadSubstreamList>::Value
                    >::SubstreamSizeList
    >::Result                                                                   SubstreamSizeList;
};


template <Int Idx>
class LeafListTool<TypeList<>, Idx> {
public:
	typedef TypeList<>                                                          LeafStructList;
	typedef TypeList<>                                                          SubstreamSizeList;
};


}


template <typename List, Int Idx = 0>
class PackedStructListBuilder;


template <
    typename StructsTF,
    typename... Tail,
    Int Idx
>
class PackedStructListBuilder<TypeList<StructsTF, Tail...>, Idx> {

    typedef TypeList<StructsTF, Tail...> List;

public:
    typedef typename MergeLists<
            StreamDescr<
                typename StructsTF::NonLeafType,
                Idx
            >,
            typename PackedStructListBuilder<
                TypeList<Tail...>,
                Idx + 1
            >::NonLeafStructList
    >::Result                                                                   NonLeafStructList;

    typedef typename MergeLists<
                StreamDescr<
                    typename StructsTF::LeafType,
                    Idx
                >,
                typename PackedStructListBuilder<
                    TypeList<Tail...>,
                    Idx + 1
                >::LeafStructList
    >::Result                                                                   LeafStructList;

    typedef typename MergeLists<
                    IntValue<1>,
                    typename PackedStructListBuilder<
                        TypeList<Tail...>,
                        Idx + 1
                    >::SubstreamSizeList
    >::Result                                                                   SubstreamSizeList;
};






template <Int Idx>
class PackedStructListBuilder<TypeList<>, Idx> {
public:
    typedef TypeList<>                                                          NonLeafStructList;
    typedef TypeList<>                                                          LeafStructList;
    typedef TypeList<>                                                          SubstreamSizeList;
};


}
}



#endif /* MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_ */
