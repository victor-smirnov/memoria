
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

#include <memoria/profiles/impl/cow_profile.hpp>
#include <memoria/store/memory_cow/threads/threads_memory_store_cow_impl.hpp>

namespace memoria {

using Profile = CowProfile<>;
using ApiProfileT = ApiProfile<Profile>;
template class IMemoryStore<ApiProfileT>;


SharedPtr<IMemoryStore<ApiProfileT>> create_memory_store()
{

    MaybeError maybe_error;

    auto snp = MakeShared<store::memory_cow::ThreadsMemoryStoreImpl<Profile>>(maybe_error);

    if (!maybe_error) {
        return std::move(snp);
    }
    else {
        std::move(maybe_error.get()).do_throw();
    }
}

SharedPtr<IMemoryStore<ApiProfileT>> load_memory_store(U8String path) {
    auto fileh = FileInputStreamHandler::create(path.data());
    return store::memory_cow::ThreadsMemoryStoreImpl<Profile>::load(fileh.get());
}

AllocSharedPtr<IMemoryStore<ApiProfileT>> load_memory_store(InputStreamHandler* input_stream)
{
    return store::memory_cow::ThreadsMemoryStoreImpl<Profile>::load(input_stream);
}


bool is_memory_store(U8String path) {
    auto fileh = FileInputStreamHandler::create(path.data());
    return store::memory_cow::ThreadsMemoryStoreImpl<Profile>::is_my_data(fileh.get());
}


namespace store {
namespace memory_cow {

template class ThreadsMemoryStoreImpl<Profile>;
template class ThreadsSnapshot<Profile, ThreadsMemoryStoreImpl<Profile>>;


template <typename PP>
struct Initializer {
    Initializer() {
        ThreadsSnapshot<PP, ThreadsMemoryStoreImpl<PP>>::init_profile_metadata();
    }
};

}}

void InitCoWInMemStore() {
    store::memory_cow::Initializer<Profile> init0;
}


}
