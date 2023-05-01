
// Copyright 2023 Victor Smirnov
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

#include <memoria/api/store/swmr_store_api.hpp>

#include <memoria/store/oltp/blockio_oltp_store.hpp>

namespace memoria {

using Profile = CowLiteProfile;
using ApiProfileT = ApiProfile<Profile>;

template struct IOLTPStore<ApiProfileT>;
template class BlockIOOLTPStore<Profile>;
//template class OLTPStoreWritableSnapshot<Profile>;
//template class OLTPStoreReadOnlySnapshot<Profile>;


namespace {

template <typename PP>
struct Initializer {
    Initializer() {
//        OLTPStore<Profile>::init_profile_metadata();
    }
};

}

void InitOLTPStore() {
    Initializer<Profile> init0;
}


SharedPtr<IOLTPStore<ApiProfileT>> open_oltp_store_seastar(U8StringView path)
{
//    MaybeError maybe_error;
//    auto ptr = MakeShared<OLTPStore<Profile>>(maybe_error, path, false);

//    if (maybe_error) {
//        std::move(maybe_error.get()).do_throw();
//    }

//    return ptr;
    return {};
}

SharedPtr<IOLTPStore<ApiProfileT>> create_oltp_store_seastar(U8StringView path, uint64_t store_size_mb)
{
//    MaybeError maybe_error;
//    auto ptr = MakeShared<OLTPStore<Profile>>(maybe_error, path, store_size_mb);

//    if (maybe_error) {
//        std::move(maybe_error.get()).do_throw();
//    }

//    ptr->init_store();

//    return ptr;
    return {};
}

}
