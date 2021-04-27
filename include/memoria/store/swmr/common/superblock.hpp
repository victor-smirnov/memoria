
// Copyright 2020-2021 Victor Smirnov
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
#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>


namespace memoria {

enum class SWMRStoreStatus {
    UNCLEAN, CLEAN
};

enum class SWMRCommitStateMetadataBits: uint64_t {
    PERSISTENT = 1,
    STATE_BITS = 8,
    ALL_STATES_MASK = PERSISTENT
};

static inline constexpr uint64_t val(SWMRCommitStateMetadataBits bits) {
    return static_cast<uint64_t>(bits);
}

template <typename Profile>
class SWMRSuperblock {
public:
    static constexpr uint64_t PROFILE_HASH = TypeHash<Profile>::Value;

    // b18ba23f-fb6d-4c70-a2c5-f759a3c38b5a
    static constexpr UUID MAGICK1 = UUID(8091963556349774769ull, 6524523591532791202ull);

    // 67ca2314-39fe-4264-ae0d-28fcc7e78ef6
    static constexpr UUID MAGICK2 = UUID(7224616273360177767ull, 17766392426138176942ull);

    using BlockID    = ProfileBlockID<Profile>;
    using CommitUUID = ProfileSnapshotID<Profile>;
    using CommitID   = ApiProfileSnapshotID<ApiProfile<Profile>>;
    using SequenceID = uint64_t;

private:

    UUID magick1_;
    UUID magick2_;
    uint64_t profile_hash_;

    char magic_buffer_[256];
    char reserved_[4 * 8];
    SequenceID sequence_id_;
    CommitID commit_id_;
    uint64_t file_size_;
    uint64_t superblock_file_pos_;
    uint64_t superblock_size_;
    uint64_t block_counters_file_pos_;
    uint64_t block_counters_size_;

    BlockID history_root_id_;
    BlockID directory_root_id_;
    BlockID counters_root_id_;
    BlockID allocator_root_id_;
    BlockID blockmap_root_id_;

    CommitUUID commit_uuid_;

    SWMRStoreStatus store_status_;

    PackedAllocator allocator_;

public:
    SWMRSuperblock() noexcept {}

    BlockID& history_root_id() noexcept   {return history_root_id_;}
    BlockID& directory_root_id() noexcept {return directory_root_id_;}
    BlockID& counters_root_id() noexcept  {return counters_root_id_;}
    BlockID& allocator_root_id() noexcept {return allocator_root_id_;}
    BlockID& blockmap_root_id() noexcept  {return blockmap_root_id_;}

    const BlockID& history_root_id() const noexcept   {return history_root_id_;}
    const BlockID& directory_root_id() const noexcept {return directory_root_id_;}
    const BlockID& counters_root_id() const noexcept  {return counters_root_id_;}
    const BlockID& allocator_root_id() const noexcept {return allocator_root_id_;}
    const BlockID& blockmap_root_id() const noexcept {return blockmap_root_id_;}

    const char* magic_buffer() const noexcept {return magic_buffer_;}
    char* magic_buffer() noexcept {return magic_buffer_;}

    const UUID& magick1() const {
        return magick1_;
    }

    const UUID& magick2() const {
        return magick2_;
    }

    uint64_t profile_hash() const {
        return profile_hash_;
    }

    const SequenceID& sequence_id() const noexcept {return sequence_id_;}
    SequenceID& sequence_id() noexcept {return sequence_id_;}

    void mark_for_rollback() noexcept {
        sequence_id_ -= 2;
        commit_id_ = CommitID{};
    }

    const CommitID& commit_id() const noexcept {return commit_id_;}
    CommitID& commit_id() noexcept {return commit_id_;}

    uint64_t file_size() const noexcept {return file_size_;}
    uint64_t& file_size() noexcept {return file_size_;}

    uint64_t superblock_size() const noexcept {return superblock_size_;}
    uint64_t& superblock_size() noexcept {return superblock_size_;}

    uint64_t superblock_file_pos() const noexcept {return superblock_file_pos_;}
    uint64_t& superblock_file_pos() noexcept {return superblock_file_pos_;}

    uint64_t block_counters_file_pos() const noexcept {return block_counters_file_pos_;}
    uint64_t& block_counters_file_pos() noexcept {return block_counters_file_pos_;}

    uint64_t block_counters_size() const noexcept {return block_counters_size_;}
    uint64_t& block_counters_size() noexcept {return block_counters_size_;}

    const CommitUUID& commit_uuid() const noexcept {return commit_uuid_;}
    CommitUUID& commit_uuid() noexcept {return commit_uuid_;}

    SWMRStoreStatus status() const noexcept {return store_status_;}

    bool is_clean() const noexcept {return store_status_ == SWMRStoreStatus::CLEAN;}
    bool is_unclean() const noexcept {return store_status_ == SWMRStoreStatus::UNCLEAN;}

    void set_clean_status() noexcept {
        store_status_ = SWMRStoreStatus::CLEAN;
    }

    bool match_magick() noexcept {
        return magick1_ == MAGICK1 && magick2_ == MAGICK2;
    }

    bool match_profile_hash() noexcept {
        return profile_hash_ == PROFILE_HASH;
    }

    void init(uint64_t superblock_file_pos, uint64_t file_size, const CommitID& commit_id, size_t superblock_size, SequenceID sequence_id = 1)
    {
        magick1_ = MAGICK1;
        magick2_ = MAGICK2;
        profile_hash_ = PROFILE_HASH;

        std::memset(magic_buffer_, 0, sizeof(magic_buffer_));
        std::memset(reserved_, 0, sizeof(reserved_));

        sequence_id_ = sequence_id;
        commit_id_   = commit_id;

        superblock_file_pos_ = superblock_file_pos;
        file_size_           = file_size;
        superblock_size_     = superblock_size;

        block_counters_file_pos_ = 0;
        block_counters_size_ = 0;

        history_root_id_   = BlockID{};
        directory_root_id_ = BlockID{};
        counters_root_id_  = BlockID{};
        allocator_root_id_ = BlockID{};
        blockmap_root_id_  = BlockID{};

        store_status_ = SWMRStoreStatus::UNCLEAN;

        return allocator_.init(allocator_block_size(superblock_size), 1).get_or_throw();
    }

    void init_from(const SWMRSuperblock& other, uint64_t superblock_file_pos, const CommitID& commit_id)
    {
        magick1_ = other.magick1_;
        magick2_ = other.magick2_;
        profile_hash_ = other.profile_hash_;

        history_root_id_   = other.history_root_id_;
        directory_root_id_ = other.directory_root_id_;
        counters_root_id_  = other.counters_root_id_;
        allocator_root_id_ = other.allocator_root_id_;
        blockmap_root_id_  = other.blockmap_root_id_;

        sequence_id_ = other.sequence_id_ + 1;
        commit_id_   = commit_id;

        superblock_file_pos_ = superblock_file_pos;
        file_size_           = other.file_size_;
        superblock_size_     = other.superblock_size_;
        block_counters_file_pos_ = other.block_counters_file_pos_;
        block_counters_size_ = other.block_counters_size_;
        store_status_        = other.store_status_;

        return allocator_.init(allocator_block_size(other.superblock_size_), 1).get_or_throw();
    }

    void build_superblock_description()
    {
        return set_description(
            "MEMORIA SWMR MAPPED STORE. VERSION:{}; CommitID:{}, SequenceID:{}, SuperblockFilePos:{}, FileSize:{}",
            0, commit_id_, sequence_id_, superblock_file_pos_, file_size_
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
            MEMORIA_MAKE_GENERIC_ERROR("Supplied SWMR Superblock magic string is too long: {}", str).do_throw();
        }
    }

private:
    static int32_t allocator_block_size(size_t superblock_size) noexcept
    {
        int32_t sb_size = sizeof(SWMRSuperblock);
        int32_t alloc_size = sizeof(PackedAllocator);
        return superblock_size - (sb_size - alloc_size);
    }
};

}
