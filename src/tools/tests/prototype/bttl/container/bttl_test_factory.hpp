
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tools/profile_tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/prototypes/bt_tl/tools/bttl_tools_random_gen.hpp>

#include "bttl_test_tools.hpp"
#include "bttl_test_names.hpp"


#include "container/bttl_test_c_api.hpp"
#include "iterator/bttl_test_i_api.hpp"

#include <functional>

namespace memoria {
namespace v1 {


template <Int Levels, PackedSizeType SizeType>
class BTTLTestCtr {};


template <
    typename Profile,
    Int Levels,
    PackedSizeType SizeType
>
struct BTTLTestTypesBase: public BTTypes<Profile, BTTreeLayout> {

    using Base = BTTypes<Profile, BTTreeLayout>;

    using Key   = BigInt;
    using Value = Byte;

    using CtrSizeT = BigInt;


    using StreamVariableTF = StreamTF<
        TL<
            StreamSize
        >,
        FSEBranchStructTF,
        TL<TL<>>
    >;

    using StreamFixedTF = StreamTF<
        TL<
            StreamSize
        >,
        FSEBranchStructTF,
        TL<TL<>>
    >;

    using DataStreamTF  = StreamTF<
        TL<TL<
            StreamSize,
            PackedFSEArray<PackedFSEArrayTypes<Value>>
            >
        >,
        FSEBranchStructTF,
        TL<TL<TL<>, TL<>>>
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

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                v1::bttl_test::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                v1::bttl_test::IterApiName
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
class CtrTF<Profile, BTTLTestCtr<Levels, SizeType>, T>: public CtrTF<Profile, v1::BTTreeLayout, T> {
    using Base = CtrTF<Profile, v1::BTTreeLayout, T>;
public:

//    struct Types: Base::Types
//    {
//      using CtrTypes          = TableCtrTypes<Types>;
//        using IterTypes       = TableIterTypes<Types>;
//
//        using PageUpdateMgr   = PageUpdateManager<CtrTypes>;
//    };
//
//    using CtrTypes    = typename Types::CtrTypes;
//    using Type        = Ctr<CtrTypes>;
};


template <Int Level, PackedSizeType SizeType>
struct TypeHash<BTTLTestCtr<Level, SizeType>>:   UIntValue<
    HashHelper<3001, Level, (Int)SizeType>::Value
> {};


}}