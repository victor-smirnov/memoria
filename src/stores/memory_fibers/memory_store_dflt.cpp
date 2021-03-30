
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

#include <memoria/store/memory/fibers/fibers_memory_store_impl.hpp>

namespace memoria {

using Profile = DefaultProfile<>;
using ApiProfileT = ApiProfile<Profile>;

template class IMemoryStore<ApiProfile<Profile>>;


Result<SharedPtr<IMemoryStore<ApiProfileT>>> create_memory_store_noncow() noexcept
{
    using ResultT = Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>;
    MaybeError maybe_error;

    auto snp = MakeShared<store::memory::FibersMemoryStoreImpl<Profile>>(maybe_error);

    if (!maybe_error) {
        return ResultT::of(std::move(snp));
    }
    else {
        return std::move(maybe_error.get());
    }
}

Result<SharedPtr<IMemoryStore<ApiProfileT>>> load_memory_store_noncow(U8String path) noexcept {
    auto fileh = FileInputStreamHandler::create(path.data());
    auto rr = store::memory::FibersMemoryStoreImpl<Profile>::load(fileh.get());

    if (rr.is_ok()) {
        return Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>::of(rr.get());
    }

    return std::move(rr).transfer_error();
}

Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> load_memory_store_noncow(InputStreamHandler* input_stream) noexcept
{
    auto rr = store::memory::FibersMemoryStoreImpl<Profile>::load(input_stream);
    if (rr.is_ok()) {
        return Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>::of(rr.get());
    }

    return std::move(rr).transfer_error();
}

namespace store {
namespace memory {
    
template class FibersMemoryStoreImpl<Profile>;
template class FibersSnapshot<Profile, FibersMemoryStoreImpl<Profile>>;

template <typename PP>
struct Initializer {
    Initializer() {
        FibersSnapshot<PP, FibersMemoryStoreImpl<PP>>::init_profile_metadata();
    }
};

}}

void InitDefaultInMemStore() {
    store::memory::Initializer<Profile> init0;
}

}
