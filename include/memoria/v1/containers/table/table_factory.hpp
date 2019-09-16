
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/containers/table/table_names.hpp>

#include <memoria/v1/containers/table/container/table_c_api.hpp>
#include <memoria/v1/containers/table/iterator/table_i_misc.hpp>

#include <memoria/v1/containers/table/table_tools.hpp>
#include <memoria/v1/containers/table/table_iterator.hpp>



#include <memoria/v1/core/tools/i7_codec.hpp>
#include <memoria/v1/core/packed/packed.hpp>


#include <tuple>

namespace memoria {
namespace v1 {



template <
    typename Profile,
    typename Key_,
    typename Value_,
    PackedDataTypeSize SizeType
>
struct TableBTTypesBase: public BTTypes<Profile, BTTreeLayout> {

    using Base = BTTypes<Profile, BTTreeLayout>;

    using ValueType = IfThenElse<
                    IfTypesEqual<Value_, IDType>::Value,
                    typename Base::ID,
                    Value_
    >;


    using Key   = Key_;
    using Value = Value_;

    using CtrSizeT = int64_t;

    static constexpr int32_t Levels = 3;

    using FirstStreamVariableTF = StreamTF<
        TL<
            //PkdVTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, 1, UByteI7Codec>>
        >,
        FSEBranchStructTF,
        TL<
            //IndexRange<0, 1>
        >

    >;

    using StreamVariableTF = StreamTF<
        TL<>,
        TL<>,
        FSEBranchStructTF
    >;


    using FirstStreamFixedTF = StreamTF<
        TL<
            //PkdFQTree<CtrSizeT, 1>
        >,
        FSEBranchStructTF,
        TL<
            //TL<IndexRange<0, 1>>
        >
    >;

    using StreamFixedTF = StreamTF<
        TL<>,
        FSEBranchStructTF,
        TL<>
    >;

    using DataStreamTF  = StreamTF<
        TL<TL<PkdFSQArrayT<Value, 1>>>,
        FSEBranchStructTF,
        TL<TL<TL<>>>
    >;


    using RawStreamDescriptors = IfThenElse<
            SizeType == PackedDataTypeSize::FIXED,
            MergeLists<
                FirstStreamFixedTF,
                typename MakeList<StreamFixedTF, Levels - 2>::Type,
                DataStreamTF
            >,
            MergeLists<
                FirstStreamVariableTF,
                typename MakeList<StreamVariableTF, Levels - 2>::Type,
                DataStreamTF
            >
    >;

    using StreamDescriptors = typename bttl::BTTLAugmentStreamDescriptors<
            RawStreamDescriptors,
            //PkdVDTreeT<CtrSizeT, 1, UByteI7Codec>
            PkdFQTreeT<CtrSizeT, 1>
    >::Type;


    using Metadata = BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >;


    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                table::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                table::ItrMiscName
    >;
};







template <
    typename Profile,
    typename Key_,
    typename Value_,
    PackedDataTypeSize SizeType
>
struct BTTypes<Profile, Table<Key_, Value_, SizeType>>: public TableBTTypesBase<Profile, Key_, Value_, SizeType>
{
};


template <typename Profile, typename Key, typename Value, PackedDataTypeSize SizeType, typename T>
class CtrTF<Profile, Table<Key, Value, SizeType>, T>: public CtrTF<Profile, BTTreeLayout, T> {
    using Base = CtrTF<Profile, BTTreeLayout, T>;
public:

    struct Types: Base::Types
    {
        using CtrTypes          = TableCtrTypes<Types>;
        using IterTypes         = TableIterTypes<Types>;

        using BlockUpdateMgr     = BlockUpdateManager<CtrTypes>;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}}
