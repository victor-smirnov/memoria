
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SRTREAMDSCR_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SRTREAMDSCR_FACTORY_HPP

#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/list/linearize.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/core/types/algo/fold.hpp>
#include <memoria/core/exceptions/bounds.hpp>



#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_tree.hpp>
#include <memoria/core/packed/tree/packed_fse_quick_tree.hpp>

#include <iostream>
#include <tuple>
#include <utility>

namespace memoria   {
namespace bt        {


namespace details {

	template <typename State, typename Element>
	struct PackedStructsIndexesSum
	{
		static constexpr Int Value = StructSizeProvider<Element>::Value + State::Value;
	};

	template <typename Element>
	struct PackedStructsIndexesSum<EmptyType, Element>
	{
		static constexpr Int Value = StructSizeProvider<Element>::Value;
	};

	template <>
	struct PackedStructsIndexesSum<EmptyType, EmptyType>
	{
		static constexpr Int Value = 0;
	};



	template <typename LeafStruct, template <Int> class BranchStructTF>
	struct BTBuildBranchStruct
	{
		using Type = typename BranchStructTF<
				StructSizeProvider<LeafStruct>::Value
		>::Type;
	};

	template <typename... LeafStructs, template <Int> class BranchStructTF>
	struct BTBuildBranchStruct<TL<LeafStructs...>, BranchStructTF>
	{
		using Type = typename BranchStructTF<
				FoldTLRight<TL<LeafStructs...>, PackedStructsIndexesSum>::Type::Value
		>::Type;
	};



	template <typename LeafStruct, template <Int> class BranchStructTF>
	struct BTBuildBranchStructSS
	{
		using Type = typename BranchStructTF<
				StructSizeProvider<LeafStruct>::Value + 1
		>::Type;
	};

	template <typename... LeafStructs, template <Int> class BranchStructTF>
	struct BTBuildBranchStructSS<TL<LeafStructs...>, BranchStructTF>
	{
		using Type = typename BranchStructTF<
				FoldTLRight<TL<LeafStructs...>, PackedStructsIndexesSum>::Type::Value + 1
		>::Type;
	};
}


template <Int LeafIndexes>
struct FSEBranchStructTF {
	using Type = PkdFQTree<BigInt, LeafIndexes>;
};

template <Int LeafIndexes>
struct VLEBranchStructTF {
	using Type = PkdVTree<Packed2TreeTypes<BigInt, BigInt, LeafIndexes, UByteExintCodec>>;
};


template <typename LeafStructList, template <Int> class BranchStructTF, Int Idx = 0> struct BTStreamDescritorsBuilder;

template <typename LeafStruct, typename... Tail, template <Int> class BranchStructTF, Int Idx>
struct BTStreamDescritorsBuilder<TL<LeafStruct, Tail...>, BranchStructTF, Idx>
{
	using Type = typename details::BTBuildBranchStruct<LeafStruct, BranchStructTF>::Type;
};

template <typename LeafStruct, typename... Tail, template <Int> class BranchStructTF>
struct BTStreamDescritorsBuilder<TL<LeafStruct, Tail...>, BranchStructTF, 0>
{
	using Type = typename details::BTBuildBranchStructSS<LeafStruct, BranchStructTF>::Type;
};

template <template <Int> class BranchStructTF, Int Idx>
struct BTStreamDescritorsBuilder<TL<>, BranchStructTF, Idx>
{
	using Type = TL<>;
};




}
}

#endif

