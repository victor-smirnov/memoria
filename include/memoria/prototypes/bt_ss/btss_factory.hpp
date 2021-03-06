
// Copyright 2017 Victor Smirnov
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

#include <memoria/prototypes/bt_ss/btss_names.hpp>

#include <memoria/prototypes/bt_ss/container/btss_c_leaf_common.hpp>
#include <memoria/prototypes/bt_ss/container/btss_c_leaf_fixed.hpp>
#include <memoria/prototypes/bt_ss/container/btss_c_leaf_variable.hpp>
#include <memoria/prototypes/bt_ss/container/btss_c_find.hpp>
#include <memoria/prototypes/bt_ss/container/btss_c_remove.hpp>


#include <memoria/prototypes/bt_ss/iterator/btss_i_misc.hpp>

#include <memoria/prototypes/bt_ss/btss_input_iovector.hpp>

#include <memoria/api/common/ctr_api_btss.hpp>

#include <tuple>

namespace memoria {

struct BTSingleStream {};

template <
    typename Profile
>
struct BTTypes<Profile, BTSingleStream>: public BTTypes<Profile, BT> {

    using Base = BTTypes<Profile, BT>;

    using IteratorInterface = BTSSIterator<ApiProfile<Profile>>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                btss::LeafCommonName,
                btss::FindName,
                btss::RemoveName
    >;

    using FixedLeafContainerPartsList = MergeLists<
                typename Base::FixedLeafContainerPartsList,
                btss::LeafFixedName
    >;


    using VariableLeafContainerPartsList = MergeLists<
                typename Base::VariableLeafContainerPartsList,
                btss::LeafVariableName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                btss::IteratorMiscName
    >;



    template <typename Types, typename LeafPath>
    using CountForwardWalker   = bt::CountForwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using CountBackwardWalker  = bt::CountBackwardWalker<bt::WalkerTypes<Types, LeafPath>>;
};




template <typename Profile, typename T>
class CtrTF<Profile, BTSingleStream, T>: public CtrTF<Profile, BT, T> {

    using Base = CtrTF<Profile, BT, T>;
public:

    struct Types: Base::Types
    {
        using CtrTypes          = BTSSCtrTypes<Types>;
        using IterTypes         = BTSSIterTypes<Types>;

        using BlockUpdateMgr     = bt::BlockUpdateManager<CtrTypes>;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}
