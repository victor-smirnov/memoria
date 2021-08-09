
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
#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/tools/span.hpp>

#include <memoria/api/allocation_map/allocation_map_api.hpp>


namespace memoria {

template <typename BlockID> class CounterCodec;

enum class SWMRStoreStatus {
    UNCLEAN, CLEAN
};

template <typename Profile>
class SWMRSuperblock {
public:
    static constexpr uint64_t PROFILE_HASH = TypeHash<Profile>::Value;
    static constexpr uint64_t VERSION = 1;

    // b18ba23f-fb6d-4c70-a2c5-f759a3c38b5a
    static constexpr UUID MAGICK1 = UUID(8091963556349774769ull, 6524523591532791202ull);

    // 67ca2314-39fe-4264-ae0d-28fcc7e78ef6
    static constexpr UUID MAGICK2 = UUID(7224616273360177767ull, 17766392426138176942ull);

    using BlockID    = ProfileBlockID<Profile>;
    using CommitID   = ApiProfileSnapshotID<ApiProfile<Profile>>;
    using SequenceID = uint64_t;
    using CtrSizeT   = ProfileCtrSizeT<Profile>;

    using AllocationMetadataT = AllocationMetadata<ApiProfile<Profile>>;

private:

    UUID magick1_;
    UUID magick2_;
    uint64_t profile_hash_;
    uint64_t version_;

    char magic_buffer_[512];
    SequenceID sequence_id_;
    SequenceID consistency_point_sequence_id_;
    CommitID commit_id_;
    uint64_t file_size_;
    uint64_t superblock_file_pos_;
    uint64_t superblock_size_;
    uint64_t global_block_counters_file_pos_;
    uint64_t global_block_counters_size_;

    BlockID history_root_id_;
    BlockID directory_root_id_;
    BlockID allocator_root_id_;
    BlockID blockmap_root_id_;

    SWMRStoreStatus store_status_;

    size_t preallocated_pool_size_;

    static constexpr size_t PREALLOCATED_BLOCKS = 64;
    uint64_t preallocated_blocks_[PREALLOCATED_BLOCKS];

public:
    SWMRSuperblock() noexcept = default;

    BlockID& history_root_id() noexcept   {return history_root_id_;}
    BlockID& directory_root_id() noexcept {return directory_root_id_;}
    BlockID& allocator_root_id() noexcept {return allocator_root_id_;}
    BlockID& blockmap_root_id() noexcept  {return blockmap_root_id_;}

    const BlockID& history_root_id() const noexcept   {return history_root_id_;}
    const BlockID& directory_root_id() const noexcept {return directory_root_id_;}
    const BlockID& allocator_root_id() const noexcept {return allocator_root_id_;}
    const BlockID& blockmap_root_id() const noexcept {return blockmap_root_id_;}

    const char* magic_buffer() const noexcept {return magic_buffer_;}
    char* magic_buffer() noexcept {return magic_buffer_;}

    const UUID& magick1() const noexcept {
        return magick1_;
    }

    const UUID& magick2() const noexcept {
        return magick2_;
    }

    uint64_t profile_hash() const noexcept {
        return profile_hash_;
    }

    uint64_t version() const noexcept {
        return version_;
    }

    const SequenceID& sequence_id() const noexcept {return sequence_id_;}
    SequenceID& sequence_id() noexcept {return sequence_id_;}

    SequenceID consistency_point_sequence_id() const noexcept {
        return consistency_point_sequence_id_;
    }

    void inc_consistency_point_sequence_id() noexcept {
        consistency_point_sequence_id_++;
    }

    void mark_for_rollback() noexcept
    {
        consistency_point_sequence_id_ -= 2;
        commit_id_ = CommitID{};
    }

    const CommitID& commit_id() const noexcept {return commit_id_;}
    void set_commit_id(const CommitID& id) noexcept {commit_id_ = id;}

    uint64_t file_size() const noexcept {return file_size_;}
    void set_file_size(uint64_t size) noexcept {file_size_ = size;}

    uint64_t superblock_size() const noexcept {return superblock_size_;}
    void set_superblock_size(uint64_t size) noexcept {superblock_size_ = size;}

    uint64_t superblock_file_pos() const noexcept {return superblock_file_pos_;}
    void set_superblock_file_pos(uint64_t pos) noexcept {superblock_file_pos_ = pos;}

    uint64_t global_block_counters_file_pos() const noexcept {return global_block_counters_file_pos_;}
    void set_global_block_counters_file_pos(uint64_t pos) noexcept {global_block_counters_file_pos_ = pos;}

    uint64_t global_block_counters_size() const noexcept {return global_block_counters_size_;}
    void set_global_block_counters_size(uint64_t size) noexcept {global_block_counters_size_ = size;}

    SWMRStoreStatus status() const noexcept {return store_status_;}

    bool is_clean() const noexcept {return store_status_ == SWMRStoreStatus::CLEAN;}
    bool is_unclean() const noexcept {return store_status_ == SWMRStoreStatus::UNCLEAN;}

    void set_clean_status() noexcept {
        store_status_ = SWMRStoreStatus::CLEAN;
    }

    bool match_magick() const noexcept {
        return magick1_ == MAGICK1 && magick2_ == MAGICK2;
    }

    bool match_profile_hash() const noexcept {
        return profile_hash_ == PROFILE_HASH;
    }

    bool match_version() const noexcept {
        return version_ == VERSION;
    }

    Span<const uint64_t> preallocated_blocks() const noexcept {
        return Span<const uint64_t>(preallocated_blocks_, PREALLOCATED_BLOCKS);
    }

    size_t preallocated_pool_size() const noexcept {
        return preallocated_pool_size_;
    }

    size_t preallocated_pool_capacity() const noexcept {
        return PREALLOCATED_BLOCKS - preallocated_pool_size_;
    }

    Optional<AllocationMetadataT> allocate_one(size_t reserve = 0)
    {
        for (size_t i = 0, rcnt = 0; i < PREALLOCATED_BLOCKS && preallocated_pool_size_ > 0; i++)
        {
            if (preallocated_blocks_[i])
            {
                if (rcnt == reserve)
                {
                    AllocationMetadataT meta{(CtrSizeT)preallocated_blocks_[i], 1, 0};
                    preallocated_blocks_[i] = 0;
                    preallocated_pool_size_--;
                    return meta;
                }

                rcnt++;
            }
        }

        return Optional<AllocationMetadataT>{};
    }

    template <typename AllocationPool>
    size_t preallocate_from_pool(AllocationPool& allocation_pool)
    {
        return preallocate_from_fn([&]{
            return allocation_pool.allocate_one(0);
        });
    }

    template <typename Fn>
    size_t preallocate_from_fn(Fn&& fn)
    {
        size_t c = 0;
        for (size_t i = 0; i < PREALLOCATED_BLOCKS; i++) {
            if (preallocated_blocks_[i] == 0) {
                auto alc = fn();
                if (alc) {
                    preallocated_blocks_[i] = alc.get().position();
                    preallocated_pool_size_++;
                    c++;
                }
                else {
                    break;
                }
            }
        }
        return c;
    }

    std::pair<size_t, AllocationMetadataT> get_allocation() const
    {
        for (size_t i = 0; i < PREALLOCATED_BLOCKS; i++) {
            if (preallocated_blocks_[i]) {
                return std::make_pair(
                    i, AllocationMetadataT{(CtrSizeT)preallocated_blocks_[i], 1, 0}
                );
            }
        }

        MEMORIA_MAKE_GENERIC_ERROR("Superblock's allocation pool is empty").do_throw();
    }

    void remove_block_from_pool(size_t idx)
    {
        if (MMA_LIKELY(preallocated_blocks_[idx])) {
            preallocated_blocks_[idx] = 0;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Trying to mark unallocated block {} as allocated in the superblock {}",
                idx,
                commit_id_
            ).do_throw();
        }
    }

    void init(
            uint64_t superblock_file_pos,
            uint64_t file_size,
            const CommitID& commit_id,
            size_t superblock_size,
            SequenceID sequence_id = 1,
            SequenceID cp_sequence_id = 1
    )
    {
        magick1_ = MAGICK1;
        magick2_ = MAGICK2;
        profile_hash_ = PROFILE_HASH;
        version_ = VERSION;

        std::memset(magic_buffer_, 0, sizeof(magic_buffer_));

        sequence_id_ = sequence_id;
        consistency_point_sequence_id_ = cp_sequence_id;
        commit_id_   = commit_id;

        superblock_file_pos_ = superblock_file_pos;
        file_size_           = file_size;
        superblock_size_     = superblock_size;

        global_block_counters_file_pos_ = 0;
        global_block_counters_size_ = 0;

        history_root_id_   = BlockID{};
        directory_root_id_ = BlockID{};
        allocator_root_id_ = BlockID{};
        blockmap_root_id_  = BlockID{};

        store_status_ = SWMRStoreStatus::UNCLEAN;

        preallocated_pool_size_ = 0;
        std::memset(preallocated_blocks_, 0, PREALLOCATED_BLOCKS * sizeof(uint64_t));
    }

    void init_from(const SWMRSuperblock& other, uint64_t superblock_file_pos, const CommitID& commit_id)
    {
        magick1_ = other.magick1_;
        magick2_ = other.magick2_;
        profile_hash_ = other.profile_hash_;
        version_ = other.version_;

        history_root_id_   = other.history_root_id_;
        directory_root_id_ = other.directory_root_id_;
        allocator_root_id_ = other.allocator_root_id_;
        blockmap_root_id_  = other.blockmap_root_id_;

        sequence_id_ = other.sequence_id_ + 1;
        consistency_point_sequence_id_ = other.consistency_point_sequence_id_;
        commit_id_   = commit_id;

        superblock_file_pos_ = superblock_file_pos;
        file_size_           = other.file_size_;
        superblock_size_     = other.superblock_size_;
        global_block_counters_file_pos_ = other.global_block_counters_file_pos_;
        global_block_counters_size_ = other.global_block_counters_size_;
        store_status_        = other.store_status_;

        preallocated_pool_size_ = other.preallocated_pool_size_;
        std::memcpy(preallocated_blocks_, other.preallocated_blocks_, PREALLOCATED_BLOCKS * sizeof(uint64_t));
    }

    void build_superblock_description()
    {
        return set_description(
            "MEMORIA SWMR MAPPED STORE. VERSION:{}; CommitID:{}, SequenceID:{}, SuperblockFilePos:{}, FileSize:{}, Status:{}, Counters:{}, CounterAt:{}, Profile:{}",
            version_, commit_id_, sequence_id_, superblock_file_pos_,
            file_size_, (is_clean() ? "CLEAN" : "UNCLEAN"), global_block_counters_size_,
                    global_block_counters_file_pos_,
                    TypeNameFactory<Profile>::name()
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
