

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

#include <memoria/profiles/common/metadata.hpp>

#include <memoria/profiles/impl/no_cow_profile.hpp>
#include <memoria/profiles/impl/cow_lite_profile.hpp>
#include <memoria/profiles/impl/cow_profile.hpp>

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

namespace memoria {

template <typename Profile>
ProfileMetadataStore<Profile>& ProfileMetadataStore<Profile>::global()
{
    static ProfileMetadataStore<Profile> metadata;
    return metadata;
}

template <typename Profile>
const ProfileMetadataPtr<Profile>& ProfileMetadata<Profile>::local()
{
    static thread_local ProfileMetadataPtr<Profile> metadata(
        std::make_shared<ProfileMetadata>(ProfileMetadataStore<Profile>::global())
    );

    return metadata;
}

template class ProfileMetadataStore<NoCowProfile<>>;
template class ProfileMetadata<NoCowProfile<>>;

template class ProfileMetadataStore<CowLiteProfile<>>;
template class ProfileMetadata<CowLiteProfile<>>;

template class ProfileMetadataStore<CowProfile<>>;
template class ProfileMetadata<CowProfile<>>;

void InitProfileMetadata() {
    ProfileMetadataStore<NoCowProfile<>>::Init init_store1;
    ProfileMetadataStore<CowLiteProfile<>>::Init init_store2;
    ProfileMetadataStore<CowProfile<>>::Init init_store3;
}



}

