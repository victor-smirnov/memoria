
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
#include <memoria/core/tools/simple_2q_cache.hpp>
#include <memoria/core/tools/uid_64.hpp>

#include <memoria/profiles/impl/cow_profile.hpp>

#include <boost/pool/object_pool.hpp>

#include <type_traits>



namespace memoria {

template <typename>
class MappedSWMRStoreReadOnlySnapshot;

template <typename>
class MappedSWMRStoreWritableSnapshot;

template <typename ChildProfile>
class MappedSWMRStoreWritableSnapshot<CowProfileT<ChildProfile>>:
        public SWMRStoreWritableSnapshotBase<CowProfileT<ChildProfile>>,
        public EnableSharedFromThis<MappedSWMRStoreWritableSnapshot<CowProfileT<ChildProfile>>>
{
    using Profile = CowProfileT<ChildProfile>;

    using Base = SWMRStoreWritableSnapshotBase<Profile>;

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

    using BlockIDValueHolder = typename BlockID::ValueHolder;

    using BlockMapCtrType    = Map<BlockIDValueHolder, UID64>;
    using BlockMapCtr        = ICtrApi<BlockMapCtrType, ApiProfileT>;

    class CacheEntryBase: public Shared {
        uint64_t file_pos_;
    public:
        CacheEntryBase(const BlockID& id, BlockType* block, uint64_t file_pos)  :
            Shared(id, block, 0), file_pos_(file_pos)
        {}

        uint64_t file_pos() const  {return file_pos_;}
    };

    using SharedBlockCache = SimpleTwoQueueCache<BlockID, CacheEntryBase>;
    using BlockCacheEntry = typename SharedBlockCache::EntryT;

    using Base::BASIC_BLOCK_SIZE;
    using Base::store_;
    using Base::state_;
    using Base::snapshot_descriptor_;

    Span<uint8_t> buffer_;

    CtrSharedPtr<BlockMapCtr> blockmap_ctr_;

    mutable boost::object_pool<BlockCacheEntry> cache_entry_pool_;
    mutable SharedBlockCache block_cache_;
    mutable boost::object_pool<detail::MMapSBPtrPooledSharedImpl> sb_shared_pool_;

public:
    using Base::check;
    using Base::init_snapshot;
    using Base::init_store_snapshot;
    using Base::newId;
    using Base::unref_ctr_root;
    using Base::get_superblock;
    using Base::snapshot_id;
    using Base::CustomLog2;
    using Base::allocation_level;

    MappedSWMRStoreWritableSnapshot(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CDescrPtr& snapshot_descriptor,
            RemovingBlocksConsumerFn removing_blocks_consumer_fn = RemovingBlocksConsumerFn{}
    ) :
        Base(store, snapshot_descriptor, store.get(), removing_blocks_consumer_fn),
        buffer_(buffer),
        block_cache_(1024*128)
    {}

    void init_idmap() override
    {
        auto sb = get_superblock();

        MaybeError maybe_error{};
        this->template internal_init_system_ctr<BlockMapCtrType>(
            maybe_error,
            blockmap_ctr_,
            sb->blockmap_root_id(),
            BlockMapCtrID
        );

        if (maybe_error) {
            std::move(maybe_error.get()).do_throw();
        }
    }

    void open_idmap() override
    {
        auto blockmap_root_id = get_superblock()->blockmap_root_id();
        if (blockmap_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<BlockMapCtrType>(blockmap_root_id);

            blockmap_ctr_ = ctr_ref;
            blockmap_ctr_->internal_reset_allocator_holder();
        }
    }

    void drop_idmap() override
    {
        auto sb = get_superblock();
        if (sb->blockmap_root_id().is_set()) {
            unref_ctr_root(sb->blockmap_root_id());
        }
    }

    virtual BlockID newId() override {
        return ProfileTraits<Profile>::make_random_block_id();
    }

    virtual SnpSharedPtr<StoreT> my_self_ptr()  override {
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
        auto existing_entry = block_cache_.get(block_id);
        if (existing_entry)
        {
            BlockCacheEntry* shared = existing_entry.get();
            return {shared->file_pos() * BASIC_BLOCK_SIZE, SharedBlockConstPtr{shared}};
        }
        else {
            uint64_t at;

            if (MMA_UNLIKELY(block_id.value().is_type3()))
            {
                at = block_id.value().counter();
            }
            else {
                auto ii = blockmap_ctr_->find(block_id.value());
                if (ii->is_found(block_id.value())) {
                    at = ii->value().view().value();
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Can't find block ID {} in the BlockMap", block_id).do_throw();
                }
            }

            BlockType* block = ptr_cast<BlockType>(buffer_.data() + at * BASIC_BLOCK_SIZE);
            BlockCacheEntry* shared = cache_entry_pool_.construct(block_id, block, at);
            shared->set_allocator(this);

            shared->set_mutable(block->snapshot_id() == snapshot_id());

            block_cache_.insert(shared);

            return {at * BASIC_BLOCK_SIZE, SharedBlockConstPtr{shared}};
        }
    }

    AllocationMetadataT resolve_block_allocation(const BlockID& block_id) override
    {
        auto existing_entry = block_cache_.get(block_id);
        if (existing_entry)
        {
            BlockCacheEntry* shared = existing_entry.get();
            return AllocationMetadataT(
                static_cast<int64_t>(shared->file_pos()),
                1,
                static_cast<int32_t>(allocation_level(shared->ptr()->memory_block_size()))
            );
        }
        else {
            uint64_t at;
            int64_t level;

            if (MMA_UNLIKELY(block_id.value().is_type3()))
            {
                at = block_id.value().counter();
                level = block_id.value().metadata();
            }
            else {
                auto ii = blockmap_ctr_->find(block_id.value());
                if (ii->is_found(block_id.value()))
                {
                    at = ii->value().view().value();
                    level = ii->value().view().metadata();
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Can't find block ID {} in the BlockMap", block_id).do_throw();
                }
            }

            return AllocationMetadataT(static_cast<int64_t>(at), 1, static_cast<int32_t>(level));
        }
    }



    virtual Shared* allocate_block(uint64_t at, size_t size, bool for_idmap) override
    {
        uint8_t* block_addr = buffer_.data() + at * BASIC_BLOCK_SIZE;

        std::memset(block_addr, 0, size);

        BlockID id;

        if (!for_idmap) {
            id = newId();
            blockmap_ctr_->assign_key(id.value(), UID64{at, static_cast<uint64_t>(allocation_level(size))});
        }
        else {
            id = BlockID{UID256::make_type3(UID256{}, static_cast<uint64_t>(allocation_level(size)), at)};
        }

        BlockType* block = new (block_addr) BlockType(id, id.value(), id.value());

        block->memory_block_size() = size;
        block->snapshot_id() = snapshot_id();

        BlockCacheEntry* shared = cache_entry_pool_.construct(id, block, at);
        shared->set_allocator(this);
        shared->set_mutable(true);

        block_cache_.insert(shared);

        return shared;
    }

    virtual Shared* allocate_block_from(const BlockType* source, uint64_t at, bool for_idmap) override
    {
        uint8_t* block_addr = buffer_.data() + at * BASIC_BLOCK_SIZE;

        size_t block_size = source->memory_block_size();

        std::memcpy(block_addr, source, block_size);

        BlockID id;

        if (!for_idmap) {
            id = newId();
            blockmap_ctr_->assign_key(id.value(), UID64{at, static_cast<uint64_t>(allocation_level(block_size))});
        }
        else {
            id = BlockID{UID256::make_type3(UID256{}, static_cast<uint64_t>(allocation_level(block_size)), at)};
        }

        BlockType* new_block = ptr_cast<BlockType>(block_addr);
        new_block->id()   = id;
        new_block->uuid() = id.value();
        new_block->id_value() = id.value();
        new_block->snapshot_id() = snapshot_id();

        new_block->set_references(0);

        BlockCacheEntry* shared = cache_entry_pool_.construct(id, new_block, at);
        shared->set_allocator(this);
        shared->set_mutable(true);

        block_cache_.insert(shared);

        return shared;
    }


    virtual void updateBlock(Shared* block) override {
    }

    virtual void releaseBlock(Shared* block) noexcept override {
        block_cache_.attach(static_cast<BlockCacheEntry*>(block), [&](BlockCacheEntry* entry){
            cache_entry_pool_.destroy(entry);
        });
    }

    virtual SharedSBPtr<Superblock> get_superblock(uint64_t pos) override {
        Superblock* sb = ptr_cast<Superblock>(buffer_.data() + pos);
        return SharedSBPtr(sb, sb_shared_pool_.construct(&sb_shared_pool_));
    }

    virtual AllocationMetadataT get_allocation_metadata(const BlockID& block_id) override {
        return AllocationMetadataT{block_id.value(), 1, 0};
    }

    void check_storage_specific(
            SharedBlockConstPtr block,
            const CheckResultConsumerFn& consumer
            ) override {

    }
};

}
