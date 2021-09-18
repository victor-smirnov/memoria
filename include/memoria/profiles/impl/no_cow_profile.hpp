
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

#include <memoria/profiles/common/common.hpp>
#include <memoria/profiles/common/block_nocow.hpp>
#include <memoria/profiles/core_api/core_api_profile.hpp>
#include <memoria/core/container/store.hpp>

#include <memoria/core/tools/uid_256.hpp>

namespace memoria {

template <typename ChildType = void>
class NoCowProfile  {};

template <>
struct ProfileTraits<NoCowProfile<>>: ApiProfileTraits<CoreApiProfile<>> {
    using Base = ApiProfileTraits<CoreApiProfile<>>;

    using typename Base::CtrID;
    using typename Base::CtrSizeT;
    using typename Base::SnapshotID;

    using BlockGUID = UID256;
    using BlockID = UID256;
    using Profile = NoCowProfile<>;

    using Block = AbstractPage <BlockGUID, BlockID, EmptyType, SnapshotID>;
    using BlockType = Block;

    using StoreType = IStore<Profile>;

    using BlockShared = PageShared<StoreType, Block, BlockID>;

    template <typename TargetBlockType>
    using SharedBlockPtrTF = NoCowSharedBlockPtr<TargetBlockType, StoreType, BlockShared>;

    using SharedBlockPtr       = NoCowSharedBlockPtr<Block, StoreType, BlockShared>;
    using SharedBlockConstPtr  = NoCowSharedBlockPtr<const Block, StoreType, BlockShared>;

    static constexpr bool IsCoW = false;

    static BlockID make_random_block_id() {
        return BlockID::make_random();
    }

    static BlockID make_random_block_guid() {
        return BlockID::make_random();
    }
};

template <>
struct ProfileSpecificBlockTools<NoCowProfile<>>{
    template <typename BlockT>
    static void after_deserialization(BlockT*) noexcept {
        // do nothing here
    }
};

}
