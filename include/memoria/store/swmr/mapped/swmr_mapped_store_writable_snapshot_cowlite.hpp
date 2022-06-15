
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
#include <memoria/store/swmr/common/swmr_store_writable_snapshot_base.hpp>
#include <memoria/store/swmr/common/allocation_pool.hpp>

#include <memoria/core/datatypes/type_registry.hpp>

#include <memoria/profiles/impl/cow_lite_profile.hpp>

#include <boost/pool/object_pool.hpp>

#include <type_traits>


namespace memoria {

template <typename>
class MappedSWMRStoreReadOnlySnapshot;


template <typename>
class MappedSWMRStoreWritableSnapshot;

template <typename ChildProfile>
class MappedSWMRStoreWritableSnapshot<CowLiteProfileT<ChildProfile>>:
        public SWMRStoreWritableSnapshotBase<CowLiteProfileT<ChildProfile>>,
        public EnableSharedFromThis<MappedSWMRStoreWritableSnapshot<CowLiteProfileT<ChildProfile>>>
{
    using Profile = CowLiteProfileT<ChildProfile>;

    using Base = SWMRStoreWritableSnapshotBase<CowLiteProfileT<ChildProfile>>;

    using typename Base::Store;
    using typename Base::CDescrPtr;

    using typename Base::StoreT;
    using typename Base::SnapshotID;
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
    using typename Base::CounterBlockT;

    using typename Base::DirectoryCtrType;
    using typename Base::Shared;
    using typename Base::RemovingBlocksConsumerFn;
    using typename Base::AllocationMetadataT;

    using CtrID = ProfileCtrID<Profile>;
    using CtrReferenceableResult = Result<CtrReferenceable<ApiProfile<Profile>>>;

    using typename Base::State;
    using Base::BASIC_BLOCK_SIZE;
    using Base::store_;
    using Base::state_;
    using Base::newId;

    Span<uint8_t> buffer_;

    mutable boost::object_pool<Shared> shared_pool_;
    mutable boost::object_pool<detail::MMapSBPtrPooledSharedImpl> sb_shared_pool_;

public:
    using Base::check;
    using Base::snapshot_id;
    using Base::CustomLog2;

    MappedSWMRStoreWritableSnapshot(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CDescrPtr& snapshot_descriptor,
            RemovingBlocksConsumerFn removing_blocks_consumer_fn = RemovingBlocksConsumerFn{}
    ) :
        Base(store, snapshot_descriptor, store.get(), removing_blocks_consumer_fn),
        buffer_(buffer)
    {}

    SnpSharedPtr<StoreT> my_self_ptr()  override {
        return this->shared_from_this();
    }

    virtual uint64_t get_memory_size()  override {
        return buffer_.size();
    }


    virtual SharedSBPtr<Superblock> new_superblock(uint64_t pos) override {
        Superblock* sb = new (buffer_.data() + pos) Superblock();
        return SharedSBPtr(sb, sb_shared_pool_.construct(&sb_shared_pool_));
    }


    using typename Base::ResolvedBlock;

    virtual ResolvedBlock resolve_block(const BlockID& block_id) override
    {
        if (block_id)
        {
            auto vv = block_id.value().value() * BASIC_BLOCK_SIZE;

            BlockType* block = ptr_cast<BlockType>(buffer_.data() + vv );
            Shared* shared = shared_pool_.construct(block_id, block, 0);
            shared->set_allocator(this);
            shared->set_mutable(block->snapshot_id() == snapshot_id());

            return {block_id.value().value() * BASIC_BLOCK_SIZE, SharedBlockConstPtr{shared}};
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("BlockId is null").do_throw();
        }
    }

    AllocationMetadataT resolve_block_allocation(const BlockID& block_id) override
    {
        int32_t level = block_id.value().metadata();
        int64_t id_value = static_cast<int64_t>(block_id.value().value());


        return AllocationMetadataT(id_value, 1, level);
    }


    virtual Shared* allocate_block(uint64_t at, size_t size, bool for_idmap) override
    {
        int32_t scale_factor = size / BASIC_BLOCK_SIZE;
        uint64_t level = CustomLog2(scale_factor);

        UID64 bid{at, level};
        BlockID id{bid};
        uint8_t* block_addr = buffer_.data() + at * BASIC_BLOCK_SIZE;

        std::memset(block_addr, 0, size);

        BlockType* block = new (block_addr) BlockType(id);

        block->memory_block_size() = size;
        block->snapshot_id() = snapshot_id();

        Shared* shared = shared_pool_.construct(id, block, 0);
        shared->set_allocator(this);
        shared->set_mutable(true);

        return shared;
    }

    virtual Shared* allocate_block_from(const BlockType* source, uint64_t at, bool for_idmap) override
    {
        uint8_t* block_addr = buffer_.data() + at * BASIC_BLOCK_SIZE;
        std::memcpy(block_addr, source, source->memory_block_size());

        int32_t size = source->memory_block_size();
        int32_t scale_factor = size / BASIC_BLOCK_SIZE;
        uint64_t level = CustomLog2(scale_factor);

        UID64 bid{at, level};
        auto id = BlockID{bid};
        BlockType* new_block = ptr_cast<BlockType>(block_addr);
        new_block->id() = id;
        new_block->snapshot_id() = snapshot_id();

        new_block->set_references(0);

        Shared* shared = shared_pool_.construct(id, new_block, 0);
        shared->set_allocator(this);
        shared->set_mutable(true);

        return shared;
    }


    virtual void updateBlock(Shared* block) override {
    }

    virtual void releaseBlock(Shared* block) noexcept override {
        shared_pool_.destroy(block);
    }

    virtual SharedSBPtr<Superblock> get_superblock(uint64_t pos) override {
        Superblock* sb = ptr_cast<Superblock>(buffer_.data() + pos);
        return SharedSBPtr(sb, sb_shared_pool_.construct(&sb_shared_pool_));
    }

    virtual AllocationMetadataT get_allocation_metadata(const BlockID& block_id) override {        
        return AllocationMetadataT(
            (int64_t)block_id.value().value(),
            1,
            (int32_t)block_id.value().metadata()
        );
    }

    void check_storage_specific(
            SharedBlockConstPtr block,
            const CheckResultConsumerFn& consumer
    ) override
    {
        int32_t block_size = block->memory_block_size();
        int32_t expected_block_size = (1 << block->id().value().metadata()) * BASIC_BLOCK_SIZE;

        if (block_size != expected_block_size) {
            consumer(CheckSeverity::ERROR, make_string_document("Block size mismatch for block {}. Expected {}, actual {}", expected_block_size, block_size));
        }
    }

};

}
