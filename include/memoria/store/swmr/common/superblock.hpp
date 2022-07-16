
// Copyright 2020-2022 Victor Smirnov
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
#include <memoria/store/swmr/common/allocation_pool.hpp>


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
    using SnapshotID   = ApiProfileSnapshotID<ApiProfile<Profile>>;
    using SequenceID = uint64_t;
    using CtrSizeT   = ProfileCtrSizeT<Profile>;

    using AllocationMetadataT = AllocationMetadata<ApiProfile<Profile>>;

    static constexpr size_t METADATA_SIZE = 1536;
    static constexpr int32_t ALLOCATION_MAP_LEVELS = ICtrApi<AllocationMap, ApiProfile<Profile>>::LEVELS;

private:

    UUID magick1_;
    UUID magick2_;
    uint64_t profile_hash_;
    uint64_t version_;

    char magic_buffer_[512];
    SequenceID sequence_id_;
    SequenceID consistency_point_sequence_id_;
    SnapshotID snapshot_id_;
    uint64_t file_size_;
    uint64_t superblock_file_pos_;
    uint64_t superblock_size_;
    uint64_t global_block_counters_file_pos_;
    uint64_t global_block_counters_size_;
    uint64_t global_block_counters_blocks_;

    BlockID history_root_id_;
    BlockID directory_root_id_;
    BlockID allocator_root_id_;
    BlockID blockmap_root_id_;

    SWMRStoreStatus store_status_;
    AllocationPoolData<ApiProfile<Profile>, ALLOCATION_MAP_LEVELS> allocation_pool_data_;

    uint8_t metadata_[METADATA_SIZE];

public:
    SWMRSuperblock()  = default;

    BlockID& history_root_id()    {return history_root_id_;}
    BlockID& directory_root_id()  {return directory_root_id_;}
    BlockID& allocator_root_id()  {return allocator_root_id_;}
    BlockID& blockmap_root_id()   {return blockmap_root_id_;}

    const BlockID& history_root_id() const    {return history_root_id_;}
    const BlockID& directory_root_id() const  {return directory_root_id_;}
    const BlockID& allocator_root_id() const  {return allocator_root_id_;}
    const BlockID& blockmap_root_id() const  {return blockmap_root_id_;}

    auto& allocation_pool_data() {
        return allocation_pool_data_;
    }

    const auto& allocation_pool_data() const {
        return allocation_pool_data_;
    }

    Span<uint8_t> metadata()  {
        return Span<uint8_t>(metadata_, METADATA_SIZE);
    }

    Span<const uint8_t> metadata() const  {
        return Span<const uint8_t>(metadata_, METADATA_SIZE);
    }

    const char* magic_buffer() const  {return magic_buffer_;}
    char* magic_buffer()  {return magic_buffer_;}

    const UUID& magick1() const  {
        return magick1_;
    }

    const UUID& magick2() const  {
        return magick2_;
    }

    uint64_t profile_hash() const  {
        return profile_hash_;
    }

    uint64_t version() const  {
        return version_;
    }

    const SequenceID& sequence_id() const  {return sequence_id_;}
    SequenceID& sequence_id()  {return sequence_id_;}

    SequenceID consistency_point_sequence_id() const  {
        return consistency_point_sequence_id_;
    }

    void inc_consistency_point_sequence_id()  {
        consistency_point_sequence_id_++;
    }

    void mark_for_rollback()
    {
        consistency_point_sequence_id_ -= 2;
        snapshot_id_ = SnapshotID{};
    }

    const SnapshotID& snapshot_id() const  {return snapshot_id_;}
    void set_snapshot_id(const SnapshotID& id)  {snapshot_id_ = id;}

    uint64_t file_size() const  {return file_size_;}
    void set_file_size(uint64_t size)  {file_size_ = size;}

    uint64_t superblock_size() const  {return superblock_size_;}
    void set_superblock_size(uint64_t size)  {superblock_size_ = size;}

    uint64_t superblock_file_pos() const  {return superblock_file_pos_;}
    void set_superblock_file_pos(uint64_t pos)  {superblock_file_pos_ = pos;}

    uint64_t global_block_counters_file_pos() const  {return global_block_counters_file_pos_;}
    void set_global_block_counters_file_pos(uint64_t pos)  {global_block_counters_file_pos_ = pos;}

    uint64_t global_block_counters_size() const  {return global_block_counters_size_;}
    void set_global_block_counters_size(uint64_t size)  {global_block_counters_size_ = size;}

    uint64_t global_block_counters_blocks() const  {return global_block_counters_blocks_;}
    void set_global_block_counters_blocks(uint64_t blocks)  {global_block_counters_blocks_ = blocks;}


    SWMRStoreStatus status() const  {return store_status_;}

    bool is_clean() const  {return store_status_ == SWMRStoreStatus::CLEAN;}
    bool is_unclean() const  {return store_status_ == SWMRStoreStatus::UNCLEAN;}

    void set_clean_status()  {
        store_status_ = SWMRStoreStatus::CLEAN;
    }

    bool match_magick() const  {
        return magick1_ == MAGICK1 && magick2_ == MAGICK2;
    }

    bool match_profile_hash() const  {
        return profile_hash_ == PROFILE_HASH;
    }

    bool match_version() const  {
        return version_ == VERSION;
    }


    void init_metadata(LDDocumentView doc)
    {
        auto span = doc.span();

        if (span.length() <= METADATA_SIZE) {
            std::memcpy(metadata_, span.data(), span.length());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Initial superblock metadata size is too big: {}", span.length()).do_throw();
        }
    }

    void init(
            uint64_t superblock_file_pos,
            uint64_t file_size,
            const SnapshotID& commit_id,
            size_t superblock_size,
            SequenceID sequence_id,
            SequenceID cp_sequence_id,
            LDDocumentView meta
    )
    {
        magick1_ = MAGICK1;
        magick2_ = MAGICK2;
        profile_hash_ = PROFILE_HASH;
        version_ = VERSION;

        std::memset(magic_buffer_, 0, sizeof(magic_buffer_));

        sequence_id_ = sequence_id;
        consistency_point_sequence_id_ = cp_sequence_id;
        snapshot_id_   = commit_id;

        superblock_file_pos_ = superblock_file_pos;
        file_size_           = file_size;
        superblock_size_     = superblock_size;

        global_block_counters_file_pos_ = 0;
        global_block_counters_size_ = 0;
        global_block_counters_blocks_ = 0;

        history_root_id_   = BlockID{};
        directory_root_id_ = BlockID{};
        allocator_root_id_ = BlockID{};
        blockmap_root_id_  = BlockID{};

        store_status_ = SWMRStoreStatus::UNCLEAN;

        allocation_pool_data_.clear();


        init_metadata(meta);
    }

    void init_from(const SWMRSuperblock& other, uint64_t superblock_file_pos, const SnapshotID& commit_id, LDDocumentView meta)
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
        snapshot_id_   = commit_id;

        superblock_file_pos_ = superblock_file_pos;
        file_size_           = other.file_size_;
        superblock_size_     = other.superblock_size_;
        global_block_counters_file_pos_ = other.global_block_counters_file_pos_;
        global_block_counters_size_ = other.global_block_counters_size_;
        global_block_counters_blocks_ = other.global_block_counters_blocks_;

        store_status_        = other.store_status_;

        allocation_pool_data_ = other.allocation_pool_data_;


        init_metadata(meta);
    }

    LDDocumentView cmetadata_doc() const  {
        return LDDocumentView{metadata()};
    }

    LDDocumentView metadata_doc()  {
        return LDDocumentView{metadata()};
    }

    void set_metadata_doc(LDDocumentView view) {
        init_metadata(view);
    }

    void build_superblock_description()
    {
        return set_description(
            "MEMORIA SWMR MAPPED STORE. VERSION:{}; SnapshotID:{}, SeqID:{}, CPSeqID:{}, SBFilePos:{}, FileSize:{}, Status:{}, Counters:{}, CounterAt:{}, Profile:{}",
            version_, snapshot_id_, sequence_id_,
            consistency_point_sequence_id_, superblock_file_pos_,
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
    static int32_t allocator_block_size(size_t superblock_size)
    {
        int32_t sb_size = sizeof(SWMRSuperblock);
        int32_t alloc_size = sizeof(PackedAllocator);
        return superblock_size - (sb_size - alloc_size);
    }
};

}
