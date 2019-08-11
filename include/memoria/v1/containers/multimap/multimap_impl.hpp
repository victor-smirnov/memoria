
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

#include "mmap_factory.hpp"

#include <memoria/v1/api/multimap/multimap_api.hpp>
#include <memoria/v1/api/multimap/multimap_input.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/container/ctr_impl_btfl.hpp>


#include <memory>

namespace memoria {
namespace v1 {

template <typename Key, typename Value, typename Profile>
void ICtrApi<Multimap<Key, Value>, Profile>::init_profile_metadata()
{
    SharedCtr<Multimap<Key, Value>, ProfileAllocatorType<Profile>, Profile>::init_profile_metadata();
}

template <typename Key, typename Value, typename Profile>
std::shared_ptr<io::IOVector> ICtrApi<Multimap<Key, Value>, Profile>::create_iovector()
{
    return SharedCtr<Multimap<Key, Value>, ProfileAllocatorType<Profile>, Profile>::create_iovector();
}

}}
