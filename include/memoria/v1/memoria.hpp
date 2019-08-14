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

#include <memoria/v1/api/allocator/allocator_inmem_api.hpp>
#include <memoria/v1/api/set/set_api.hpp>
#include <memoria/v1/api/map/map_api.hpp>
#include <memoria/v1/api/multimap/multimap_api.hpp>
#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>
#include <memoria/v1/api/db/update_log/update_log_api.hpp>

#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/tools/uuid.hpp>


namespace memoria {
namespace v1{

template <typename ProfileT = DefaultProfile<>>
struct StaticLibraryCtrs {
    static void init()
    {
//        CtrApi<Set<FixedArray<16>>, ProfileT>::do_link();
        ICtrApi<Map<Varchar, Varchar>, ProfileT>::init_profile_metadata();
        ICtrApi<Map<BigInt, Varchar>, ProfileT>::init_profile_metadata();
        ICtrApi<Map<BigInt, BigInt>, ProfileT>::init_profile_metadata();
        ICtrApi<Multimap<BigInt, UTinyInt>, ProfileT>::init_profile_metadata();
        ICtrApi<Multimap<Varchar, Varchar>, ProfileT>::init_profile_metadata();

//        CtrApi<Multimap<int64_t, uint8_t>, ProfileT>::do_link();
//        CtrApi<Multimap<UUID, uint8_t>, ProfileT>::do_link();
//        CtrApi<EdgeMap, ProfileT>::do_link();
//        CtrApi<UpdateLog, ProfileT>::do_link();
    }
};

}}
