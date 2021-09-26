
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
#include <memoria/core/tools/uid_64.hpp>
#include <memoria/core/tools/uid_256.hpp>

#include <memoria/store/swmr/common/swmr_store_datatypes.hpp>

namespace memoria {

template<>
class [[clang::annotate(R"(
    @CodegenConfig = {
        "groups": {
            "default": {
                "datatypes": @FileGenerator = {
                    "filename": "src/containers/generated/ctr_datatypes.cpp",
                    "handler": "codegen.datatype_init.DatatypeInitGenerator"
                },
                "containers": @TypeInstance = {
                    "path": "src/containers/generated",
                    "init": @FileGenerator = {
                        "includes": [
                            "memoria/codegen/codegen_ctrinit.hpp"
                        ],
                        "filename": "src/containers/generated/ctr_init.cpp",
                        "handler": "codegen.ctr_init.CtrInitGenerator",
                        "function": "init_core_containers"
                    }
                }
            },

            "stores": {
                "datatypes": @FileGenerator = {
                    "filename": "src/stores/generated/ctr_datatypes.cpp",
                    "handler": "codegen.datatype_init.DatatypeInitGenerator"
                },
                "containers": @TypeInstance = {
                    "path": "src/stores/generated",
                    "init": @FileGenerator = {
                        "includes": [
                            "memoria/codegen/codegen_ctrinit.hpp"
                        ],
                        "filename": "src/stores/generated/ctr_init.cpp",
                        "handler": "codegen.ctr_init.CtrInitGenerator",
                        "function": "init_store_containers"
                    }
                }
            }
        },
        "profiles": {
            "CowProfile<>": {
                "includes": [
                    "memoria/profiles/impl/cow_profile.hpp"
                ]
            },
            "NoCowProfile<>": {
                "enabled": false,
                "includes": [
                    "memoria/profiles/impl/no_cow_profile.hpp"
                ]
            },
            "CowLiteProfile<>": {
                "includes": [
                    "memoria/profiles/impl/cow_lite_profile.hpp"
                ]
            }
        }
    }
)")]] CodegenConfig<> {};

#ifdef MEMORIA_BUILD_EXTRA_CONTAINERS

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/set/set_api.hpp",
            "memoria/core/tools/fixed_array.hpp"
        ]
    }
)")]] TypeInstance<Set<FixedArray<16>>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/set/set_api.hpp"
        ]
    }
)")]] TypeInstance<Set<Varchar>>;

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/set/set_api.hpp",
            "memoria/core/tools/uuid.hpp"
        ]
    }
)")]] TypeInstance<Set<UUID>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/vector/vector_api.hpp"
        ]
    }
)")]] TypeInstance<Vector<Varchar>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/vector/vector_api.hpp"
        ]
    }
)")]] TypeInstance<Vector<UTinyInt>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/vector/vector_api.hpp"
        ]
    }
)")]] TypeInstance<Vector<LinkedData>> {};


template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/map/map_api.hpp"
        ]
    }
)")]] TypeInstance<Map<Varchar, Varchar>> {};


template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/map/map_api.hpp"
        ]
    }
)")]] TypeInstance<Map<BigInt, Varchar>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/multimap/multimap_api.hpp"
        ]
    }
)")]] TypeInstance<Multimap<Varchar, Varchar>> {};


#endif

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/map/map_api.hpp"
        ]
    }
)")]] TypeInstance<Map<BigInt, BigInt>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/map/map_api.hpp",
            "memoria/core/tools/uid_256.hpp"
        ]
    }
)")]] TypeInstance<Map<UID256, UID256>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/map/map_api.hpp",
            "memoria/core/tools/uid_256.hpp",
            "memoria/core/tools/uid_64.hpp"
        ]
    }
)")]] TypeInstance<Map<UID256, UID64>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/multimap/multimap_api.hpp"
        ]
    }
)")]] TypeInstance<Multimap<BigInt, UTinyInt>> {};

template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "profiles": "ALL",
        "includes": [
            "memoria/api/multimap/multimap_api.hpp",
            "memoria/core/tools/uuid.hpp"
        ]
    }
)")]] TypeInstance<Multimap<UUID, UTinyInt>> {};


template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "config": "$/groups/stores/containers",
        "profiles": ["CowProfile<>", "CowLiteProfile<>"],
        "includes": [
            "memoria/api/allocation_map/allocation_map_api.hpp"
        ]
    }
)")]] TypeInstance<AllocationMap> {};


template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "config": "$/groups/stores/containers",
        "profiles": ["CowLiteProfile<>"],
        "includes": [
            "memoria/api/map/map_api.hpp",
            "memoria/core/container/cow.hpp",
            "memoria/core/tools/uid_64.hpp",
            "memoria/core/tools/uid_256.hpp"
        ]
    }
)")]] TypeInstance<Map<UID256, CowBlockID<UID64>>> {};


template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "config": "$/groups/stores/containers",
        "profiles": ["CowProfile<>"],
        "includes": [
            "memoria/api/map/map_api.hpp",
            "memoria/core/container/cow.hpp",
            "memoria/core/tools/uid_256.hpp"
        ]
    }
)")]] TypeInstance<Map<UID256, CowBlockID<UID256>>> {};


template<>
class [[clang::annotate(R"(
    @TypeInstance = {
        "config": "$/groups/stores/containers",
        "profiles": ["CowProfile<>", "CowLiteProfile<>"],
        "includes": [
            "memoria/api/map/map_api.hpp",
            "memoria/store/swmr/common/swmr_store_datatypes.hpp",
            "memoria/core/tools/uid_256.hpp"
        ]
    }
)")]] TypeInstance<Map<UID256, SnapshotMetadataDT<DataTypeFromProfile<ApiProfile<CowProfile<>>>>>> {};

}
