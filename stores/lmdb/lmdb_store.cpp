
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

using Profile = NoCowProfile;
using ApiProfileT = ApiProfile<Profile>;

template struct ILMDBStore<ApiProfileT>;
template class LMDBStore<Profile>;
template class LMDBStoreWritableSnapshot<Profile>;
template class LMDBStoreReadOnlySnapshot<Profile>;


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



SharedPtr<ILMDBStore<ApiProfileT>> open_lmdb_store(U8StringView path)
{
    MaybeError maybe_error;
    auto ptr = MakeShared<LMDBStore<Profile>>(maybe_error, path, false);

    if (maybe_error) {
        std::move(maybe_error.value()).do_throw();
    }

    return ptr;
}

SharedPtr<ILMDBStore<ApiProfileT>> open_lmdb_store_readonly(U8StringView path)
{
    MaybeError maybe_error;
    auto ptr = MakeShared<LMDBStore<Profile>>(maybe_error, path, true);

    if (maybe_error) {
        std::move(maybe_error.value()).do_throw();
    }

    return ptr;
}

SharedPtr<ILMDBStore<ApiProfileT>> create_lmdb_store(U8StringView path, uint64_t store_size_mb)
{
    MaybeError maybe_error;
    auto ptr = MakeShared<LMDBStore<Profile>>(maybe_error, path, store_size_mb);

    if (maybe_error) {
        std::move(maybe_error.value()).do_throw();
    }

    ptr->init_store();

    return ptr;
}

bool is_lmdb_store(U8StringView path) {
    return LMDBStore<Profile>::is_my_file(path);
}

}
