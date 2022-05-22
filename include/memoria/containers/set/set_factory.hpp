
// Copyright 2014 Victor Smirnov
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

#include <memoria/containers/set/container/set_c_api.hpp>
#include <memoria/containers/set/container/set_c_insert.hpp>
#include <memoria/containers/set/container/set_c_remove.hpp>
#include <memoria/containers/set/iterator/set_i_nav.hpp>
#include <memoria/containers/set/set_names.hpp>
#include <memoria/containers/set/set_tools.hpp>

#include <memoria/containers/collection/collection_c_api.hpp>

#include <memoria/prototypes/bt_ss/btss_factory.hpp>

#include <memoria/core/packed/packed.hpp>

#ifdef HAVE_BOOST
#include <memoria/core/bignum/bigint.hpp>
#endif


#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/string_codec.hpp>
#include <memoria/core/bytes/bytes_codec.hpp>

#include <memoria/core/tools/uuid.hpp>

#include <memoria/api/set/set_api.hpp>

#include <tuple>

#include "container/set_c_insert.hpp"
#include "container/set_c_remove.hpp"

namespace memoria {


template <
    typename Profile,
    typename Key_
>
struct SetBTTypesBaseBase: public BTTypes<Profile, BTSingleStream> {

    using Base = BTTypes<Profile, BTSingleStream>;

    using Key = Key_;

    using IteratorInterface = EmptyType;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                collection::CtrApiName,
                set::CtrApiName,
                set::CtrInsertName,
                set::CtrRemoveName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                set::ItrNavName
    >;
};



template <
    typename Profile,
    typename Key_,
    int32_t Special = 0
>
struct SetBTTypesBase: public SetBTTypesBaseBase<Profile, Key_> {

    using LeafKeyStruct = typename set::SetKeyStructTF<Key_>::Type;

    using StreamDescriptors = TL<
            bt::StreamTF<
                TL<
                    TL<StreamSize>,
                    TL<LeafKeyStruct>
                >,
                bt::DefaultBranchStructTF
            >
    >;
};





template <
    typename Profile,
    typename Key_
>
struct BTTypes<Profile, Set<Key_>>: public SetBTTypesBase<Profile, Key_>{};


template <typename Profile, typename Key, typename T>
class CtrTF<Profile, Set<Key>, T>: public CtrTF<Profile, BTSingleStream, T> {
};



}
