
// Copyright 2013 Victor Smirnov
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

#include <memoria/containers/wt/wt_names.hpp>
#include <memoria/containers/wt/container/wt_c_checks.hpp>


namespace memoria {

template <typename Profile, typename... LabelDescriptors>
struct BTTypes<Profile, wt::WTLabeledTree<LabelDescriptors...>>:
    BTTypes<Profile, LabeledTree<LabelDescriptors...>> {

    typedef BTTypes<Profile, LabeledTree<LabelDescriptors...>>         Base;

    using CommonContainerPartsList = MergeLists<
                    typename Base::CommonContainerPartsList,
                    wt::CtrChecksName
    >;

};


template <typename Profile, typename... LabelDescriptors, typename T>
class CtrTF<Profile, wt::WTLabeledTree<LabelDescriptors...>, T>:
    public CtrTF<Profile, LabeledTree<LabelDescriptors...>, T> {
};

}
