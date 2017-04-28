
// Copyright 2016 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_factory.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include "btfl_test_tools.hpp"
#include "btfl_test_names.hpp"


#include "container/btfl_test_c_api.hpp"
#include "iterator/btfl_test_i_api.hpp"

#include <functional>

namespace memoria {
namespace v1 {


template <Int DataStreams, PackedSizeType SizeType = PackedSizeType::VARIABLE>
class BTFLTestCtr {};


template <
    typename Profile,
    Int DataStreams,
    PackedSizeType SizeType
>
struct BTFLTestTypesBase: public BTTypes<Profile, BTFreeLayout> {

    using Base = BTTypes<Profile, BTFreeLayout>;

    using Key       = BigInt;
    using Value     = Byte;
    using Column    = BigInt;

    using CtrSizeT = BigInt;

    using StreamVariableTF = StreamTF<
        TL<TL<
            StreamSize,
                      PkdFQTreeT<Key>
        >>,
                bt::DefaultBranchStructTF
    >;

    using DataStreamTF = StreamTF<
        TL<TL<
            StreamSize,
            PackedFSEArray<PackedFSEArrayTypes<Value>>
            >
        >,
                bt::DefaultBranchStructTF
    >;

    using StructureStreamTF = StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename btfl::StructureStreamTF<DataStreams>::Type>
        >,
        bt::DefaultBranchStructTF
    >;


    using StreamDescriptors = MergeLists<
        typename MakeList<StreamVariableTF, DataStreams - 1>::Type,
        DataStreamTF,
        StructureStreamTF
    >;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                v1::btfl_test::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                v1::btfl_test::IterApiName
    >;

    template <Int Level>
    using IOData = btfl::BTFLData<
        DataStreams,
            Level,
            Key,
            Value,
            Column
    >;
};







template <
    typename Profile,
    Int Levels,
    PackedSizeType SizeType
>
struct BTTypes<Profile, BTFLTestCtr<Levels, SizeType>>: public BTFLTestTypesBase<Profile, Levels, SizeType>
{
};


template <typename Profile, Int Levels, PackedSizeType SizeType, typename T>
class CtrTF<Profile, BTFLTestCtr<Levels, SizeType>, T>: public CtrTF<Profile, v1::BTFreeLayout, T> {
    using Base = CtrTF<Profile, v1::BTFreeLayout, T>;
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


template <Int DataStreams, PackedSizeType SizeType>
struct TypeHash<BTFLTestCtr<DataStreams, SizeType>>:   UIntValue<
    HashHelper<30011, DataStreams, (Int)SizeType>::Value
> {};


}}
