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

#include <memoria/v1/api/set/set_api.hpp>
#include <memoria/v1/api/map/map_api.hpp>
#include <memoria/v1/api/vector/vector_api.hpp>
#include <memoria/v1/api/multimap/multimap_api.hpp>

//#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>
//#include <memoria/v1/api/db/update_log/update_log_api.hpp>

#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/tools/uuid.hpp>


namespace memoria {
namespace v1{

void InitCoreLDDatatypes();
void InitCoreDatatypes();
void InitSimpleNumericDatatypes();
void InitCtrDatatypes();
void InitDefaultInMemStore();

void InitMemoriaExplicit();

template <typename T, typename ProfileT>
void InitCtrMetadata() {
    ICtrApi<T, ProfileT>::init_profile_metadata();
}

template <typename ProfileT = DefaultProfile<>>
struct StaticLibraryCtrs {
    static void init()
    {
        InitCtrMetadata<Set<FixedArray<16>>, ProfileT>();
        InitCtrMetadata<Set<Varchar>, ProfileT>();
        InitCtrMetadata<Vector<Varchar>, ProfileT>();
        InitCtrMetadata<Vector<UTinyInt>, ProfileT>();
        InitCtrMetadata<Map<Varchar, Varchar>, ProfileT>();

        InitCtrMetadata<Vector<LinkedData>, ProfileT>();
        InitCtrMetadata<Map<BigInt, Varchar>, ProfileT>();
        InitCtrMetadata<Map<BigInt, BigInt>, ProfileT>();
        //InitCtrMetadata<Multimap<BigInt, UTinyInt>, ProfileT>();
        //InitCtrMetadata<Multimap<UUID, UTinyInt>, ProfileT>();
        InitCtrMetadata<Multimap<Varchar, Varchar>, ProfileT>();
    }
};

}}
