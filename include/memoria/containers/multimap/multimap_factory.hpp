
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

#include <memoria/prototypes/bt_fl/btfl_factory.hpp>
#include <memoria/containers/multimap/multimap_names.hpp>
#include <memoria/containers/multimap/container/multimap_c_api.hpp>

#include <memoria/containers/multimap/multimap_tools.hpp>

#include <memoria/api/multimap/multimap_api.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/varchars/varchar_dt.hpp>

#include <memoria/containers/multimap/multimap_keys_chunk_impl.hpp>
#include <memoria/containers/multimap/multimap_values_chunk_impl.hpp>

#include <tuple>

namespace memoria {

template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct MultimapBTTypesBaseBase: public BTTypes<Profile, BTFreeLayout> {

    using Base = BTTypes<Profile, BTFreeLayout>;

    using Key   = Key_;
    using Value = Value_;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                multimap::CtrApiName
    >;

};



template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct MultimapBTTypesBase: public MultimapBTTypesBaseBase<Profile, Key_, Value_> {

    using Base = MultimapBTTypesBaseBase<Profile, Key_, Value_>;

    using CtrSizeT = typename Base::CtrSizeT;

    using FirstStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename multimap::MMapKeyStructTF<Key_>::Type>
        >,
        multimap::MMapBranchStructTF
    >;

    using DataStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename multimap::MMapValueStructTF<Value_>::Type>
        >,
        multimap::MMapBranchStructTF
    >;

    using StructureStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename btfl::StructureStreamTF<2>::Type>
        >,
        multimap::MMapBranchStructTF
    >;


    using StreamDescriptors = TL<
        FirstStreamTF,
        DataStreamTF,
        StructureStreamTF
    >;


    static constexpr int32_t DataStreams = 2;
};







template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, Multimap<Key_, Value_>>: public MultimapBTTypesBase<Profile, Key_, Value_>
{
};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, Multimap<Key, Value>, T>: public CtrTF<Profile, BTFreeLayout, T> {
};

}
