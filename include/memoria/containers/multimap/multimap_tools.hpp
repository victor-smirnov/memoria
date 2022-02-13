
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/tools/ticker.hpp>

#include <memoria/core/strings/string_codec.hpp>

#include <memoria/core/packed/packed.hpp>

#include <tuple>
#include <vector>

namespace memoria {
namespace multimap {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename DataType>
struct MMapKeyStructTF: HasType<PackedDataTypeBufferT<DataType, true, 1, DTOrdering::MAX>> {};

template <typename KeyType, bool Selector = DTTIsNDFixedSize<KeyType>> struct MMapSumKeyStructTF;

template <typename T> struct MMapBranchStructTF;


template <typename KeyType>
struct MMapSumKeyStructTF<KeyType, true>: HasType<PkdFQTreeT<KeyType>> {};

template <typename DataType>
struct MMapValueStructTF: HasType<PackedDataTypeBufferT<DataType, false, 1, DTOrdering::UNORDERED>> {};


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
    using Type = PkdFQTreeT<KeyType, Indexes>;

    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, int32_t Indexes>
struct MMapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

    using Type = PackedDataTypeOptBufferT<KeyType, Indexes == 1, 1, DTOrdering::MAX>;
//    bt::PkdStructSelector<
//            DTTIsNDFixedSize<KeyType>,
//            PackedFixedElementOptArray,
//            PackedVLenElementOptArray,

//            PackedFixedElementOptArrayTypes<KeyType, Indexes, Indexes>,
//            PackedVLenElementArrayTypes<KeyType, Indexes, Indexes>
//    >;

    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};


}}
