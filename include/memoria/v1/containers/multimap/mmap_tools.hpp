
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

#include <tuple>
#include <vector>

namespace memoria {
namespace v1 {
namespace mmap {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename KeyType, Int Selector = HasFieldFactory<KeyType>::Value> struct MMapKeyStructTF;

template <typename KeyType>
struct MMapKeyStructTF<KeyType, 1>: HasType<PkdFMTreeT<KeyType>> {};

template <typename KeyType>
struct MMapKeyStructTF<KeyType, 0>: HasType<PkdVBMTreeT<KeyType>> {};



template <typename ValueType, Int Selector = HasFieldFactory<ValueType>::Value> struct MMapValueStructTF;

template <typename ValueType>
struct MMapValueStructTF<ValueType, 1>: HasType<PkdFSQArrayT<ValueType>> {};

template <typename ValueType>
struct MMapValueStructTF<ValueType, 0>: HasType<PkdVDArrayT<ValueType>> {};


template <typename K, typename V>
using MapData = std::vector<std::pair<K, std::vector<V>>>;


}
}}
