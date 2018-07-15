
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



#include <memoria/v1/allocators/inmem/threads/thread_inmem_allocator_impl.hpp>

namespace memoria {
namespace v1 {

using Profile = DefaultProfile<>;    
    
namespace persistent_inmem {
    template class ThreadInMemAllocatorImpl<Profile>;
    template class ThreadSnapshot<Profile, ThreadInMemAllocatorImpl<Profile>>;
    
    template <typename PP>
    struct Initializer {
        Initializer() {
            ThreadSnapshot<PP, ThreadInMemAllocatorImpl<PP>>::initMetadata();
        }
    };
}

template class ThreadInMemAllocator<Profile>;
template class ThreadInMemSnapshot<Profile>;

namespace {

persistent_inmem::Initializer<Profile> init0;

}

}
}
