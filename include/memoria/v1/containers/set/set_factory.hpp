
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

#include <memoria/v1/containers/set/container/set_c_insert.hpp>
#include <memoria/v1/containers/set/container/set_c_remove.hpp>
#include <memoria/v1/containers/set/iterator/set_i_nav.hpp>
#include <memoria/v1/containers/set/set_iterator.hpp>
#include <memoria/v1/containers/set/set_names.hpp>
#include <memoria/v1/containers/set/set_tools.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_factory.hpp>
#include <memoria/v1/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/fse_max/packed_fse_max_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/v1/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>
#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/array/packed_vle_dense_array.hpp>
#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>

#include <memoria/v1/core/tools/bignum/bigint.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>
#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/bytes/bytes_codec.hpp>

#include <memoria/v1/core/tools/uuid.hpp>

#include <tuple>

#include "container/set_c_insert.hpp"
#include "container/set_c_remove.hpp"

namespace memoria {
namespace v1 {


template <
    typename Profile,
    typename Key_
>
struct SetBTTypesBaseBase: public BTTypes<Profile, v1::BTSingleStream> {

    using Base = BTTypes<Profile, v1::BTSingleStream>;

    using Key   = Key_;

    using Entry = std::tuple<Key>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                v1::set::CtrInsertName,
                v1::set::CtrRemoveName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                v1::set::ItrNavName
    >;
};



template <
    typename Profile,
    typename Key,
    Int Special = 0
>
struct SetBTTypesBase: public SetBTTypesBaseBase<Profile, Key> {

    static_assert(
            IsExternalizable<Key>::Value ,
            "Key type must have either ValueCodec or FieldFactory defined"
    );

    using LeafKeyStruct = typename set::SetKeyStructTF<Key, HasFieldFactory<Key>::Value>::Type;

    using StreamDescriptors = TL<
            StreamTF<
                TL<
                    TL<StreamSize>,
                    TL<LeafKeyStruct>
                >,
                set::SetBranchStructTF
            >
    >;
};





template <
    typename Profile,
    typename Key_
>
struct BTTypes<Profile, v1::Set<Key_>>: public SetBTTypesBase<Profile, Key_>{};


template <typename Profile, typename Key, typename T>
class CtrTF<Profile, v1::Set<Key>, T>: public CtrTF<Profile, v1::BTSingleStream, T> {
};



}}
