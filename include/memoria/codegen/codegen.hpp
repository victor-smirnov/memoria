
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/api/allocation_map/allocation_map_api.hpp>
#include <memoria/api/map/map_api.hpp>
#include <memoria/api/set/set_api.hpp>
#include <memoria/api/vector/vector_api.hpp>
#include <memoria/api/multimap/multimap_api.hpp>

#include <memoria/core/tools/assert.hpp>

#define MMA_ANN(X) [[clang::annotate(#X)]]

namespace memoria {

template <typename CtrNameTemplates>
struct TypeFactory {
    static constexpr uint64_t ID = 0ull;
};

template <typename InstanceConfig>
class TypeInstance {};

template <typename Discriminator = void>
class CodegenConfig {};

template <typename Key, typename Value>
struct [[clang::annotate(R"(
@CtrTF = {
    "name": "map_tf",
    "type": "BTSS",
    "includes": [
        "memoria/containers/map/map_factory.hpp",
        "memoria/containers/map/map_impl.hpp"
    ],
    "generator": "codegen.ctr_typefactory.CtrTypeFactory"
})")]]
TypeFactory<Map<Key, Value>> {
    static constexpr uint64_t ID = 12734987298625893454ull;
};


template <typename Key>
struct [[clang::annotate(R"(
    @CtrTF = {
        "name": "set_tf",
        "type": "BTSS",
        "includes": [
            "memoria/containers/set/set_factory.hpp",
            "memoria/containers/set/set_api_impl.hpp"
        ],
        "generator": "codegen.ctr_typefactory.CtrTypeFactory"
    }
)")]]
TypeFactory<Set<Key>> {
    static constexpr uint64_t ID = 5936230912039530945ull;
};


template <typename Value>
struct [[clang::annotate(R"(
    @CtrTF = {
        "name": "vector_tf",
        "type": "BTSS",
        "includes": [
            "memoria/containers/vector/vctr_factory.hpp",
            "memoria/containers/vector/vector_api_impl.hpp"
        ],
        "generator": "codegen.ctr_typefactory.CtrTypeFactory"
    }
)")]]
TypeFactory<Vector<Value>> {
    static constexpr uint64_t ID = 4947672392732876033ull;
};


template <typename Key, typename Value>
struct [[clang::annotate(R"(
    @CtrTF = {
        "name": "multimap_tf",
        "type": "BTFL",
        "includes": [
            "memoria/containers/multimap/multimap_factory.hpp",
            "memoria/containers/multimap/multimap_impl.hpp"
        ],
        "generator": "codegen.ctr_typefactory.CtrTypeFactory"
    }
)")]]
TypeFactory<Multimap<Key, Value>> {
    static constexpr uint64_t ID = 4773459834585883945ull;
};


template <>
struct [[clang::annotate(R"(
    @CtrTF = {
        "name": "allocation_map_tf",
        "type": "BTSS",
        "includes": [
            "memoria/containers/allocation_map/allocation_map_factory.hpp",
            "memoria/containers/allocation_map/allocation_map_api_impl.hpp",
            "memoria/containers/allocation_map/allocation_map_impl.hpp"
        ],
        "generator": "codegen.ctr_typefactory.CtrTypeFactory"
    }
)")]]
TypeFactory<AllocationMap> {
    static constexpr uint64_t ID = 9503245888843438934ull;
};

}
