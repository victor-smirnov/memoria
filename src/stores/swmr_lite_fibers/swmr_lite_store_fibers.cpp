
// Copyright 2019-2021 Victor Smirnov
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

using Profile = CowLiteProfile<>;
using ApiProfileT = ApiProfile<Profile>;

template struct ISWMRStore<ApiProfileT>;
template class MappedSWMRStore<Profile>;
template class MappedSWMRStoreWritableCommit<Profile>;
template class MappedSWMRStoreReadOnlyCommit<Profile>;

namespace {

template <typename PP>
struct Initializer {
    Initializer() {
        MappedSWMRStore<Profile>::init_profile_metadata();
    }
};

}

void InitLiteSWMRStore() {
    Initializer<Profile> init0;
}



Result<SharedPtr<ISWMRStore<ApiProfileT>>> open_lite_swmr_store(U8StringView path)
{
    using ResultT = Result<SharedPtr<ISWMRStore<ApiProfileT>>>;

    MaybeError maybe_error;
    auto ptr = MakeShared<MappedSWMRStore<Profile>>(maybe_error, path);

    if (maybe_error) {
        return std::move(maybe_error.get());
    }

    MEMORIA_TRY_VOID(ptr->do_open_file());

    return ResultT::of(ptr);
}

Result<SharedPtr<ISWMRStore<ApiProfileT>>> create_lite_swmr_store(U8StringView path, uint64_t store_size_mb)
{
    using ResultT = Result<SharedPtr<ISWMRStore<ApiProfileT>>>;

    MaybeError maybe_error;
    auto ptr = MakeShared<MappedSWMRStore<Profile>>(maybe_error, path, store_size_mb);

    if (maybe_error) {
        return std::move(maybe_error.get());
    }

    MEMORIA_TRY_VOID(ptr->init_store());

    return ResultT::of(ptr);
}

}
