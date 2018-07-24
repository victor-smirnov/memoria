
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

#include <memoria/v1/prototypes/bt/bt_factory.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_input.hpp>
#include <memoria/v1/prototypes/bt_fl/io/btfl_output.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_iterator.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/container/btfl_c_misc.hpp>
#include <memoria/v1/prototypes/bt_fl/container/btfl_c_insert.hpp>
#include <memoria/v1/prototypes/bt_fl/container/btfl_c_leaf_common.hpp>
#include <memoria/v1/prototypes/bt_fl/container/btfl_c_leaf_fixed.hpp>
#include <memoria/v1/prototypes/bt_fl/container/btfl_c_leaf_variable.hpp>
#include <memoria/v1/prototypes/bt_fl/container/btfl_c_branch_common.hpp>
#include <memoria/v1/prototypes/bt_fl/container/btfl_c_branch_fixed.hpp>
#include <memoria/v1/prototypes/bt_fl/container/btfl_c_branch_variable.hpp>
#include <memoria/v1/prototypes/bt_fl/container/btfl_c_ranks.hpp>
#include <memoria/v1/prototypes/bt_fl/container/btfl_c_checks.hpp>

#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_misc.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_srank.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_sums.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_find.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_skip.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_update.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_remove.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_insert.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_read.hpp>

#include <memoria/v1/prototypes/bt_fl/tools/btfl_tools_random_gen.hpp>
#include <memoria/v1/prototypes/bt_fl/tools/btfl_tools_streamdescr.hpp>

#include <memoria/v1/prototypes/bt/walkers/bt_count_walkers.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_selectge_walkers.hpp>

#include <memoria/v1/prototypes/bt_fl/iodata/btfl_iodata.hpp>



#include <tuple>

namespace memoria {
namespace v1 {

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


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                btfl::IteratorMiscName,
                btfl::IteratorStreamRankName,
                btfl::IteratorStreamSumsName,
                btfl::IteratorFindName,
                btfl::IteratorSkipName,
                btfl::IteratorUpdateName,
                btfl::IteratorRemoveName,
                btfl::IteratorInsertName,
                btfl::IteratorReadName
    >;




    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef btfl::BTFLIteratorPrefixCache<Iterator, Container>   Type;
    };

    template <typename Types, typename LeafPath>
    using CountForwardWalker  = bt::CountForwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using CountBackwardWalker  = bt::CountBackwardWalker<bt::WalkerTypes<Types, LeafPath>>;


    template <typename Types, typename LeafPath>
    using SelectGEForwardWalker  = bt::SelectGEForwardWalker<bt::WalkerTypes<Types, LeafPath>>;
};




template <typename Profile, typename T>
class CtrTF<Profile, BTFreeLayout, T>: public CtrTF<Profile, BT, T> {

    using Base1 = CtrTF<Profile, BT, T>;

public:

    struct Types: Base1::Types
    {
		using BaseTypes = typename Base1::Types;

        using CtrTypes          = BTFLCtrTypes<Types>;
        using IterTypes         = BTFLIterTypes<Types>;

        using PageUpdateMgr     = bt::PageUpdateManager<CtrTypes>;

        static const int32_t DataStreams            = BaseTypes::Streams - 1;
        static const int32_t StructureStreamIdx     = DataStreams;

        using DataSizesT = core::StaticVector<typename BaseTypes::CtrSizeT, DataStreams>;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}}
