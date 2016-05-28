
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
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_find.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_skip.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_update.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_remove.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_insert.hpp>
#include <memoria/v1/prototypes/bt_fl/iterator/btfl_i_read.hpp>

#include <memoria/v1/prototypes/bt_fl/tools/btfl_tools_random_gen.hpp>
#include <memoria/v1/prototypes/bt_fl/tools/btfl_tools_streamdescr.hpp>

#include <memoria/v1/prototypes/bt/walkers/bt_count_walkers.hpp>

#include <tuple>

namespace memoria {
namespace v1 {

struct BTFreeLayout {};

template <
    typename Profile
>
struct BTTypes<Profile, v1::BTFreeLayout>: public BTTypes<Profile, v1::BT> {

    using Base = BTTypes<Profile, v1::BT>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                v1::btfl::MiscName,
                v1::btfl::InsertName,
                v1::btfl::BranchCommonName,
                v1::btfl::LeafCommonName,
                v1::btfl::RanksName,
                v1::btfl::ChecksName
    >;

    using FixedBranchContainerPartsList = MergeLists<
                typename Base::FixedBranchContainerPartsList,
                v1::btfl::BranchFixedName
    >;

    using VariableBranchContainerPartsList = MergeLists<
                typename Base::VariableBranchContainerPartsList,
                v1::btfl::BranchVariableName
    >;

    using FixedLeafContainerPartsList = MergeLists<
                    typename Base::FixedLeafContainerPartsList,
                    v1::btfl::LeafFixedName
    >;

    using VariableLeafContainerPartsList = MergeLists<
                    typename Base::VariableLeafContainerPartsList,
                    v1::btfl::LeafVariableName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                v1::btfl::IteratorMiscName,
                v1::btfl::IteratorStreamRankName,
                v1::btfl::IteratorFindName,
                v1::btfl::IteratorSkipName,
                v1::btfl::IteratorUpdateName,
                v1::btfl::IteratorRemoveName,
                v1::btfl::IteratorInsertName,
                v1::btfl::IteratorReadName
    >;




    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef v1::btfl::BTFLIteratorPrefixCache<Iterator, Container>   Type;
    };

    template <typename Types, typename LeafPath>
    using CountWalker  = bt::CountForwardWalker<bt::WalkerTypes<Types, LeafPath>>;

};




template <typename Profile, typename T>
class CtrTF<Profile, v1::BTFreeLayout, T>: public CtrTF<Profile, v1::BT, T> {

    using Base = CtrTF<Profile, v1::BT, T>;
public:

    struct Types: Base::Types
    {
        using CtrTypes          = BTFLCtrTypes<Types>;
        using IterTypes         = BTFLIterTypes<Types>;

        using PageUpdateMgr     = PageUpdateManager<CtrTypes>;



        static const Int DataStreams            = Base::Types::Streams - 1;
        static const Int StructureStreamIdx     = DataStreams;


        using DataSizesT = core::StaticVector<typename Base::Types::CtrSizeT, DataStreams>;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}}
