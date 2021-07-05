
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

#include <memoria/codegen/codegen.hpp>

#include <memoria/profiles/impl/cow_lite_profile.hpp>
#include <memoria/profiles/impl/cow_profile.hpp>
#include <memoria/profiles/impl/no_cow_profile.hpp>

#include <memoria/core/tools/fixed_array.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <memoria/containers/multimap/multimap_impl.hpp>

namespace memoria {

template<>
class [[clang::annotate(R"(
    @CodegenConfig = {
        "resources": {
            "container": @TypeInstance = {
                "target_folder": "src/containers"
            },
            "container_factory": @TypeFactory = {
                "target_folder": "src/containers"
            },
            "datatypes_init": @FileSink = {
                "filename": "src/contianers/datatypes.cpp",
                "handler": "DatatypesInitSink"
            }
        },
        "profiles": ["CowProfile<>", "NoCowProfile<>", "CowLiteProfile<>"],
        "script": "codegen/python/codegen.py"
    }
)")]] CodegenConfig<> {};



template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "fixed_array_set_ctr",
        "includes": [
            "memoria/api/set/set_api.hpp"
        ]
    }
)")]] TypeInstance<Set<FixedArray<16>>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "varchar_set_ctr",
        "includes": [
            "memoria/api/set/set_api.hpp"
        ]
    }
)")]] TypeInstance<Set<Varchar>>;

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "uuid_set_ctr",
        "includes": [
            "memoria/api/set/set_api.hpp"
        ]
    }
)")]] TypeInstance<Set<UUID>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "varchar_vector_ctr",
        "includes": [
            "memoria/api/vector/vector_api.hpp"
        ]
    }
)")]] TypeInstance<Vector<Varchar>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "uint8_vector_ctr",
        "includes": [
            "memoria/api/vector/vector_api.hpp"
        ]
    }
)")]] TypeInstance<Vector<UTinyInt>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "lddoc_vector_ctr",
        "includes": [
            "memoria/api/vector/vector_api.hpp"
        ]
    }
)")]] TypeInstance<Vector<LinkedData>> {};


template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "varchar_varchar_map_ctr",
        "includes": [
            "memoria/api/map/map_api.hpp"
        ]
    }
)")]] TypeInstance<Map<Varchar, Varchar>> {};


template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "int64_varchar_map_ctr",
        "includes": [
            "memoria/api/map/map_api.hpp"
        ]
    }
)")]] TypeInstance<Map<BigInt, Varchar>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "int64_int64_map_ctr",
        "includes": [
            "memoria/api/map/map_api.hpp"
        ]
    }
)")]] TypeInstance<Map<BigInt, BigInt>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "uuid_uuid_map_ctr",
        "includes": [
            "memoria/api/map/map_api.hpp"
        ]
    }
)")]] TypeInstance<Map<UUID, UUID>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "uuid_uint8_map_ctr",
        "includes": [
            "memoria/api/map/map_api.hpp"
        ]
    }
)")]] TypeInstance<Map<UUID, UBigInt>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "int64_uint8_multimap_ctr",
        "includes": [
            "memoria/api/multimap/multimap_api.hpp"
        ]
    }
)")]] TypeInstance<Multimap<BigInt, UTinyInt>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "uuid_uint8_multimap_ctr",
        "includes": [
            "memoria/api/multimap/multimap_api.hpp"
        ]
    }
)")]] TypeInstance<Multimap<UUID, UTinyInt>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "varchar_varchar_multimap_ctr",
        "includes": [
            "memoria/api/multimap/multimap_api.hpp"
        ]
    }
)")]] TypeInstance<Multimap<Varchar, Varchar>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "name": "allocation_map_ctr",
        "includes": [
            "memoria/api/allocation_map/allocation_map_api.hpp"
        ]
    }
)")]] TypeInstance<AllocationMap> {};

}
