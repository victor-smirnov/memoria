
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/containers/wt/wt_names.hpp>
#include <memoria/v1/containers/wt/container/wt_c_checks.hpp>


namespace memoria {
namespace v1 {

template <typename Profile, typename... LabelDescriptors>
struct BTTypes<Profile, wt::WTLabeledTree<LabelDescriptors...>>:
    BTTypes<Profile, v1::LabeledTree<LabelDescriptors...>> {

    typedef BTTypes<Profile, v1::LabeledTree<LabelDescriptors...>>         Base;

    using CommonContainerPartsList = MergeLists<
                    typename Base::CommonContainerPartsList,
                    wt::CtrChecksName
    >;

};


template <typename Profile, typename... LabelDescriptors, typename T>
class CtrTF<Profile, wt::WTLabeledTree<LabelDescriptors...>, T>:
    public CtrTF<Profile, v1::LabeledTree<LabelDescriptors...>, T> {
};



}}