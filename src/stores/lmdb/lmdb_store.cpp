
// Copyright 2021 Victor Smirnov
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
#include <memoria/store/lmdb/lmdb_store.hpp>

namespace memoria {

using Profile = NoCowProfile<>;
using ApiProfileT = ApiProfile<Profile>;

template struct ILMDBStore<ApiProfileT>;
template class LMDBStore<Profile>;
template class LMDBStoreWritableCommit<Profile>;
template class LMDBStoreReadOnlyCommit<Profile>;


namespace {

template <typename PP>
struct Initializer {
    Initializer() {
        LMDBStore<Profile>::init_profile_metadata();
    }
};

}

void InitLMDBStore() {
    Initializer<Profile> init0;
}



Result<SharedPtr<ILMDBStore<ApiProfileT>>> open_lmdb_store(U8StringView path)
{
    using ResultT = Result<SharedPtr<ILMDBStore<ApiProfileT>>>;

    MaybeError maybe_error;
    auto ptr = MakeShared<LMDBStore<Profile>>(maybe_error, path);

    if (maybe_error) {
        return std::move(maybe_error.get());
    }

    MEMORIA_TRY_VOID(ptr->do_open_file());

    return ResultT::of(ptr);
}

Result<SharedPtr<ILMDBStore<ApiProfileT>>> create_lmdb_store(U8StringView path, uint64_t store_size_mb)
{
    using ResultT = Result<SharedPtr<ILMDBStore<ApiProfileT>>>;

    MaybeError maybe_error;
    auto ptr = MakeShared<LMDBStore<Profile>>(maybe_error, path, store_size_mb);

    if (maybe_error) {
        return std::move(maybe_error.get());
    }

    MEMORIA_TRY_VOID(ptr->init_store());

    return ResultT::of(ptr);
}

}
