
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


#pragma once

#include <memoria/profiles/common/common.hpp>
#include <memoria/profiles/common/block_cow.hpp>
#include <memoria/profiles/common/block_lw_cow.hpp>
#include <memoria/profiles/core_api/core_api_profile.hpp>
#include <memoria/profiles/impl/cow_impl_common.hpp>

#include <memoria/core/container/store.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/tools/uid_64.hpp>



#include <unordered_set>

namespace memoria {

template <typename ChildType = void>
class CowLiteProfileT  {};
using CowLiteProfile = CowLiteProfileT<>;

template<>
struct TypeHash<CowLiteProfile>: HasValue<uint64_t, 348752398579234857ull> {};

template <>
struct ProfileTraits<CowLiteProfile>: ApiProfileTraits<CoreApiProfile> {
    using Base = ApiProfileTraits<CoreApiProfile>;

    using typename Base::CtrID;
    using typename Base::CtrSizeT;
    using typename Base::SnapshotID;

    using BlockGUID     = UID64;
    using BlockID       = CowBlockID<BlockGUID>;
    using Profile       = CowLiteProfile;

    using Block = AbstractPage<BlockID, SnapshotID>;
    using BlockType = Block;

    using StoreType = ICowStore<Profile>;

    using BlockShared = PageShared<StoreType, Block, BlockID>;

    template <typename TargetBlockType>
    using SharedBlockPtrTF  = CowSharedBlockPtr<TargetBlockType, StoreType, BlockShared>;

    using SharedBlockPtr        = CowSharedBlockPtr<Block, StoreType, BlockShared>;
    using SharedBlockConstPtr   = CowSharedBlockPtr<const Block, StoreType, BlockShared>;

    static constexpr bool IsCoW = true;

    static BlockID make_random_block_id() {
        return BlockID();
    }

    static UUID make_random_block_guid() {
        return UUID::make_random();
    }

    static CtrID make_random_ctr_id() {
        return CtrID::make_random();
    }

    static SnapshotID make_random_snapshot_id() {
        return SnapshotID::make_random();
    }
};


template <>
struct ProfileSpecificBlockTools<CowLiteProfile>{

    template <typename BlockT>
    static void after_deserialization(BlockT* block) noexcept {
        // do nothing here
    }
};




template <>
struct IBlockOperations<CowLiteProfile>: IBlockOperationsBase<CowLiteProfile> {

    using Base = IBlockOperationsBase<CowLiteProfile>;
    using typename Base::IDValueResolver;
    using typename Base::BlockType;

    virtual void cow_resolve_ids(BlockType* block, const IDValueResolver* id_resolver) const = 0;
};





}
