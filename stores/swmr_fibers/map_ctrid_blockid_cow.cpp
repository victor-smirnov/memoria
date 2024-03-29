
// Copyright 2017 Victor Smirnov
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

#include <memoria/containers/map/map_impl.hpp>

#include <memoria/core/strings/string.hpp>

namespace memoria {

using Profile = CowLiteProfile;
using CtrName = Map<ProfileCtrID<Profile>, ProfileBlockID<Profile>>;

MMA_INSTANTIATE_CTR_BTSS(CtrName, Profile, map_ctrid_blockid)

}

