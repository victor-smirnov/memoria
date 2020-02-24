
// Copyright 2017 Victor Smirnov
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

#include "allocation_map_factory.hpp"

#include <memoria/api/allocation_map/allocation_map_api.hpp>
#include <memoria/core/container/ctr_impl_btss.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memory>

namespace memoria {

template <typename Profile>
void ICtrApi<AllocationMap, Profile>::init_profile_metadata()
{
    SharedCtr<AllocationMap, ProfileAllocatorType<Profile>, Profile>::init_profile_metadata();
}

}
