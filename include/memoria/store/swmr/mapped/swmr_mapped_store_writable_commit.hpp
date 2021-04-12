
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

#include <memoria/store/swmr/mapped/swmr_mapped_store_common.hpp>
#include <memoria/store/swmr/common/swmr_store_writable_commit_base.hpp>
#include <memoria/store/swmr/common/allocation_pool.hpp>

#include <memoria/core/datatypes/type_registry.hpp>

#include <type_traits>

template <typename Profile>
class MappedSWMRStoreReadOnlyCommit;

namespace memoria {

template <typename Profile>
class MappedSWMRStoreWritableCommit:
        public SWMRStoreWritableCommitBase<Profile>,
        public EnableSharedFromThis<MappedSWMRStoreWritableCommit<Profile>>
{
    using Base = SWMRStoreWritableCommitBase<Profile>;

    using typename Base::Store;
    using typename Base::CommitDescriptorT;

    using typename Base::AllocatorT;
    using typename Base::CommitID;
    using typename Base::BlockID;
    using typename Base::BlockG;
    using typename Base::BlockType;
    using typename Base::ApiProfileT;
    using typename Base::Superblock;

    using typename Base::AllocationMapCtr;
    using typename Base::AllocationMapCtrType;

    using typename Base::HistoryCtr;
    using typename Base::HistoryCtrType;
    using typename Base::CounterStorageT;

    using typename Base::DirectoryCtrType;

    using CtrID = ProfileCtrID<Profile>;
    using CtrReferenceableResult = Result<CtrReferenceable<ApiProfile<Profile>>>;

    using Base::BASIC_BLOCK_SIZE;
    using Base::superblock_;
    using Base::store_;
    using Base::committed_;

    Span<uint8_t> buffer_;

public:
    using Base::check;

    MappedSWMRStoreWritableCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CommitDescriptorT* commit_descriptor
    ) noexcept:
        Base(store, commit_descriptor, store.get()),
        buffer_(buffer)
    {
    }


    virtual ~MappedSWMRStoreWritableCommit() noexcept {
        if (!committed_) {
            store_->unlock_writer();
            delete Base::commit_descriptor_;
        }
    }

    virtual VoidResult store_superblock(Superblock* superblock, uint64_t sb_slot) noexcept {
        std::memcpy(buffer_.data() + sb_slot * BASIC_BLOCK_SIZE, superblock_, BASIC_BLOCK_SIZE);
        return VoidResult::of();
    }

    virtual SnpSharedPtr<AllocatorT> self_ptr() noexcept {
        return this->shared_from_this();
    }

    virtual uint64_t get_memory_size() noexcept {
        return buffer_.size();
    }


    virtual Superblock* newSuperblock(uint64_t pos) noexcept {
        return new (buffer_.data() + pos) Superblock();
    }


    virtual Result<BlockG> getBlock(const BlockID& id) noexcept
    {
        using ResultT = Result<BlockG>;
        BlockType* block = ptr_cast<BlockType>(buffer_.data() + id.value() * BASIC_BLOCK_SIZE);
        return ResultT::of(BlockG{block});
    }

    virtual Result<uint8_t*> allocate_block(uint64_t at, size_t size) noexcept {
        uint8_t* block_addr = buffer_.data() + at * BASIC_BLOCK_SIZE;
        return Result<uint8_t*>::of(block_addr);
    }
};

}
