
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

#include <memoria/v1/containers/multimap/mmap_tools.hpp>

//#include <memoria/v1/retired/prototypes/bt_fl/io/btfl_input.hpp>

#include <tuple>
#include <vector>

namespace memoria {
namespace v1 {
namespace edge_map {

template <typename K, typename V>
using MapData = std::vector<std::pair<K, std::vector<V>>>;





}
}}
