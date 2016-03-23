
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/bt_factory.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_input.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_names.hpp>
#include <memoria/v1/prototypes/bt_ss/btss_iterator.hpp>



#include <memoria/v1/prototypes/bt_ss/container/btss_c_leaf_common.hpp>
#include <memoria/v1/prototypes/bt_ss/container/btss_c_leaf_fixed.hpp>
#include <memoria/v1/prototypes/bt_ss/container/btss_c_leaf_variable.hpp>
#include <memoria/v1/prototypes/bt_ss/container/btss_c_find.hpp>

#include <memoria/v1/prototypes/bt_ss/iterator/btss_i_misc.hpp>

#include <tuple>

namespace memoria {
namespace v1 {

struct BTSingleStream {};

template <
    typename Profile
>
struct BTTypes<Profile, v1::BTSingleStream>: public BTTypes<Profile, v1::BT> {

    using Base = BTTypes<Profile, v1::BT>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                v1::btss::LeafCommonName,
                v1::btss::FindName
    >;

    using FixedLeafContainerPartsList = MergeLists<
                typename Base::FixedLeafContainerPartsList,
                v1::btss::LeafFixedName
    >;


    using VariableLeafContainerPartsList = MergeLists<
                typename Base::VariableLeafContainerPartsList,
                v1::btss::LeafVariableName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                v1::btss::IteratorMiscName
    >;

};




template <typename Profile, typename T>
class CtrTF<Profile, v1::BTSingleStream, T>: public CtrTF<Profile, v1::BT, T> {

    using Base = CtrTF<Profile, v1::BT, T>;
public:

    struct Types: Base::Types
    {
        using CtrTypes          = BTSSCtrTypes<Types>;
        using IterTypes         = BTSSIterTypes<Types>;

        using PageUpdateMgr     = PageUpdateManager<CtrTypes>;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}}