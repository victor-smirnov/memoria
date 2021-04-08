
// Copyright 2021 Victor Smirnov
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
#include <memoria/core/strings/format.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>

namespace memoria {

template <typename Profile>
class LMDBSuperblock {
public:
    using BlockID = ProfileBlockID<Profile>;

private:    
    BlockID directory_root_id_;
    uint64_t superblock_size_;
    uint64_t updates_;
    uint64_t block_id_counter_;

    PackedAllocator allocator_;

public:
    LMDBSuperblock() noexcept {}

    BlockID& directory_root_id() noexcept {return directory_root_id_;}
    uint64_t superblock_size() const noexcept {return superblock_size_;}

    VoidResult init(uint64_t superblock_size)
    {
        directory_root_id_ = BlockID{};
        superblock_size_ = superblock_size;

        updates_ = 1;
        block_id_counter_ = 1000;

        return allocator_.init(allocator_block_size(superblock_size), 1);
    }

    bool is_updated() const noexcept {
        return updates_ > 0;
    }

    void clear_updates() noexcept {
        updates_ = 0;
    }

    void touch() noexcept {
        ++updates_;
    }

    uint64_t new_block_id() noexcept {
        ++block_id_counter_;
        ++updates_;
        return block_id_counter_;
    }

private:
    static int32_t allocator_block_size(size_t superblock_size) noexcept
    {
        int32_t sb_size = sizeof(LMDBSuperblock);
        int32_t alloc_size = sizeof(PackedAllocator);
        return superblock_size - (sb_size - alloc_size);
    }
};

}
