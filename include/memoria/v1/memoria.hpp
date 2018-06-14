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
#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>

#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/strings/string.hpp>


namespace memoria {
namespace v1{

struct LibraryCtrs{
    static void init() {
        CtrApi<Set<FixedArray<16>>, DefaultProfile<>>::do_link();
        CtrApi<Map<U8String, U8String>, DefaultProfile<>>::do_link();
        CtrApi<EdgeMap, DefaultProfile<>>::do_link();
    }
};

}}
