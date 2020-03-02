// Copyright 2011 Victor Smirnov
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

#include <memoria/api/common/ctr_api.hpp>

#ifdef MEMORIA_BUILD_CONTAINERS
#   include <memoria/api/set/set_api.hpp>
#   include <memoria/api/map/map_api.hpp>
#   include <memoria/api/vector/vector_api.hpp>
#   include <memoria/api/multimap/multimap_api.hpp>
#   include <memoria/api/allocation_map/allocation_map_api.hpp>
#else
#   if defined (MEMORIA_BUILD_CONTAINERS_MULTIMAP)
#       include <memoria/api/multimap/multimap_api.hpp>
#   endif
#   if defined (MEMORIA_BUILD_CONTAINERS_SET)
#       include <memoria/api/set/set_api.hpp>
#       include <memoria/api/allocation_map/allocation_map_api.hpp>
#   endif
#   if defined (MEMORIA_BUILD_CONTAINERS_VECTOR)
#       include <memoria/api/vector/vector_api.hpp>
#   endif
#endif

#include <memoria/core/tools/fixed_array.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/uuid.hpp>


namespace memoria {

void InitCoreLDDatatypes();
void InitCoreDatatypes();
void InitSimpleNumericDatatypes();
void InitCtrDatatypes();

#ifdef MEMORIA_BUILD_MEMORY_STORE
void InitDefaultInMemStore();
#endif

#if defined(MEMORIA_BUILD_MEMORY_STORE_COW)
void InitCoWInMemStore();
#endif

#if defined(MEMORIA_BUILD_SWMR_STORE_MAPPED)
void InitSWMRMappedStore();
#endif

void InitMemoriaExplicit();

template <typename T, typename ProfileT>
void InitCtrMetadata() {
    ICtrApi<T, ProfileT>::init_profile_metadata();
}

template <typename ProfileT = DefaultProfile<>>
struct StaticLibraryCtrs {
    static void init()
    {
#ifdef MEMORIA_BUILD_CONTAINERS
        InitCtrMetadata<Set<FixedArray<16>>, ProfileT>();
        InitCtrMetadata<Set<Varchar>, ProfileT>();
        InitCtrMetadata<Set<UUID>, ProfileT>();
        InitCtrMetadata<Vector<Varchar>, ProfileT>();
        InitCtrMetadata<Vector<UTinyInt>, ProfileT>();
        InitCtrMetadata<Map<Varchar, Varchar>, ProfileT>();

        InitCtrMetadata<Vector<LinkedData>, ProfileT>();
        InitCtrMetadata<Map<BigInt, Varchar>, ProfileT>();
        InitCtrMetadata<Map<BigInt, BigInt>, ProfileT>();

        InitCtrMetadata<Multimap<BigInt, UTinyInt>, ProfileT>();
        InitCtrMetadata<Multimap<UUID, UTinyInt>, ProfileT>();

        InitCtrMetadata<Multimap<Varchar, Varchar>, ProfileT>();

        InitCtrMetadata<AllocationMap, ProfileT>();
#else
#   if defined(MEMORIA_BUILD_CONTAINERS_MULTIMAP)
        InitCtrMetadata<Multimap<Varchar, Varchar>, ProfileT>();
        InitCtrMetadata<Multimap<UUID, UTinyInt>, ProfileT>();
#   endif

#   if defined(MEMORIA_BUILD_CONTAINERS_SET)
        InitCtrMetadata<Set<Varchar>, ProfileT>();
        InitCtrMetadata<Set<UUID>, ProfileT>();
        InitCtrMetadata<AllocationMap, ProfileT>();
#   endif

#   if defined(MEMORIA_BUILD_CONTAINERS_VECTOR)
        InitCtrMetadata<Vector<Varchar>, ProfileT>();
        InitCtrMetadata<Vector<UTinyInt>, ProfileT>();
#   endif

#endif
    }
};

}
