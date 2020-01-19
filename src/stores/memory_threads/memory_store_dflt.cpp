
// Copyright 2019 Victor Smirnov
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

#include <memoria/profiles/default/default.hpp>

#include <memoria/store/memory/threads/threads_memory_store_impl.hpp>

namespace memoria {

using Profile = DefaultProfile<>;

template class IMemoryStore<Profile>;

namespace store {
namespace memory {

template class ThreadsMemoryStoreImpl<Profile>;
template class ThreadsSnapshot<Profile, ThreadsMemoryStoreImpl<Profile>>;


template <typename PP>
struct Initializer {
    Initializer() {
        ThreadsSnapshot<PP, ThreadsMemoryStoreImpl<PP>>::init_profile_metadata();
    }
};

}}

void InitDefaultInMemStore() {
    store::memory::Initializer<Profile> init0;
}

}
