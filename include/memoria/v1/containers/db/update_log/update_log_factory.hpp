
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
#include <memoria/v1/containers/db/update_log/update_log_names.hpp>

#include <memoria/v1/containers/db/update_log/update_log_input.hpp>

#include <memoria/v1/containers/db/update_log/container/update_log_c_api.hpp>
#include <memoria/v1/containers/db/update_log/iterator/update_log_i_api.hpp>

#include <memoria/v1/containers/db/update_log/update_log_tools.hpp>
#include <memoria/v1/containers/db/update_log/update_log_iterator.hpp>

#include <tuple>

namespace memoria {
namespace v1 {


template <typename Profile>
struct UpdateLogBTTypesBaseBase: public BTTypes<Profile, BTFreeLayout> {

    using Base = BTTypes<Profile, BTFreeLayout>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                update_log::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                update_log::ItrApiName
    >;
};



template <typename Profile>
struct UpdateLogBTTypesBase: public UpdateLogBTTypesBaseBase<Profile> {

    using Base = UpdateLogBTTypesBaseBase<Profile>;

    using SnapshotIdT    = UUID;
    using CtrNameT       = UAcc128T;
    using DataT          = uint8_t;

    using CtrSizeT = typename Base::CtrSizeT;

    using SnapshotIdStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename update_log::SnapshotIdStructTF<SnapshotIdT>::Type>
        >,
        update_log::UpdateLogBranchStructTF
    >;

    using CtrNameStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename update_log::CtrNameStructTF<CtrNameT>::Type>
        >,
        update_log::UpdateLogBranchStructTF
    >;

    using CommandDataStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename update_log::CommandDataStructTF<DataT>::Type>
        >,
        update_log::UpdateLogBranchStructTF
    >;

    using StructureStreamTF = bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename btfl::StructureStreamTF<3>::Type>
        >,
        update_log::UpdateLogBranchStructTF
    >;


    using StreamDescriptors = TL<
        SnapshotIdStreamTF,
        CtrNameStreamTF,
        CommandDataStreamTF,
        StructureStreamTF
    >;


    static constexpr int32_t DataStreams = ListSize<StreamDescriptors> - 1;
};







template <
    typename Profile
>
struct BTTypes<Profile, UpdateLog>: public UpdateLogBTTypesBase<Profile>
{
};


template <typename Profile, typename T>
class CtrTF<Profile, UpdateLog, T>: public CtrTF<Profile, BTFreeLayout, T> {
    using Base1 = CtrTF<Profile, BTFreeLayout, T>;
public:

    struct Types: Base1::Types
    {
	using BaseTypes = typename Base1::Types;

        using CtrTypes          = UpdateLogCtrTypes<Types>;
        using IterTypes         = UpdateLogIterTypes<Types>;

        using BlockUpdateMgr     = bt::BlockUpdateManager<CtrTypes>;

        using LeafStreamsStructList = typename BaseTypes::LeafStreamsStructList;

        using IteratorBranchNodeEntry = typename BaseTypes::IteratorBranchNodeEntry;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}}
