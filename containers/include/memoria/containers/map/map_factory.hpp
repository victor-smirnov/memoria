
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

#include <memoria/containers/map/container/map_cr_api.hpp>
#include <memoria/containers/map/container/map_cw_api.hpp>
#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>


#include <memoria/prototypes/bt_ss/btss_factory.hpp>

#include <memoria/core/packed/packed.hpp>



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
struct BTTypes<Profile, Map<Key_, Value_>>:
        public BTTypes<Profile, BTSingleStream>,
        public ICtrApiTypes<Map<Key_, Value_>, Profile>
{
    using Base = BTTypes<Profile, BTSingleStream>;

    using Key = Key_;
    using Value = Value_;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                map::CtrRApiName
    >;

    using RWCommonContainerPartsList = MergeLists<
                typename Base::RWCommonContainerPartsList,
                map::CtrWApiName
    >;

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


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, Map<Key, Value>, T>: public CtrTF<Profile, BTSingleStream, T> {
};

}
