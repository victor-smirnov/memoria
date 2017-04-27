
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

#include <memoria/v1/prototypes/bt_cow/btcow_factory.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_input.hpp>

#include <tuple>

#include "btsscow_iterator.hpp"
#include "btsscow_names.hpp"
#include "container/btsscow_c_find.hpp"
#include "container/btsscow_c_leaf_common.hpp"
#include "container/btsscow_c_leaf_fixed.hpp"
#include "container/btsscow_c_leaf_variable.hpp"
#include "iterator/btsscow_i_misc.hpp"

namespace memoria {
namespace v1 {

struct BTCowSingleStream {};

template <
    typename Profile
>
struct BTCowTypes<Profile, v1::BTCowSingleStream>: public BTCowTypes<Profile, v1::BTCow> {

    using Base = BTCowTypes<Profile, v1::BT>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                v1::btss_cow::LeafCommonName,
                v1::btss_cow::FindName
    >;

    using FixedLeafContainerPartsList = MergeLists<
                typename Base::FixedLeafContainerPartsList,
                v1::btss_cow::LeafFixedName
    >;


    using VariableLeafContainerPartsList = MergeLists<
                typename Base::VariableLeafContainerPartsList,
                v1::btss_cow::LeafVariableName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                v1::btss_cow::IteratorMiscName
    >;

};




template <typename Profile, typename T>
class CtrTF<Profile, v1::BTCowSingleStream, T>: public CtrTF<Profile, v1::BTCow, T> {

    using Base = CtrTF<Profile, v1::BTCow, T>;
public:

    struct Types: Base::Types
    {
        using CtrTypes          = BTCowSSCtrTypes<Types>;
        using IterTypes         = BTCowSSIterTypes<Types>;

        using PageUpdateMgr     = PageUpdateManager<CtrTypes>;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}}