
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

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/core/packed/packed.hpp>

namespace memoria {
namespace set {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename DataType>
struct SetKeyStructTF: HasType<PackedDataTypeBufferT<DataType, true, 1, DTOrdering::MAX>> {};


template <typename T> struct SetBranchStructTF;

template <typename KeyType>
struct SetBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};

template <typename KeyType>
struct SetBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};

template <typename KeyType, int32_t Indexes>
struct SetBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>>
{
    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    //FIXME: Extend KeyType to contain enough space to represent practically large sums
    //Should be done systematically on the level of BT

    using Type = PackedDataTypeBufferT<KeyType, true, Indexes, DTOrdering::SUM>;
    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, int32_t Indexes>
struct SetBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

    static_assert(Indexes <= 1, "");

    using Type = PackedDataTypeBufferT<KeyType, Indexes == 1, 1, DTOrdering::MAX>;
    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};





template <typename Key, typename CtrSizeT = int64_t>
class KeyEntry {
    const Key& key_;
    CtrSizeT zero_ = 0;
public:
    KeyEntry(const Key& key): key_(key) {}

    const auto& get(StreamTag<0>, StreamTag<0>, int32_t) const {
        return zero_;
    }

    const auto& get(StreamTag<0>, StreamTag<1>, int32_t) const {
        return key_;
    }
};








}}
