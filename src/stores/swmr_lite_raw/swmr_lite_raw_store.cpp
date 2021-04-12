
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
#include <memoria/store/swmr/lite_raw/swmr_lite_raw_store.hpp>

namespace memoria {

using Profile = CowLiteProfile<>;
using ApiProfileT = ApiProfile<Profile>;

template struct ISWMRStore<ApiProfileT>;
template class SWMRLiteRawStore<Profile>;
template class MappedSWMRStoreWritableCommit<Profile>;
template class MappedSWMRStoreReadOnlyCommit<Profile>;

namespace {

template <typename PP>
struct Initializer {
    Initializer() {
        SWMRLiteRawStore<Profile>::init_profile_metadata();
    }
};

}

void InitLiteRawSWMRStore() {
    Initializer<Profile> init0;
}



Result<SharedPtr<ISWMRStore<ApiProfileT>>> open_lite_raw_swmr_store(Span<uint8_t> buffer)
{
    using ResultT = Result<SharedPtr<ISWMRStore<ApiProfileT>>>;

    auto ptr = MakeShared<SWMRLiteRawStore<Profile>>(buffer);

    MEMORIA_TRY_VOID(ptr->do_open_buffer());

    return ResultT::of(ptr);
}

Result<SharedPtr<ISWMRStore<ApiProfileT>>> create_lite_raw_swmr_store(Span<uint8_t> buffer)
{
    using ResultT = Result<SharedPtr<ISWMRStore<ApiProfileT>>>;

    auto ptr = MakeShared<SWMRLiteRawStore<Profile>>(buffer);

    MEMORIA_TRY_VOID(ptr->init_store());

    return ResultT::of(ptr);
}

}
