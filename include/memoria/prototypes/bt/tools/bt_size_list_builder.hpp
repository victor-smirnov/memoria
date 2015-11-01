
// Copyright Victor Smirnov 2014+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BT_SIZE_LIST_BUILDER_HPP_
#define MEMORIA_BT_SIZE_LIST_BUILDER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/core/types/algo/select.hpp>

namespace memoria   {
namespace bt        {

template <typename Tree>
using FlattenBranchTree = Linearize<Tree, 1>;

template <typename Tree>
using FlattenLeafTree = Linearize<Tree, 2>;

template <typename SizeList>
struct StreamStartTag {
    using Type = SizeList;
};


template <typename LeafTypeT, typename IndexRangeListT>
struct StreamTF {};


namespace detail {

	template <typename List> struct FixStreamStart;


	template <Int Head, Int... Tail>
	struct FixStreamStart<IntList<Head, Tail...>> {
		using Type = MergeValueListsT<
				IntList<Head + 1>,
				typename FixStreamStart<IntList<Tail...>>::Type
		>;
	};


	template <>
	struct FixStreamStart<IntList<>> {
		using Type = IntList<>;
	};




	template <
		typename OffsetList,
		typename List,
		Int Idx                 = 0,
		Int Max                 = ListSize<List>::Value
	>
	class TagStreamsStart {
		static const Int StreamOffset = list_tree::LeafCount<List, IntList<Idx>, 2>::Value;

		using StreamStart = typename Select<StreamOffset, OffsetList>::Result;

		using FixedList = Replace<
				OffsetList,
				StreamStartTag<typename FixStreamStart<StreamStart>::Type>,
				StreamOffset
				>;

	public:
		using Type = typename TagStreamsStart<FixedList, List, Idx + 1, Max>::Type;
	};

	template <typename OffsetList, typename List, Int Idx>
	class TagStreamsStart<OffsetList, List, Idx, Idx> {
	public:
		using Type = OffsetList;
	};


	template <typename List> struct OffsetBuilder;
	template <typename List, Int Offset> struct InternalOffsetBuilder;

	template <
		typename Head,
		typename... Tail
	>
	struct OffsetBuilder<TypeList<Head, Tail...>> {
		using Type = MergeLists<
				IntList<0>,
				typename OffsetBuilder<TypeList<Tail...>>::Type
				>;
	};

	template <
		typename... Head,
		typename... Tail
	>
	struct OffsetBuilder<TypeList<TypeList<Head...>, Tail...>> {
		using Type = MergeLists<
				typename InternalOffsetBuilder<TypeList<Head...>, 0>::Type,
				typename OffsetBuilder<TypeList<Tail...>>::Type
				>;
	};


	template <>
	struct OffsetBuilder<TypeList<>> {
		using Type = TypeList<>;
	};


	template <
		typename Head,
		typename... Tail,
		Int Offset
	>
	struct InternalOffsetBuilder<TypeList<Head, Tail...>, Offset>
	{
		using Type = MergeValueListsT<
				IntList<Offset>,
				typename InternalOffsetBuilder<
				TypeList<Tail...>,
				Offset + StructSizeProvider<Head>::Value
			>::Type
		>;
	};

	template <Int Offset>
	struct InternalOffsetBuilder<TypeList<>, Offset>
	{
		using Type = IntList<>;
	};


	template <typename List, Int Offset = 0, Int Idx = 0, Int Max = ListSize<List>::Value>
	class ForAllTopLevelElements;

	template <typename List, Int Offset, Int Idx, Int Max>
	class ForAllTopLevelElements {
		static const Int LeafOffset = memoria::list_tree::LeafCountInf<List, IntList<Idx>>::Value;
	public:
		using Type = AppendItemToList<
				IntValue<LeafOffset + Offset>,
				typename ForAllTopLevelElements<
					List,
					Offset,
					Idx + 1,
					Max
				>::Type
			>;
	};


	template <typename List, Int Offset, Int Max>
	class ForAllTopLevelElements<List, Offset, Max, Max> {
	public:
		using Type = IntList<>;
	};


}


template <typename LeafTree>
class LeafOffsetListBuilder {
    using LinearLeafList = FlattenLeafTree<LeafTree>;
    using OffsetList = typename detail::OffsetBuilder<LinearLeafList>::Type;
public:
    using Type = typename detail::TagStreamsStart<OffsetList, LeafTree>::Type;
};




template <typename List, typename Path>
using LeafSubsetInf = typename detail::ForAllTopLevelElements<
		typename memoria::list_tree::Subtree<List, Path>::Type,
		memoria::list_tree::LeafCount<List, Path>::Value
>::Type;


template <typename List>
using StreamsStartSubset = LeafSubsetInf<List, IntList<>>;



template <typename List, Int Idx, Int Pos = 0> struct FindTopLevelIdx;

template <
	typename Head,
	typename... Tail,
	Int Idx,
	Int Pos
>
struct FindTopLevelIdx<TypeList<Head, Tail...>, Idx, Pos>
{
	static const Int Children = memoria::list_tree::SubtreeLeafCount<TypeList<Head>, IntList<>>::Value;

	static const Int Value = Idx < Children ?
			Pos :
			FindTopLevelIdx<
				TypeList<Tail...>,
				Idx - Children,
				Pos + 1
			>::Value;
};

template <Int Idx, Int Pos>
struct FindTopLevelIdx<TypeList<>, Idx, Pos> {
	static const Int Value = -1;
};





template <typename List, Int Idx, Int Pos = 0> struct FindLocalLeafOffsetV;
template <typename List, Int Idx, Int Pos = 0> struct FindLocalLeafOffsetT;

namespace detail {

	template <typename List, Int Idx, Int Pos, bool Condition> struct FindLocalLeafOffsetHelperV;
	template <typename List, Int Idx, Int Pos, bool Condition> struct FindLocalLeafOffsetHelperT;

	template <typename List>
	struct ListSizeHelper {
		static const Int Value = ListSize<List>::Value;
	};

	template <typename List>
	struct ListSizeHelper<StreamStartTag<List>> {
		static const Int Value = ListSize<List>::Value;
	};



	template <
		typename Head,
		typename... Tail,
		Int Idx, Int Pos
	>
	struct FindLocalLeafOffsetHelperV<TypeList<Head, Tail...>, Idx, Pos, true> {
		static const Int Value = Idx - Pos;
	};

	template <
		typename Head,
		typename... Tail,
		Int Idx, Int Pos
	>
	struct FindLocalLeafOffsetHelperT<TypeList<Head, Tail...>, Idx, Pos, true> {
		using Type = Head;
	};


	template <
		typename Head,
		typename... Tail,
		Int Idx, Int Pos
	>
	struct FindLocalLeafOffsetHelperV<TypeList<Head, Tail...>, Idx, Pos, false> {
		static const Int Value = FindLocalLeafOffsetV<TypeList<Tail...>, Idx, Pos + ListSizeHelper<Head>::Value>::Value;
	};


	template <
		typename Head,
		typename... Tail,
		Int Idx, Int Pos
	>
	struct FindLocalLeafOffsetHelperT<TypeList<Head, Tail...>, Idx, Pos, false> {
		using Type = typename FindLocalLeafOffsetT<TypeList<Tail...>, Idx, Pos + ListSizeHelper<Head>::Value>::Type;
	};
}


template <
	typename Head,
	typename... Tail,
	Int Idx,
	Int Pos
>
struct FindLocalLeafOffsetV<TypeList<Head, Tail...>, Idx, Pos> {
	static const Int Value = FindLocalLeafOffsetV<TypeList<Tail...>, Idx, Pos + 1>::Value;
};


template <
	typename Head,
	typename... Tail,
	Int Idx,
	Int Pos
>
struct FindLocalLeafOffsetT<TypeList<Head, Tail...>, Idx, Pos> {
	using Type = typename FindLocalLeafOffsetT<TypeList<Tail...>, Idx, Pos + 1>::Type;
};




template <
	typename Head,
	typename... Tail,
	Int Idx
>
struct FindLocalLeafOffsetV<TypeList<Head, Tail...>, Idx, Idx> {
	static const Int Value = 0;
};


template <
	typename Head,
	typename... Tail,
	Int Idx
>
struct FindLocalLeafOffsetT<TypeList<Head, Tail...>, Idx, Idx> {
	using Type = Head;
};




template <
	typename... List,
	typename... Tail,
	Int Idx,
	Int Pos
>
struct FindLocalLeafOffsetV<TypeList<TypeList<List...>, Tail...>, Idx, Pos> {
private:
	using LocalList = TypeList<List...>;
	static const Int LocalSize = ListSize<LocalList>::Value;
public:

	static const Int Value = detail::FindLocalLeafOffsetHelperV<
		TypeList<TypeList<List...>, Tail...>,
		Idx,
		Pos,
		Idx < Pos + LocalSize
		>::Value;
};



template <
	typename... Tail,
	Int... List,
	Int Idx,
	Int Pos
>
struct FindLocalLeafOffsetT<TypeList<IntList<List...>, Tail...>, Idx, Pos> {
private:
	static const Int LocalSize = ListSize<IntList<List...>>::Value;
public:
	using Type = typename detail::FindLocalLeafOffsetHelperT<
			TypeList<IntList<List...>, Tail...>,
			Idx,
			Pos,
			Idx < Pos + LocalSize
	>::Type;
};



template <
	typename... Tail,
	Int... List,
	Int Idx,
	Int Pos
>
struct FindLocalLeafOffsetT<TypeList<StreamStartTag<IntList<List...>>, Tail...>, Idx, Pos> {
private:
	static const Int LocalSize = ListSize<IntList<List...>>::Value;
public:
	using Type = typename detail::FindLocalLeafOffsetHelperT<
			TypeList<StreamStartTag<IntList<List...>>, Tail...>,
			Idx,
			Pos,
			Idx < Pos + LocalSize
	>::Type;
};



template <
	typename... Tail,
	typename... List,
	Int Idx
>
struct FindLocalLeafOffsetV<TypeList<TypeList<List...>, Tail...>, Idx, Idx> {
	static const Int Value = 0;
};

template <
	typename... Tail,
	Int... List,
	Int Idx
>
struct FindLocalLeafOffsetT<TypeList<IntList<List...>, Tail...>, Idx, Idx> {
	using Type = IntList<List...>;
};

template <
	typename... Tail,
	Int... List,
	Int Idx
>
struct FindLocalLeafOffsetT<TypeList<StreamStartTag<IntList<List...>>, Tail...>, Idx, Idx> {
	using Type = StreamStartTag<IntList<List...>>;
};


template <
	Int Idx,
	Int Pos
>
struct FindLocalLeafOffsetV<TypeList<>, Idx, Pos>;

template <
	Int Idx,
	Int Pos
>
struct FindLocalLeafOffsetT<TypeList<>, Idx, Pos>;






template <typename List, Int Idx> struct GetLeafPrefix;


template <typename List, Int Idx>
struct GetLeafPrefix {
	static const Int Value = Select<Idx, List>::Value;
};

template <typename List, Int Idx>
struct GetLeafPrefix<StreamStartTag<List>, Idx> {
	static const Int Value = Select<Idx, List>::Value;
};


template <typename Path, Int Depth = 0> struct IsStreamStart;

template <Int Head, Int... Tail>
struct IsStreamStart<IntList<Head, Tail...>, 0> {
	static const bool Value = IsStreamStart<IntList<Tail...>, 1>::Value;
};

template <Int Head, Int... Tail, Int Idx>
struct IsStreamStart<IntList<Head, Tail...>, Idx> {
	static const bool Value = Head == 0 && IsStreamStart<IntList<Tail...>, Idx + 1>::Value;
};

template <Int Idx>
struct IsStreamStart<IntList<>, Idx> {
	static const bool Value = true;
};




namespace detail {
	template <typename List> struct IsStreamStartTag;

	template <Int... List>
	struct IsStreamStartTag<IntList<List...>> {
		static const bool Value = false;
	};


	template <Int... List>
	struct IsStreamStartTag<StreamStartTag<IntList<List...>>> {
		static const bool Value = true;
	};
}



template <typename LeafStructList, Int LeafIdx>
struct LeafToBranchIndexByValueTranslator {
protected:
	using LeafOffsets = typename LeafOffsetListBuilder<LeafStructList>::Type;

	using Leafs = FlattenLeafTree<LeafStructList>;

	static const Int LocalLeafOffset = FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;

	using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

	using LeafPath = typename memoria::list_tree::BuildTreePath<LeafStructList, LeafIdx>::Type;

public:
	static const Int LeafOffset = GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;

	static const Int BranchStructIdx = memoria::list_tree::LeafCount<LeafStructList, LeafPath, 2>::Value - LocalLeafOffset;

	static const bool IsStreamStart = LocalLeafOffset == 0 && detail::IsStreamStartTag<LocalLeafGroup>::Value;
};




template <typename LeafStructList, typename LeafPath, Int LeafIndex = 0>
struct LeafToBranchIndexTranslator {
protected:
	using LeafOffsets = typename LeafOffsetListBuilder<LeafStructList>::Type;

	using Leafs = FlattenLeafTree<LeafStructList>;

	static const Int LeafIdx 			= memoria::list_tree::LeafCount<LeafStructList, LeafPath>::Value;
	static const Int LocalLeafOffset	= FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;

	using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

public:
	static const Int BranchIndex = LeafIndex
									+ GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;


	static Int translate(Int leaf_index) {

		const Int LeafIdx 			= memoria::list_tree::LeafCount<LeafStructList, LeafPath>::Value;
		const Int LocalLeafOffset	= FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;

		using LocalLeafGroup = typename FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

		const Int LeafPrefix = GetLeafPrefix<LocalLeafGroup, LocalLeafOffset>::Value;

		const Int BranchIndex = leaf_index + LeafPrefix;

		return BranchIndex;
	}
};







}
}



#endif
