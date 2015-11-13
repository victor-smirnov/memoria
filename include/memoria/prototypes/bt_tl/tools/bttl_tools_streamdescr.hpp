
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_TOOLS_STREAMDESCR_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_TOOLS_STREAMDESCR_HPP

#include <memoria/core/types/list/append.hpp>

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/container/container.hpp>

namespace memoria {
namespace bttl    {

namespace details {
	template <typename StreamDescriptorsList> struct GetLeafList;

	template <
		typename... Tail,
		typename LeafType,
		typename IndexRangeList,
		template <Int LeafIndexes> class BranchStructTF
	>
	struct GetLeafList<TL<memoria::bt::StreamTF<LeafType, IndexRangeList, BranchStructTF>, Tail...>> {
		using Type = MergeLists<
				TL<LeafType>,
				typename GetLeafList<TL<Tail...>>::Type
		>;
	};

	template <>
	struct GetLeafList<TL<>> {
		using Type = TL<>;
	};




	template <typename StreamDescriptorsList, typename CtrSizeT = BigInt>
	struct InferSizeStruct {
		using LeafStreamsStructList = typename details::GetLeafList<StreamDescriptorsList>::Type;
		static const PackedSizeType LeafSizeType = PackedListStructSizeType<Linearize<LeafStreamsStructList>>::Value;

		using Type = typename IfThenElse<
				LeafSizeType == PackedSizeType::FIXED,
				PkdFQTree<CtrSizeT, 1>,
				PkdVTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, 1, UByteI7Codec>>
		>::Result;
	};


	template <typename List, typename SizeStruct> struct AppendSizeStruct;

	template <typename SizeStruct>
	struct AppendSizeStruct<TypeList<>, SizeStruct> {
		using Type = TL<TL<SizeStruct>>;
	};

	template <typename... List, typename SizeStruct>
	struct AppendSizeStruct<TypeList<List...>, SizeStruct> {
		using Type = TL<TL<List..., SizeStruct>>;
	};



	template <typename List, typename SizeIndexes> struct AppendSizeIndexes;

	template <typename SizeIndexes>
	struct AppendSizeIndexes<TypeList<>, SizeIndexes> {
		using Type = TL<TL<SizeIndexes>>;
	};

	template <typename... List, typename SizeIndexes>
	struct AppendSizeIndexes<TypeList<List...>, SizeIndexes> {
		using Type = TL<TL<List..., SizeIndexes>>;
	};
}




template <
	typename StreamDescriptorsList,
	typename SizeStruct 	= typename details::InferSizeStruct<StreamDescriptorsList>::Type,
	typename SizeIndexes	= TL<IndexRange<0, 1>>
> class BTTLAugmentStreamDescriptors;

template <
	typename... Tail,
	typename LeafType,
	typename IndexRangeList,
	template <Int LeafIndexes> class BranchStructTF,
	typename SizeStruct,
	typename SizeIndexes
>
class BTTLAugmentStreamDescriptors<TL<memoria::bt::StreamTF<LeafType, IndexRangeList, BranchStructTF>, Tail...>, SizeStruct, SizeIndexes> {
	using NewLeafType 		= typename details::AppendSizeStruct<LeafType, SizeStruct>::Type;
	using NewIndexRangeList = typename details::AppendSizeIndexes<IndexRangeList, SizeIndexes>::Type;
public:
	using Type = MergeLists<
		memoria::bt::StreamTF<NewLeafType, NewIndexRangeList, BranchStructTF>,
		typename BTTLAugmentStreamDescriptors<TL<Tail...>, SizeStruct, SizeIndexes>::Type
	>;
};



template <
	typename LeafType,
	typename IndexRangeList,
	template <Int LeafIndexes> class BranchStructTF,
	typename SizeStruct,
	typename SizeIndexes
>
class BTTLAugmentStreamDescriptors<TL<memoria::bt::StreamTF<LeafType, IndexRangeList, BranchStructTF>>, SizeStruct, SizeIndexes> {
public:
	using Type = memoria::bt::StreamTF<LeafType, IndexRangeList, BranchStructTF>;
};


}
}

#endif
