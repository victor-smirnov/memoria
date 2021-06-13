
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

#include "multimap_factory.hpp"

#include <memoria/api/multimap/multimap_api.hpp>
#include <memoria/api/multimap/multimap_input.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/container/ctr_impl_btfl.hpp>


#include <memory>

namespace memoria {

template <typename Key, typename Value, typename Profile>
template <typename ImplProfile>
void ICtrApi<Multimap<Key, Value>, Profile>::init_profile_metadata()
{
    SharedCtr<
            Multimap<Key, Value>,
            ProfileStoreType<ImplProfile>,
            ImplProfile
    >::init_profile_metadata();
}

}
