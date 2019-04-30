
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

#include <memoria/v1/core/packed/tree/fse_max/packed_fse_max_tree.hpp>
#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/array/packed_fse_array.hpp>

#include <memoria/v1/core/integer/uacc_field_factory.hpp>

#include <tuple>
#include <vector>

namespace memoria {
namespace v1 {
namespace update_log {



using bt::IdxSearchType;
using bt::StreamTag;

template <typename KeyType, int32_t Selector = HasFieldFactory<KeyType>::Value> struct SnapshotIdStructTF;
template <typename KeyType, int32_t Selector = HasFieldFactory<KeyType>::Value> struct CtrNameStructTF;
template <typename KeyType, int32_t Selector = HasFieldFactory<KeyType>::Value> struct CommandDataStructTF;

template <typename T> struct UpdateLogBranchStructTF;

template <typename ElementType>
struct SnapshotIdStructTF<ElementType, 1>: HasType<PkdFSQArrayT<ElementType>> {};

template <typename CtrNameType>
struct CtrNameStructTF<CtrNameType, 1>: HasType<PkdFQTreeT<UAcc192T, 1, CtrNameType>> {};

template <typename DataType>
struct CommandDataStructTF<DataType, 1>: HasType<PkdFSQArrayT<DataType>> {};



template <typename KeyType>
struct UpdateLogBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};


template <typename KeyType>
struct UpdateLogBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};


template <typename KeyType, int32_t Indexes>
struct UpdateLogBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>>
{
    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    //FIXME: Extend KeyType to contain enough space to represent practically large sums
    //Should be done systematically on the level of BT

    using Type = PkdFQTreeT<KeyType, Indexes>;

    static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, int32_t Indexes>
struct UpdateLogBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    using Type = PkdFMOTreeT<KeyType, Indexes>;;

    static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};


template <int32_t Stream, typename T, typename CtrSizeT>
struct SingleValueUpdateEntryFn {

    const T& value_;

    SingleValueUpdateEntryFn(const T& value): value_(value) {}

    const auto& get(const StreamTag<Stream>& , const StreamTag<0>&, int32_t block) const
    {
        return value_;
    }
};


}
}}
