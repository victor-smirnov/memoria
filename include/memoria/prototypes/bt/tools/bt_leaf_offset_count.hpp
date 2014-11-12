
#ifndef MEMORIA_BT_LEAF_OFFSET_COUNT_HPP_
#define MEMORIA_BT_LEAF_OFFSET_COUNT_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder_internal.hpp>

namespace memoria {
namespace bt {

/**
 *
 */

template <typename List, typename Path, Int Idx = 0, Int Max = ListHead<Path>::Value> struct LeafOffsetCount;

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
	static const Int Size = internal::SubstreamsTreeSize<TypeList<List...>>::Size
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








}
}



#endif /* MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_ */
