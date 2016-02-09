
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_TEST_FACTORY1_HPP_
#define MEMORIA_TESTS_BTTL_TEST_FACTORY1_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/prototypes/bt_tl/tools/bttl_tools_random_gen.hpp>
#include "bttl_test_tools.hpp"



#include <functional>

namespace memoria {


template <Int Levels, PackedSizeType SizeType>
class BTTLTestCtr {};


template <
    typename Profile,
    Int Levels,
	PackedSizeType SizeType
>
struct BTTLTestTypesBase: public BTTypes<Profile, BTTreeLayout> {

    using Base = BTTypes<Profile, BTTreeLayout>;

    using Key 	= BigInt;
    using Value	= Byte;

    using CtrSizeT = BigInt;


//    using StreamVariableTF = StreamTF<
//        TL<TL<
//			PkdVTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, 1, UByteI7Codec>>
//        >>,
//        TL<TL<TL<IndexRange<0, 1>>>>,
//		FSEBranchStructTF
//    >;
//
//    using StreamFixedTF = StreamTF<
//        TL<TL<
//        	PkdFTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, 1>>
//        >>,
//        TL<TL<TL<IndexRange<0, 1>>>>,
//		FSEBranchStructTF
//    >;

    using StreamVariableTF = StreamTF<
        TL<
			StreamSize
			//PkdVTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, 1, UByteI7Codec>>
        >,
        TL<TL<>>, //TL<IndexRange<0, 1>>
		FSEBranchStructTF
    >;

    using StreamFixedTF = StreamTF<
        TL<
			StreamSize
        	//PkdFTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, 1>>
        >,
        TL<TL<>>, //TL<IndexRange<0, 1>>
		FSEBranchStructTF
    >;

    using DataStreamTF  = StreamTF<
    	TL<TL<
			StreamSize,
			PackedFSEArray<PackedFSEArrayTypes<Value>>>
			>,
    	TL<TL<TL<>, TL<>>>,
		FSEBranchStructTF
    >;


    using RawStreamDescriptors = IfThenElse<
    		SizeType == PackedSizeType::FIXED,
			MergeLists<
				typename MakeList<StreamFixedTF, Levels - 1>::Type,
				DataStreamTF
			>,
			MergeLists<
				typename MakeList<StreamVariableTF, Levels - 1>::Type,
				DataStreamTF
			>
    >;


    using BTTLNavigationStruct = IfThenElse<
    		SizeType == PackedSizeType::FIXED,
			PkdFQTreeT<CtrSizeT, 1>,
			PkdVQTreeT<CtrSizeT, 1, UByteI7Codec>
    >;


    using StreamDescriptors = typename bttl::BTTLAugmentStreamDescriptors<
    		RawStreamDescriptors,
			BTTLNavigationStruct
	>::Type;

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
    Int Levels,
	PackedSizeType SizeType
>
struct BTTypes<Profile, BTTLTestCtr<Levels, SizeType>>: public BTTLTestTypesBase<Profile, Levels, SizeType>
{
};


template <typename Profile, Int Levels, PackedSizeType SizeType, typename T>
class CtrTF<Profile, BTTLTestCtr<Levels, SizeType>, T>: public CtrTF<Profile, memoria::BTTreeLayout, T> {
    using Base = CtrTF<Profile, memoria::BTTreeLayout, T>;
public:

//    struct Types: Base::Types
//    {
//    	using CtrTypes 			= TableCtrTypes<Types>;
//        using IterTypes 		= TableIterTypes<Types>;
//
//        using PageUpdateMgr 	= PageUpdateManager<CtrTypes>;
//    };
//
//    using CtrTypes 	= typename Types::CtrTypes;
//    using Type 		= Ctr<CtrTypes>;
};


template <Int Level, PackedSizeType SizeType>
struct TypeHash<BTTLTestCtr<Level, SizeType>>:   UIntValue<
    HashHelper<3001, Level, (Int)SizeType>::Value
> {};


}


#endif
