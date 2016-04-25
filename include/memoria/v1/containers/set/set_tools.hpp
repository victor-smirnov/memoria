
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

#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/fse_max/packed_fse_max_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/v1/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>
#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/array/packed_vle_dense_array.hpp>
#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_input.hpp>

namespace memoria {
namespace v1 {
namespace set {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename KeyType, Int Selector> struct SetKeyStructTF;

template <typename KeyType>
struct SetKeyStructTF<KeyType, 1>: HasType<PkdFMTreeT<KeyType>> {};

template <typename KeyType>
struct SetKeyStructTF<KeyType, 0>: HasType<PkdVBMTreeT<KeyType>> {};




template <typename T> struct SetBranchStructTF;

template <typename KeyType>
struct SetBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};

template <typename KeyType>
struct SetBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};

template <typename KeyType, Int Indexes>
struct SetBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>>
{
    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    //FIXME: Extend KeyType to contain enough space to represent practically large sums
    //Should be done systematically on the level of BT

    using Type = IfThenElse <
            HasFieldFactory<KeyType>::Value,
            PkdFQTreeT<KeyType, Indexes>,
            PkdVQTreeT<KeyType, Indexes>
    >;

    static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, Int Indexes>
struct SetBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    using Type = IfThenElse<
            HasFieldFactory<KeyType>::Value,
            PkdFMTreeT<KeyType, Indexes>,
            PkdVBMTreeT<KeyType>
    >;

    static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};





template <typename Key, typename CtrSizeT = BigInt>
class KeyEntry {
    const Key& key_;

public:
    KeyEntry(const Key& key): key_(key) {}

    auto get(StreamTag<0>, StreamTag<0>, Int) const {
        return 0;
    }

    auto get(StreamTag<0>, StreamTag<1>, Int) const {
        return key_;
    }
};








}
}}