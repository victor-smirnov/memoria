
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

#include <memoria/core/tools/span.hpp>

namespace memoria {

template <typename BlockID> class CounterCodec;

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
    //static constexpr size_t   EMBEDDED_COUNTERS_CAPACITY = 3520 / sizeof(ProfileBlockID<Profile>);
    static constexpr uint64_t VERSION = 1;

    // b18ba23f-fb6d-4c70-a2c5-f759a3c38b5a
    static constexpr UUID MAGICK1 = UUID(8091963556349774769ull, 6524523591532791202ull);

    // 67ca2314-39fe-4264-ae0d-28fcc7e78ef6
    static constexpr UUID MAGICK2 = UUID(7224616273360177767ull, 17766392426138176942ull);

    using BlockID    = ProfileBlockID<Profile>;
    using CommitID   = ApiProfileSnapshotID<ApiProfile<Profile>>;
    using SequenceID = uint64_t;

private:

    UUID magick1_;
    UUID magick2_;
    uint64_t profile_hash_;
    uint64_t version_;

    char magic_buffer_[512];
    SequenceID sequence_id_;
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

    void mark_for_rollback() noexcept {
        sequence_id_ -= 2;
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

    void init(uint64_t superblock_file_pos, uint64_t file_size, const CommitID& commit_id, size_t superblock_size, SequenceID sequence_id = 1)
    {
        magick1_ = MAGICK1;
        magick2_ = MAGICK2;
        profile_hash_ = PROFILE_HASH;
        version_ = VERSION;

        std::memset(magic_buffer_, 0, sizeof(magic_buffer_));

        sequence_id_ = sequence_id;
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
        commit_id_   = commit_id;

        superblock_file_pos_ = superblock_file_pos;
        file_size_           = other.file_size_;
        superblock_size_     = other.superblock_size_;
        global_block_counters_file_pos_ = other.global_block_counters_file_pos_;
        global_block_counters_size_ = other.global_block_counters_size_;
        store_status_        = other.store_status_;
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




template <typename Profile>
class CountersBlock {
    uint64_t block_size_;
    uint64_t next_block_pos_;
    uint64_t counters_size_;
    uint64_t reserved_;

    using BlockID = ProfileBlockID<Profile>;

    BlockID counters_[1];

public:
    CountersBlock() noexcept = default;

    uint64_t block_size() const noexcept {return block_size_;}
    uint64_t next_block_pos() const noexcept {return next_block_pos_;}
    uint64_t counters_size() const noexcept {return counters_size_;}

    void set_next_block_pos(uint64_t pos) noexcept {
        next_block_pos_ = pos;
    }

    void init(uint64_t block_size, uint64_t counters) {
        block_size_ = block_size;
        counters_size_ = counters;
    }

    auto get(size_t idx) noexcept {
        return CounterCodec<BlockID>::decode(counters_[idx]);
    }

    void set(size_t idx, const BlockID& id, int32_t counter) noexcept {
        counters_[idx] = CounterCodec<BlockID>::encode(id, counter);
    }

    static uint64_t estimate_block_size(uint64_t counters) noexcept {
        return sizeof(CountersBlock) + sizeof(BlockID) * (counters - 1);
    }

    static uint64_t estimate_block_capacity(uint64_t block_size) noexcept {
        uint64_t data_size = block_size - (sizeof(CountersBlock) - sizeof (BlockID));
        return data_size / sizeof(BlockID);
    }
};


template <>
class CounterCodec<CowBlockID<UUID>> {
    using BlockID = CowBlockID<UUID>;
public:
    CounterCodec() noexcept = default;

    struct Counter {
        BlockID block_id;
        int32_t refs;
    };

    static Counter decode(BlockID id) noexcept {
        int32_t cnt;

        if (id.value().version() == 14) {
            id.value().set_variant(15);
            cnt = 1;
        }
        else if (id.value().version() == 13) {
            id.value().set_variant(15);
            cnt = -1;
        }
        else if (id.value().version() == 12) {
            id.value().set_variant(4);
            cnt = 1;
        }
        else {
            id.value().set_variant(4);
            cnt = -1;
        }

        return Counter{id, cnt};
    }

    static BlockID encode(BlockID id, int32_t counter) noexcept
    {
        if (counter == 1)
        {
            if (id.value().version() == 15) {
                id.value().set_variant(14);
            }
            else {
                id.value().set_variant(12);
            }
        }
        else {
            if (id.value().version() == 15) {
                id.value().set_variant(13);
            }
            else {
                id.value().set_variant(11);
            }
        }

        return id;
    }
};


template <>
class CounterCodec<CowBlockID<uint64_t>> {
    using BlockID = CowBlockID<uint64_t>;
public:
    CounterCodec() noexcept = default;

    struct Counter {
        BlockID block_id;
        int32_t refs;
    };

    static Counter decode(BlockID id) noexcept {
        int32_t cnt;

        if (id.value() & 0x1ull) {
            cnt = 1;
        }
        else {
            cnt = -1;
        }

        id.value() >>= 2;

        return Counter{id, cnt};
    }

    static BlockID encode(BlockID id, int32_t counter) noexcept
    {
        id.value() <<= 1;

        if (counter == 1) {
            id.value() |= 0x1ull;
        }

        return id;
    }

};



}
