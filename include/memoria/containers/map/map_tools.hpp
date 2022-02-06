
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

#include <memoria/core/packed/datatypes/varchar.hpp>

namespace memoria {
namespace map {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename DataType>
struct MapKeyStructTF: HasType<PackedDataTypeBufferT<DataType, true>> {};

template <typename DataType>
struct MapValueStructTF: HasType<PackedDataTypeBufferT<DataType, false>> {};


template <typename T> struct MapBranchStructTF;

template <typename KeyType>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};

template <typename KeyType>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};

template <typename KeyType, int32_t Indexes>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>>
{
    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    //FIXME: Extend KeyType to contain enough space to represent practically large sums
    //Should be done systematically on the level of BT

    using Type = PkdFQTreeT<KeyType, Indexes>;

//    using Type = IfThenElse <
//            DTTIs1DFixedSize<KeyType>,
//            PkdFQTreeT<KeyType, Indexes>,
//            PkdVQTreeT<KeyType, Indexes>
//    >;

    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, int32_t Indexes>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {
    static_assert(Indexes <= 1, "");

    using Type = PackedDataTypeBufferT<KeyType, Indexes == 1>;

    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};





template <typename Key, typename Value, typename CtrSizeT = int64_t>
class KeyValueEntry {
    const Key& key_;
    const Value& value_;

    CtrSizeT zero_ = 0;

public:
    KeyValueEntry(const Key& key, const Value& value): key_(key), value_(value) {}

    const auto& get(StreamTag<0>, StreamTag<0>, int32_t) const {
        return zero_;
    }

    const auto& get(StreamTag<0>, StreamTag<1>, int32_t) const {
        return key_;
    }

    const auto& get(StreamTag<0>, StreamTag<2>, int32_t) const {
        return value_;
    }
};

template <typename T, int32_t SubstreamIdx = 0>
class ValueBuffer {
    const T& value_;
public:
    ValueBuffer(const T& value): value_(value) {}

    const auto& get(StreamTag<0>, StreamTag<SubstreamIdx>, int32_t) const {
        return value_;
    }
};


template <typename T>
struct MapValueHelper {
    template <typename TT>
    static T convert(TT&& value) {
        return value;
    }
};

template <typename T>
struct MapValueHelper<StaticVector<T, 1>> {
    template <typename TT>
    static T convert(TT&& value) {
        return value[0];
    }
};




}}
