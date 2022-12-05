
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/hermes/map/map_ext.hpp>
#include <memoria/core/hermes/container_ext.hpp>

namespace memoria {
namespace hermes {

using GMPoolT = pool::SimpleObjectPool<TypedGenericMap<Varchar, Object>>;
using GMPoolPtrT = boost::local_shared_ptr<GMPoolT>;

using GMEntryPoolT = pool::SimpleObjectPool<TypedGenericMapEntry<Varchar, Object>>;
using GMEntryPoolPtrT = boost::local_shared_ptr<GMEntryPoolT>;


PoolSharedPtr<GenericMap> TypedGenericMap<Varchar, Object>::make_wrapper(void* array, ViewPtrHolder* ctr_holder) {
    static thread_local GMPoolPtrT wrapper_pool = MakeShared<GMPoolT>();
    return wrapper_pool->allocate_shared(array, ctr_holder);
}

PoolSharedPtr<GenericMapEntry> TypedGenericMap<Varchar, Object>::iterator() const
{
    static thread_local GMEntryPoolPtrT entry_pool = MakeShared<GMEntryPoolT>();
    return entry_pool->allocate_shared(map_.begin(), map_.end());
}


}}
