
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

#include <memoria/v1/prototypes/bt_fl/btfl_factory.hpp>
#include <memoria/v1/containers/multimap/mmap_names.hpp>

#include <memoria/v1/containers/multimap/mmap_input.hpp>

#include <memoria/v1/containers/multimap/container/mmap_c_api.hpp>
#include <memoria/v1/containers/multimap/iterator/mmap_i_misc.hpp>

#include <memoria/v1/containers/multimap/mmap_tools.hpp>
#include <memoria/v1/containers/multimap/mmap_iterator.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>

#include <tuple>

namespace memoria {
namespace v1 {


template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct MultimapBTTypesBaseBase: public BTTypes<Profile, v1::BTFreeLayout> {

    using Base = BTTypes<Profile, v1::BTFreeLayout>;


    using Key   = Key_;
    using Value = Value_;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                v1::mmap::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                v1::mmap::ItrMiscName
    >;
};



template <
    typename Profile,
    typename Key,
    typename Value
>
struct MultimapBTTypesBase: public MultimapBTTypesBaseBase<Profile, Key, Value> {

    using Base = MultimapBTTypesBaseBase<Profile, Key, Value>;

    using CtrSizeT = typename Base::CtrSizeT;

    using FirstStreamTF = StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename mmap::MMapKeyStructTF<Key>::Type>
        >,
        mmap::MMapBranchStructTF
    >;

    using DataStreamTF = StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename mmap::MMapValueStructTF<Value>::Type>
        >,
        mmap::MMapBranchStructTF
    >;

    using StructureStreamTF = StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename btfl::StructureStreamTF<2>::Type>
        >,
        mmap::MMapBranchStructTF
    >;


    using StreamDescriptors = TL<
        FirstStreamTF,
        DataStreamTF,
        StructureStreamTF
    >;


    static constexpr int32_t DataStreams = 2;


    template <int32_t Level>
    using IOData = btfl::BTFLData<
        DataStreams,
            Level,
            Key,
            Value
    >;

};







template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, v1::Multimap<Key_, Value_>>: public MultimapBTTypesBase<Profile, Key_, Value_>
{
};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, v1::Multimap<Key, Value>, T>: public CtrTF<Profile, v1::BTFreeLayout, T> {
    using Base1 = CtrTF<Profile, v1::BTFreeLayout, T>;
public:

    struct Types: Base1::Types
    {
		using BaseTypes = typename Base1::Types;

        using CtrTypes          = MultimapCtrTypes<Types>;
        using IterTypes         = MultimapIterTypes<Types>;

        using PageUpdateMgr     = PageUpdateManager<CtrTypes>;

        using LeafStreamsStructList = typename BaseTypes::LeafStreamsStructList;

        using IteratorBranchNodeEntry = typename BaseTypes::IteratorBranchNodeEntry;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}}
