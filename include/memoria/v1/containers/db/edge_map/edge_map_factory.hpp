
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
#include <memoria/v1/containers/db/edge_map/edge_map_names.hpp>

#include <memoria/v1/containers/db/edge_map/edge_map_input.hpp>

#include <memoria/v1/containers/db/edge_map/container/edge_map_c_api.hpp>
#include <memoria/v1/containers/db/edge_map/iterator/edge_map_i_misc.hpp>

#include <memoria/v1/containers/db/edge_map/edge_map_tools.hpp>
#include <memoria/v1/containers/db/edge_map/edge_map_iterator.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>

#include <tuple>

namespace memoria {
namespace v1 {


template <
    typename Profile
>
struct EdgeMapBTTypesBaseBase: public BTTypes<Profile, BTFreeLayout> {

    using Base = BTTypes<Profile, BTFreeLayout>;

    using Key   = UUID;
    using Value = UAcc128T;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                edge_map::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                edge_map::ItrMiscName
    >;
};



template <typename Profile>
struct EdgeMapBTTypesBase: public EdgeMapBTTypesBaseBase<Profile> {

    using Base = EdgeMapBTTypesBaseBase<Profile>;

    using Key   = typename EdgeMapBTTypesBaseBase<Profile>::Key;
    using Value = typename EdgeMapBTTypesBaseBase<Profile>::Value;

    using CtrSizeT = typename Base::CtrSizeT;

    using FirstStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename edge_map::EdgeMapKeyStructTF<Key>::Type>
        >,
        edge_map::EdgeMapBranchStructTF
    >;

    using DataStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename edge_map::EdgeMapValueStructTF<Value>::Type>
        >,
        edge_map::EdgeMapBranchStructTF
    >;

    using StructureStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename btfl::StructureStreamTF<2>::Type>
        >,
        edge_map::EdgeMapBranchStructTF
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
    typename Profile
>
struct BTTypes<Profile, EdgeMap>: public EdgeMapBTTypesBase<Profile>
{
};


template <typename Profile, typename T>
class CtrTF<Profile, EdgeMap, T>: public CtrTF<Profile, BTFreeLayout, T> {
    using Base1 = CtrTF<Profile, BTFreeLayout, T>;
public:

    struct Types: Base1::Types
    {
		using BaseTypes = typename Base1::Types;

        using CtrTypes          = EdgeMapCtrTypes<Types>;
        using IterTypes         = EdgeMapIterTypes<Types>;

        using PageUpdateMgr     = bt::PageUpdateManager<CtrTypes>;

        using LeafStreamsStructList = typename BaseTypes::LeafStreamsStructList;

        using IteratorBranchNodeEntry = typename BaseTypes::IteratorBranchNodeEntry;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}}
