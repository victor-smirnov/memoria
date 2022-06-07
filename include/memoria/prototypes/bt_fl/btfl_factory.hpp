
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

#include <memoria/prototypes/bt/bt_factory.hpp>

#include <memoria/prototypes/bt_fl/btfl_names.hpp>

#include <memoria/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/prototypes/bt_fl/container/btfl_c_misc.hpp>
#include <memoria/prototypes/bt_fl/container/btfl_c_insert.hpp>
#include <memoria/prototypes/bt_fl/container/btfl_c_leaf_common.hpp>
#include <memoria/prototypes/bt_fl/container/btfl_c_leaf_fixed.hpp>
#include <memoria/prototypes/bt_fl/container/btfl_c_leaf_variable.hpp>
#include <memoria/prototypes/bt_fl/container/btfl_c_branch_common.hpp>
#include <memoria/prototypes/bt_fl/container/btfl_c_branch_fixed.hpp>
#include <memoria/prototypes/bt_fl/container/btfl_c_branch_variable.hpp>
#include <memoria/prototypes/bt_fl/container/btfl_c_ranks.hpp>
#include <memoria/prototypes/bt_fl/container/btfl_c_checks.hpp>

#include <memoria/prototypes/bt_fl/iterator/btfl_i_basic.hpp>

#include <memoria/prototypes/bt_fl/tools/btfl_tools_streamdescr.hpp>

#include <memoria/prototypes/bt_fl/io/btfl_io_input_provider_base.hpp>

#include <memoria/prototypes/bt_fl/btfl_structure_chunk_iter.hpp>

#include <tuple>

namespace memoria {

struct BTFreeLayout {};

template <
    typename Profile
>
struct BTTypes<Profile, BTFreeLayout>: public BTTypes<Profile, BT> {

    using Base = BTTypes<Profile, BT>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                btfl::MiscName,
                btfl::InsertName,
                btfl::BranchCommonName,
                btfl::LeafCommonName,
                btfl::RanksName,
                btfl::ChecksName
    >;

    using FixedBranchContainerPartsList = MergeLists<
                typename Base::FixedBranchContainerPartsList,
                btfl::BranchFixedName
    >;

    using VariableBranchContainerPartsList = MergeLists<
                typename Base::VariableBranchContainerPartsList,
                btfl::BranchVariableName
    >;

    using FixedLeafContainerPartsList = MergeLists<
                    typename Base::FixedLeafContainerPartsList,
                    btfl::LeafFixedName
    >;

    using VariableLeafContainerPartsList = MergeLists<
                    typename Base::VariableLeafContainerPartsList,
                    btfl::LeafVariableName
    >;

    using BlockIteratorStatePartsList = MergeLists<
        typename Base::BlockIteratorStatePartsList,
        btfl::IteratorBasicName
    >;
};




template <typename Profile, typename T>
class CtrTF<Profile, BTFreeLayout, T>: public CtrTF<Profile, BT, T> {

    using Base1 = CtrTF<Profile, BT, T>;

public:

    struct Types: Base1::Types
    {
		using BaseTypes = typename Base1::Types;

        using CtrTypes              = BTCtrTypes<Types>;
        using IterTypes             = BTIterTypes<Types>;
        using BlockIterStateTypes   = BTBlockIterStateTypes<Types>;

        using BlockUpdateMgr     = bt::BlockUpdateManager<CtrTypes>;

        static constexpr int32_t DataStreams            = BaseTypes::Streams - 1;
        static constexpr int32_t StructureStreamIdx     = DataStreams;

        using DataSizesT = core::StaticVector<typename BaseTypes::CtrSizeT, DataStreams>;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}
