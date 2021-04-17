
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


#pragma once

#include <memoria/core/types.hpp>

namespace memoria {

template <typename ApiProfile> struct ApiProfileTraits;

template <typename Profile> struct ProfileTraits;

template <typename Profile>
using ApiProfile = typename ProfileTraits<Profile>::ApiProfileT;

template <typename Profile>
using ProfileBlockType = typename ProfileTraits<Profile>::BlockType;

template <typename Profile>
using ProfileStoreType = typename ProfileTraits<Profile>::StoreType;

template <typename Profile>
using ProfileBlockID = typename ProfileTraits<Profile>::BlockID;

template <typename Profile>
using ProfileBlockGUID = typename ProfileTraits<Profile>::BlockGUID;

template <typename Profile>
using ApiProfileBlockID = typename ApiProfileTraits<Profile>::ApiBlockID;


template <typename Profile>
using ProfileCtrID = typename ProfileTraits<Profile>::CtrID;

template <typename Profile>
using ApiProfileCtrID = typename ApiProfileTraits<Profile>::CtrID;


template <typename Profile>
using ProfileSnapshotID = typename ProfileTraits<Profile>::SnapshotID;

template <typename Profile>
using ApiProfileSnapshotID = typename ApiProfileTraits<Profile>::SnapshotID;

template <typename Profile>
using ProfileCtrSizeT = typename ProfileTraits<Profile>::CtrSizeT;

template <typename Profile>
using ApiProfileCtrSizeT = typename ApiProfileTraits<Profile>::CtrSizeT;



template <typename Profile>
bool ProfileIsCopyOnWrite = ProfileTraits<Profile>::IsCoW;

template <typename Profile>
using ProfileSharedBlockPtr = typename ProfileTraits<Profile>::SharedBlockPtr;

template <typename Profile>
using ProfileSharedBlockConstPtr = typename ProfileTraits<Profile>::SharedBlockConstPtr;

template <typename ID> struct IDTools;

template <typename Profile>
struct ProfileSpecificBlockTools;

}
