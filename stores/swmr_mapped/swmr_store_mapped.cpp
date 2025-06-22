
// Copyright 2019-2025 Victor Smirnov
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

#include <memoria/profiles/impl/cow_lite_profile.hpp>
#include <memoria/store/swmr/mapped/swmr_mapped_store.hpp>

namespace memoria {

using Profile = CowProfile;
using ApiProfileT = ApiProfile<Profile>;

template struct ISWMRStore<ApiProfileT>;
template class MappedSWMRStore<Profile>;
template class MappedSWMRStoreWritableSnapshot<Profile>;
template class MappedSWMRStoreReadOnlySnapshot<Profile>;
template class SWMRStoreHistoryViewImpl<Profile>;

namespace {

template <typename PP>
struct Initializer {
    Initializer() {
        MappedSWMRStore<Profile>::init_profile_metadata();
    }
};

}

void InitSWMRStore() {
    Initializer<Profile> init0;
}



SharedPtr<ISWMRStore<ApiProfileT>> open_swmr_store(U8StringView path, const SWMRParams& params)
{
    MaybeError maybe_error;
    auto ptr = MakeShared<MappedSWMRStore<Profile>>(maybe_error, path, params);

    if (maybe_error) {
        std::move(*maybe_error).do_throw();
    }

    ptr->do_open_store();

    return ptr;
}

SharedPtr<ISWMRStore<ApiProfileT>> create_swmr_store(U8StringView path, const SWMRParams& params)
{
    MaybeError maybe_error;
    auto ptr = MakeShared<MappedSWMRStore<Profile>>(maybe_error, path, params, CreateMappedStore{});

    if (maybe_error) {
        std::move(*maybe_error).do_throw();
    }

    ptr->init_store();

    return ptr;
}

bool is_swmr_store(U8StringView path) {
    return MappedSWMRStore<Profile>::is_my_file(path);
}

}
