
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTSS_TEST_FACTORY_HPP_
#define MEMORIA_TESTS_BTSS_TEST_FACTORY_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/prototypes/bt_tl/tools/bttl_tools_random_gen.hpp>




#include <functional>

namespace memoria {


template <PackedSizeType LeafSizeType, PackedSizeType BranchSizeType>
class BTSSTestCtr {};

template <
	PackedSizeType LeafSizeType,
	PackedSizeType BranchSizeType,
	typename CtrSizeT,
	Int Indexes = 1
> struct BTSSTestStreamTF;

template <typename CtrSizeT, Int Indexes>
struct BTSSTestStreamTF<PackedSizeType::FIXED, PackedSizeType::FIXED, CtrSizeT, Indexes> {
	using Type = StreamTF<
			TL<TL<
				PkdFTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, Indexes>>
			>>,
			TL<TL<TL<IndexRange<0, Indexes>>>>,
			FSEBranchStructTF
	>;
};


template <typename CtrSizeT, Int Indexes>
struct BTSSTestStreamTF<PackedSizeType::VARIABLE, PackedSizeType::FIXED, CtrSizeT, Indexes> {
	using Type = StreamTF<
			TL<TL<
				PkdVTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, Indexes, UByteI7Codec>>
			>>,
			TL<TL<TL<IndexRange<0, Indexes>>>>,
			FSEBranchStructTF
	>;
};


template <typename CtrSizeT, Int Indexes>
struct BTSSTestStreamTF<PackedSizeType::FIXED, PackedSizeType::VARIABLE, CtrSizeT, Indexes> {
	using Type = StreamTF<
			TL<TL<
				PkdFTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, Indexes>>
			>>,
			TL<TL<TL<IndexRange<0, Indexes>>>>,
			VLEBranchStructTF
	>;
};


template <typename CtrSizeT, Int Indexes>
struct BTSSTestStreamTF<PackedSizeType::VARIABLE, PackedSizeType::VARIABLE, CtrSizeT, Indexes> {
	using Type = StreamTF<
			TL<TL<
				PkdVTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, Indexes, UByteI7Codec>>
			>>,
			TL<TL<TL<IndexRange<0, Indexes>>>>,
			VLEBranchStructTF
	>;
};


template <
    typename Profile,
    PackedSizeType LeafSizeType,
	PackedSizeType BranchSizeType
>
struct BTSSTestTypesBase: public BTTypes<Profile, BTSingleStream> {

    using Base = BTTypes<Profile, BTSingleStream>;

    using ValueType = Byte;

    using Key 	= BigInt;
    using Value	= Byte;

    using CtrSizeT = BigInt;

    using StreamDescriptors = TL<
    		typename BTSSTestStreamTF<LeafSizeType, BranchSizeType, CtrSizeT, 1>::Type
	>;

    using Metadata = BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >;


    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::table::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::table::ItrMiscName
    >;
};







template <
    typename Profile,
    PackedSizeType LeafSizeType,
	PackedSizeType BranchSizeType
>
struct BTTypes<Profile, BTSSTestCtr<LeafSizeType, BranchSizeType>>: public BTSSTestTypesBase<Profile, LeafSizeType, BranchSizeType>
{
};


template <typename Profile, PackedSizeType LeafSizeType, PackedSizeType BranchSizeType, typename T>
class CtrTF<Profile, BTSSTestCtr<LeafSizeType, BranchSizeType>, T>: public CtrTF<Profile, memoria::BTSingleStream, T> {
    using Base = CtrTF<Profile, memoria::BTSingleStream, T>;
public:
};


template <PackedSizeType LeafSizeType, PackedSizeType BranchSizeType>
struct TypeHash<BTSSTestCtr<LeafSizeType, BranchSizeType>>:   UIntValue<
    HashHelper<3011, (Int)LeafSizeType, (Int)BranchSizeType>::Value
> {};


}


#endif