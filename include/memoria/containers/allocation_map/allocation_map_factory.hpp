
// Copyright 2020 Victor Smirnov
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

#include <memoria/prototypes/bt_ss/btss_factory.hpp>
#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <memoria/containers/allocation_map/container/allocation_map_cr_api.hpp>
#include <memoria/containers/allocation_map/container/allocation_map_cw_api.hpp>
#include <memoria/containers/allocation_map/allocation_map_names.hpp>
#include <memoria/containers/allocation_map/allocation_map_tools.hpp>
#include <memoria/containers/allocation_map/allocation_map_api_impl.hpp>
#include <memoria/containers/allocation_map/allocation_map_chunk_impl.hpp>


#include <memoria/core/packed/packed.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/varchars/varchar_dt.hpp>

namespace memoria {

template <
    typename Profile
>
struct AllocationMapBTTypesBaseBase: public BTTypes<Profile, BTSingleStream> {

    using Base = BTTypes<Profile, BTSingleStream>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                alcmap::CtrRApiName
    >;

    using RWCommonContainerPartsList = MergeLists<
                typename Base::RWCommonContainerPartsList,
                alcmap::CtrWApiName
    >;
};



template <
    typename Profile,
    int32_t Special = 0
>
struct AllocationMapBTTypesBase: public AllocationMapBTTypesBaseBase<Profile> {

    using LeafStruct = PkdAllocationMap<PkdAllocationMapTypes>;

    using StreamDescriptors = TL<
            bt::StreamTF<
                TL<
                    TL<StreamSize>,
                    TL<LeafStruct>
                >,
                alcmap::MapBranchStructTF
            >
    >;
};


template <
    typename Profile
>
struct BTTypes<Profile, AllocationMap>: public AllocationMapBTTypesBase<Profile>{};


template <typename Profile, typename T>
class CtrTF<Profile, AllocationMap, T>: public CtrTF<Profile, BTSingleStream, T> {
};

}
