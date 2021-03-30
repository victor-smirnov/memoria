
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

#include <memoria/profiles/memory_cow/memory_cow_profile.hpp>
#include <memoria/store/memory_cow/threads/threads_memory_store_cow_impl.hpp>

namespace memoria {

using Profile = MemoryCoWProfile<>;
using ApiProfileT = ApiProfile<Profile>;
template class IMemoryStore<ApiProfileT>;


std::ostream& operator<<(std::ostream& out, const MemCoWBlockID<uint64_t>& block_id) noexcept {
    out << block_id.value();
    return out;
}



template <>
Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> IMemoryStore<ApiProfileT>::load(InputStreamHandler* input_stream) noexcept
{
    auto rr = store::memory_cow::ThreadsMemoryStoreImpl<Profile>::load(input_stream);
    if (rr.is_ok()) {
        return Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>::of(rr.get());
    }

    return std::move(rr).transfer_error();
}

template <>
Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> IMemoryStore<ApiProfileT>::load(U8String input_file) noexcept
{
    auto fileh = FileInputStreamHandler::create(input_file.data());
    auto rr = store::memory_cow::ThreadsMemoryStoreImpl<Profile>::load(fileh.get());

    if (rr.is_ok()) {
        return Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>::of(rr.get());
    }

    return std::move(rr).transfer_error();
}


template <>
Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>> IMemoryStore<ApiProfileT>::create() noexcept
{
    using ResultT = Result<AllocSharedPtr<IMemoryStore<ApiProfileT>>>;
    MaybeError maybe_error;

    auto snp = MakeShared<store::memory_cow::ThreadsMemoryStoreImpl<Profile>>(maybe_error);

    if (!maybe_error) {
        return ResultT::of(std::move(snp));
    }
    else {
        return std::move(maybe_error.get());
    }
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
