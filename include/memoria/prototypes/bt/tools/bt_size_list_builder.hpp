
// Copyright Victor Smirnov 2014.
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

template <typename SizeList>
struct StreamStartTag {
    using Type = SizeList;
};


template <typename T>
struct StructSizeProvider {
    static const Int Value = T::Indexes;
};


namespace detail {

template <
    typename OffsetList,
    typename List,
    Int Idx                 = 0,
    Int Max                 = ListSize<List>::Value
>
class TagStreamsStart {
    static const Int StreamOffset = list_tree::LeafCount<List, IntList<Idx>, 2>::Value;

    using StreamStart = typename Select<StreamOffset, OffsetList>::Result;

    using FixedList = Replace<OffsetList, StreamStartTag<StreamStart>, StreamOffset>;

public:
    using Type = typename TagStreamsStart<FixedList, List, Idx + 1>::Type;
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

template <
    Int Offset
>
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
				Idx + 1
			>::Type
	>;
};


template <typename List, Int Offset, Int Max>
class ForAllTopLevelElements<List, Offset, Max, Max> {
public:
	using Type = IntList<>;
};


}


template <typename List>
class LeafOffsetListBuilder {
    using LinearLeafList = Linearize<List, 2>;
    using OffsetList = typename detail::OffsetBuilder<LinearLeafList>::Type;
public:
    using Type = typename detail::TagStreamsStart<OffsetList, List>::Type;
};




template <typename List, typename Path>
using LeafSubsetInf = typename detail::ForAllTopLevelElements<
		typename memoria::list_tree::Subtree<List, Path>::Type,
		memoria::list_tree::LeafCount<List, Path>::Value
>::Type;


template <typename List>
using StreamsStartSubset = LeafSubsetInf<List, IntList<>>;



}
}



#endif
