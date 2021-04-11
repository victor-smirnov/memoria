// Copyright 2011-2021 Victor Smirnov
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

#include <memoria/api/set/set_api.hpp>
#include <memoria/api/map/map_api.hpp>
#include <memoria/api/vector/vector_api.hpp>
#include <memoria/api/multimap/multimap_api.hpp>
#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <memoria/core/tools/fixed_array.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/uuid.hpp>

namespace memoria {

void InitCtrDatatypes();
void InitMemoriaCtrsExplicit();

template <typename T, typename ProfileT>
void InitCtrMetadata() {
    ICtrApi<T, ApiProfile<ProfileT>>::template init_profile_metadata<ProfileT>();
}

template <typename ProfileT>
struct StaticLibraryCtrs {
    static void init()
    {
        InitCtrMetadata<Set<FixedArray<16>>, ProfileT>();
        InitCtrMetadata<Set<Varchar>, ProfileT>();
        InitCtrMetadata<Set<UUID>, ProfileT>();
        InitCtrMetadata<Vector<Varchar>, ProfileT>();
        InitCtrMetadata<Vector<UTinyInt>, ProfileT>();
        InitCtrMetadata<Map<Varchar, Varchar>, ProfileT>();

        InitCtrMetadata<Vector<LinkedData>, ProfileT>();
        InitCtrMetadata<Map<BigInt, Varchar>, ProfileT>();
        InitCtrMetadata<Map<BigInt, BigInt>, ProfileT>();
        InitCtrMetadata<Map<UUID, UUID>, ProfileT>();

        InitCtrMetadata<Multimap<BigInt, UTinyInt>, ProfileT>();
        InitCtrMetadata<Multimap<UUID, UTinyInt>, ProfileT>();

        InitCtrMetadata<Multimap<Varchar, Varchar>, ProfileT>();

        InitCtrMetadata<AllocationMap, ProfileT>();
    }
};

}
