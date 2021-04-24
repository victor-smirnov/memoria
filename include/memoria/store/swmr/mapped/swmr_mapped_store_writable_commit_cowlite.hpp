
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

#include <memoria/profiles/impl/cow_lite_profile.hpp>

#include <boost/pool/object_pool.hpp>

#include <type_traits>


namespace memoria {

template <typename>
class MappedSWMRStoreReadOnlyCommit;


template <typename>
class MappedSWMRStoreWritableCommit;

template <typename ChildProfile>
class MappedSWMRStoreWritableCommit<CowLiteProfile<ChildProfile>>:
        public SWMRStoreWritableCommitBase<CowLiteProfile<ChildProfile>>,
        public EnableSharedFromThis<MappedSWMRStoreWritableCommit<CowLiteProfile<ChildProfile>>>
{
    using Profile = CowLiteProfile<ChildProfile>;

    using Base = SWMRStoreWritableCommitBase<CowLiteProfile<ChildProfile>>;

    using typename Base::Store;
    using typename Base::CommitDescriptorT;

    using typename Base::StoreT;
    using typename Base::CommitID;
    using typename Base::BlockID;
    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockType;
    using typename Base::ApiProfileT;
    using typename Base::Superblock;

    using typename Base::AllocationMapCtr;
    using typename Base::AllocationMapCtrType;

    using typename Base::HistoryCtr;
    using typename Base::HistoryCtrType;
    using typename Base::CounterStorageT;

    using typename Base::DirectoryCtrType;
    using typename Base::Shared;

    using CtrID = ProfileCtrID<Profile>;
    using CtrReferenceableResult = Result<CtrReferenceable<ApiProfile<Profile>>>;

    using Base::BASIC_BLOCK_SIZE;
    using Base::superblock_;
    using Base::store_;
    using Base::committed_;
    using Base::newId;

    Span<uint8_t> buffer_;


    mutable boost::object_pool<Shared> shared_pool_;


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
    {}


    virtual ~MappedSWMRStoreWritableCommit() noexcept {
        if (!committed_) {
            store_->unlock_writer();
            delete Base::commit_descriptor_;
        }
    }

    virtual void store_superblock(Superblock* superblock, uint64_t sb_slot) {
        std::memcpy(buffer_.data() + sb_slot * BASIC_BLOCK_SIZE, superblock_, BASIC_BLOCK_SIZE);
    }

    virtual SnpSharedPtr<StoreT> self_ptr() noexcept {
        return this->shared_from_this();
    }

    virtual uint64_t get_memory_size() noexcept {
        return buffer_.size();
    }


    virtual Superblock* newSuperblock(uint64_t pos) {
        return new (buffer_.data() + pos) Superblock();
    }


    using typename Base::ResolvedBlock;

    virtual ResolvedBlock resolve_block(const BlockID& block_id)
    {
        BlockType* block = ptr_cast<BlockType>(buffer_.data() + block_id.value() * BASIC_BLOCK_SIZE);
        Shared* shared = shared_pool_.construct(block_id, block, 0);
        shared->set_allocator(this);
        return {block_id.value(), SharedBlockConstPtr{shared}};
    }


    virtual Shared* allocate_block(uint64_t at, size_t size, bool for_idmap)
    {
        BlockID id{at};

        uint8_t* block_addr = buffer_.data() + at * BASIC_BLOCK_SIZE;

        std::memset(block_addr, 0, size);

        BlockType* block = new (block_addr) BlockType(id, at, at);

        block->memory_block_size() = size;

        Shared* shared = shared_pool_.construct(id, block, 0);
        shared->set_allocator(this);

        return shared;
    }

    virtual Shared* allocate_block_from(const BlockType* source, uint64_t at, bool for_idmap) {
        uint8_t* block_addr = buffer_.data() + at * BASIC_BLOCK_SIZE;

        std::memcpy(block_addr, source, source->memory_block_size());

        auto id = BlockID{at};
        BlockType* new_block = ptr_cast<BlockType>(block_addr);
        new_block->id()   = id;
        new_block->uuid() = at;
        new_block->id_value() = at;

        new_block->set_references(0);

        Shared* shared = shared_pool_.construct(id, new_block, 0);
        shared->set_allocator(this);

        return shared;
    }


    virtual void updateBlock(Shared* block) {
    }

    virtual void releaseBlock(Shared* block) noexcept {
        shared_pool_.destroy(block);
    }
};

}
