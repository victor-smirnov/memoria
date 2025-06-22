
// Copyright 2020-2025 Victor Smirnov
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

#include <memoria/profiles/common/block.hpp>

#include <memoria/profiles/common/common.hpp>
#include <memoria/profiles/common/block.hpp>

#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/core/reflection/typehash.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/tools/span.hpp>

#include <memoria/api/allocation_map/allocation_map_api.hpp>
#include <memoria/store/swmr/common/allocation_pool.hpp>

namespace memoria {

template <typename BlockID> class CounterCodec;


template <typename Profile>
class OLTPSuperblock {
public:
    static constexpr uint64_t PROFILE_HASH = TypeHash<Profile>::Value;
    static constexpr uint64_t VERSION = 1;

    // {1|1aca2f5d6e364dec44b69a39606c10706d0417f404c4d7964a36aee47e2838}
    static constexpr UID256 MAGIC = {14903646909958892705ull, 504902364791663428ull, 7601315585165705430ull, 109074745960063908ull};

    using BlockID    = ProfileBlockID<Profile>;
    using SnapshotID = ApiProfileSnapshotID<ApiProfile<Profile>>;
    using SequenceID = uint64_t;
    using CtrSizeT   = ProfileCtrSizeT<Profile>;

    using AllocationMetadataT = AllocationMetadata<ApiProfile<Profile>>;

    static constexpr size_t ALLOCATION_MAP_LEVELS = ICtrApi<AllocationMap, ApiProfile<Profile>>::LEVELS;
private:
    BasicBlockHeader header_;

    UID256 magic_;
    uint64_t profile_hash_;
    uint64_t version_;

    char magic_buffer_[256];
    SequenceID sequence_id_;
    SequenceID consistency_point_sequence_id_;
    SnapshotID snapshot_id_;

    BlockID id_;

    BlockID evc_queue_root_id_;
    BlockID directory_root_id_;
    BlockID allocator_root_id_;

    uint64_t block_uid_counter_;

    alignas(8)
    uint8_t reserved_[512];

    AllocationPoolData<128> allocation_pool_data_;

public:
    OLTPSuperblock() = default;

    const BlockID& id() const {return id_;}
    void set_id(const BlockID& id) const {
        id_ = id;
    }

    BlockID& evc_queue_root_id() {return evc_queue_root_id_;}
    BlockID& directory_root_id() {return directory_root_id_;}
    BlockID& allocator_root_id() {return allocator_root_id_;}

    const BlockID& evc_queue_root_id() const  {return evc_queue_root_id_;}
    const BlockID& directory_root_id() const  {return directory_root_id_;}
    const BlockID& allocator_root_id() const  {return allocator_root_id_;}

    uint64_t block_uid_counter() const {return block_uid_counter_;}
    uint64_t new_block_uid() {return ++block_uid_counter_;}

    auto& allocation_pool_data() {
        return allocation_pool_data_;
    }

    const auto& allocation_pool_data() const {
        return allocation_pool_data_;
    }

    const char* magic_buffer() const {return magic_buffer_;}
    char* magic_buffer() {return magic_buffer_;}

    const UID256& magic() const {
        return magic_;
    }

    uint64_t profile_hash() const {
        return profile_hash_;
    }

    uint64_t version() const {
        return version_;
    }

    const SequenceID& sequence_id() const {return sequence_id_;}
    SequenceID& sequence_id()  {return sequence_id_;}

    SequenceID consistency_point_sequence_id() const {
        return consistency_point_sequence_id_;
    }

    void inc_consistency_point_sequence_id() {
        consistency_point_sequence_id_++;
    }

    void mark_for_rollback()
    {
        consistency_point_sequence_id_ -= 2;
        snapshot_id_ = SnapshotID{};
    }

    const SnapshotID& snapshot_id() const {return snapshot_id_;}
    void set_snapshot_id(const SnapshotID& id) {snapshot_id_ = id;}

    bool match_magick() const {
        return magic_ == MAGIC;
    }

    bool match_profile_hash() const {
        return profile_hash_ == PROFILE_HASH;
    }

    bool match_version() const {
        return version_ == VERSION;
    }

    void init(
            const SnapshotID& commit_id,            
            SequenceID sequence_id,
            SequenceID cp_sequence_id
    ){
        header_ = BasicBlockHeader{};
        header_.set_block_size(4096);
        header_.cache_traits().set_group(BlockCachingGroup::SUPERBLOCK);

        magic_ = MAGIC;
        profile_hash_ = PROFILE_HASH;
        version_ = VERSION;

        std::memset(magic_buffer_, 0, sizeof(magic_buffer_));

        sequence_id_ = sequence_id;
        consistency_point_sequence_id_ = cp_sequence_id;
        snapshot_id_   = commit_id;

        evc_queue_root_id_ = BlockID{};
        directory_root_id_ = BlockID{};
        allocator_root_id_ = BlockID{};

        block_uid_counter_ = {};

        allocation_pool_data_.reset();
    }

    void init_from(            
            const OLTPSuperblock& other,
            const SnapshotID& commit_id
    ){
        header_ = other.header_;

        magic_ = other.magic_;
        profile_hash_ = other.profile_hash_;
        version_ = other.version_;

        evc_queue_root_id_ = other.evc_queue_root_id_;
        directory_root_id_ = other.directory_root_id_;
        allocator_root_id_ = other.allocator_root_id_;

        block_uid_counter_ = other.block_uid_counter_;

        sequence_id_ = other.sequence_id_ + 1;
        consistency_point_sequence_id_ = other.consistency_point_sequence_id_;
        snapshot_id_   = commit_id;

        allocation_pool_data_ = other.allocation_pool_data_;
    }

    void build_superblock_description()
    {
        return set_description(
            "T:OLTP; V:{}, SNP:{}, SEQ:{}, CP:{}, ID:{}",
            version_, snapshot_id_, sequence_id_,
            consistency_point_sequence_id_, id_
        );
    }


    template <typename... Args>
    void set_description(const char* fmt, Args&&... args)
    {
        U8String str = format_u8(fmt, std::forward<Args>(args)...);
        if (str.length() < sizeof(magic_buffer_))
        {
            MemCpyBuffer(str.data(), magic_buffer_, str.length());
            magic_buffer_[str.length()] = 0;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Supplied OLTP Superblock magic string is too long: {}", str).do_throw();
        }
    }

private:
    static int32_t allocator_block_size(size_t superblock_size)
    {
        int32_t sb_size = sizeof(OLTPSuperblock);
        int32_t alloc_size = sizeof(PackedAllocator);
        return superblock_size - (sb_size - alloc_size);
    }
};



namespace io {

template <typename Profile, typename X>
struct BlockPtrConvertible<OLTPSuperblock<Profile>, BasicBlockHeader, X>: BoolValue<true> {};

template <typename Profile, typename X>
struct BlockPtrCastable<BasicBlockHeader, OLTPSuperblock<Profile>, X>: BoolValue<true> {};

}

}
