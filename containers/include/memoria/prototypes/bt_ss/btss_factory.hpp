
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
#include <memoria/prototypes/bt_ss/container/btss_c_insert.hpp>


//#include <memoria/prototypes/bt_ss/iterator/btss_i_misc.hpp>
#include <memoria/prototypes/bt_ss/iterator/btss_i_basic.hpp>

#include <memoria/prototypes/bt_ss/btss_batch_input.hpp>

#include <memoria/api/common/ctr_api_btss.hpp>

#include <tuple>

namespace memoria {

struct BTSingleStream {};

template <
    typename Profile
>
struct BTTypes<Profile, BTSingleStream>: public BTTypes<Profile, BT> {

    using Base = BTTypes<Profile, BT>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                btss::FindName
    >;

    using RWCommonContainerPartsList = MergeLists<
                typename Base::RWCommonContainerPartsList,
                btss::InsertName,
                btss::LeafCommonName,
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


    using BlockIteratorStatePartsList = MergeLists<
        typename Base::BlockIteratorStatePartsList,
        btss::IteratorBasicName
    >;
};




template <typename Profile, typename T>
class CtrTF<Profile, BTSingleStream, T>: public CtrTF<Profile, BT, T> {
};


}
