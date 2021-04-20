
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

#include <memoria/profiles/impl/no_cow_profile.hpp>

#include <memoria/store/memory_nocow/fibers/fibers_memory_store_impl.hpp>

namespace memoria {

using Profile = NoCowProfile<>;
using ApiProfileT = ApiProfile<Profile>;

template class IMemoryStore<ApiProfile<Profile>>;


SharedPtr<IMemoryStore<ApiProfileT>> create_memory_store_noncow()
{ 
    MaybeError maybe_error;

    auto snp = MakeShared<store::memory_nocow::FibersMemoryStoreImpl<Profile>>(maybe_error);

    if (!maybe_error) {
        return std::move(snp);
    }
    else {
        std::move(maybe_error.get()).do_throw();
    }
}

SharedPtr<IMemoryStore<ApiProfileT>> load_memory_store_noncow(U8String path) {
    auto fileh = FileInputStreamHandler::create(path.data());
    return store::memory_nocow::FibersMemoryStoreImpl<Profile>::load(fileh.get());
}

AllocSharedPtr<IMemoryStore<ApiProfileT>> load_memory_store_noncow(InputStreamHandler* input_stream)
{
    return store::memory_nocow::FibersMemoryStoreImpl<Profile>::load(input_stream);
}

namespace store {
namespace memory_nocow {
    
template class FibersMemoryStoreImpl<Profile>;
template class FibersSnapshot<Profile, FibersMemoryStoreImpl<Profile>>;

template <typename PP>
struct Initializer {
    Initializer() {
        FibersSnapshot<PP, FibersMemoryStoreImpl<PP>>::init_profile_metadata();
    }
};

}}

void InitNoCowInMemStore() {
    store::memory_nocow::Initializer<Profile> init0;
}

}
