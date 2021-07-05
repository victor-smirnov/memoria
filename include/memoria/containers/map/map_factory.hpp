
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

#include <memoria/containers/map/container/map_c_insert.hpp>
#include <memoria/containers/map/container/mapm_c_insert.hpp>
#include <memoria/containers/map/container/map_c_remove.hpp>
#include <memoria/containers/map/container/map_c_api.hpp>
#include <memoria/containers/map/iterator/map_i_nav.hpp>
#include <memoria/containers/map/iterator/mapm_i_nav.hpp>
#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>
#include <memoria/containers/map/map_api_impl.hpp>

#include <memoria/prototypes/bt_ss/btss_factory.hpp>

#include <memoria/core/packed/packed.hpp>

#ifdef HAVE_BOOST
#include <memoria/core/bignum/bigint.hpp>
#endif

#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/string_codec.hpp>

#include <memoria/core/tools/uuid.hpp>

#include <memoria/api/map/map_api.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/varchars/varchar_dt.hpp>

#include <tuple>

namespace memoria {


template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct MapBTTypesBaseBase: public BTTypes<Profile, BTSingleStream> {

    using Base = BTTypes<Profile, BTSingleStream>;

    using Key = Key_;
    using Value = Value_;

    using IteratorInterface = MapIterator<Key_, Value_, ApiProfile<Profile>>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                map::CtrInsertMaxName,
                map::CtrRemoveName,
                map::CtrApiName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                map::ItrNavMaxName
    >;
};



template <
    typename Profile,
    typename Key_,
    typename Value_,
    int32_t Special = 0
>
struct MapBTTypesBase: public MapBTTypesBaseBase<Profile, Key_, Value_> {

    using LeafKeyStruct = typename map::MapKeyStructTF<Key_>::Type;

    using LeafValueStruct = typename map::MapValueStructTF<Value_>::Type;

    using StreamDescriptors = TL<
            bt::StreamTF<
                TL<
                    TL<StreamSize>,
                    TL<LeafKeyStruct>,
                    TL<LeafValueStruct>
                >,
                map::MapBranchStructTF
            >
    >;
};


template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, Map<Key_, Value_>>: public MapBTTypesBase<Profile, Key_, Value_>{};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, Map<Key, Value>, T>: public CtrTF<Profile, BTSingleStream, T> {
};

}
