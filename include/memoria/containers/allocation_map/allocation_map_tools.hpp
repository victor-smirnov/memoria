
// Copyright 2020 Victor Smirnov
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
namespace alcmap {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename T> struct MapBranchStructTF;

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
//            DTTIsNDFixedSize<KeyType>,
//            PkdFQTreeT<KeyType, Indexes>,
//            PkdVQTreeT<KeyType, Indexes>
//    >;

    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};










}}
