
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/tools/ticker.hpp>

#include <memoria/v1/core/strings/string_codec.hpp>

#include <memoria/v1/core/packed/packed.hpp>

#include <tuple>
#include <vector>

namespace memoria {
namespace v1 {
namespace multimap {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename KeyType, bool Selector = DataTypeTraits<KeyType>::isFixedSize> struct MMapKeyStructTF;
template <typename KeyType, bool Selector = DataTypeTraits<KeyType>::isFixedSize> struct MMapSumKeyStructTF;

template <typename T> struct MMapBranchStructTF;

template <typename KeyType>
struct MMapKeyStructTF<KeyType, true>: HasType<PkdFSEArray2T<KeyType, 1, 1>> {};

template <typename KeyType>
struct MMapKeyStructTF<KeyType, false>: HasType<PkdVLEArrayT<KeyType, 1, 1>> {};


template <typename KeyType>
struct MMapSumKeyStructTF<KeyType, true>: HasType<PkdFQTreeT<KeyType>> {};

template <typename KeyType>
struct MMapSumKeyStructTF<KeyType, false>: HasType<PkdVQTreeT<KeyType>> {};


template <typename ValueType, bool Selector = DataTypeTraits<ValueType>::isFixedSize> struct MMapValueStructTF;

template <typename ValueType>
struct MMapValueStructTF<ValueType, true>: HasType<PkdFSEArray2T<ValueType>> {};

template <typename ValueType>
struct MMapValueStructTF<ValueType, false>: HasType<PkdVLEArrayT<ValueType>> {};



template <typename KeyType>
struct MMapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = FailIf<false, PackedEmptyStruct<KeyType, PkdSearchType::MAX>>;
};


template <typename KeyType>
struct MMapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};

template <typename KeyType, int32_t Indexes>
struct MMapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>>
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

    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, int32_t Indexes>
struct MMapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

    using Type = bt::PkdStructSelector<
            DataTypeTraits<KeyType>::isFixedSize,
            PkdFMOTree,
            PackedVLenElementOptArray,

            PkdFMTreeTypes<KeyType, Indexes>,
            PackedVLenElementArrayTypes<KeyType, Indexes, Indexes>
    >;

    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};


}
}}
