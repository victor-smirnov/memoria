
// Copyright 2020 Victor Smirnov
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
#include <memoria/store/swmr/mapped/swmr_mapped_store_readonly_commit.hpp>

#include <memoria/store/swmr/common/allocation_pool.hpp>


template <typename Profile>
class MappedSWMRStoreReadOnlyCommit;

namespace memoria {

struct InitStoreTag{};

template <typename Profile>
class MappedSWMRStoreWritableCommit:
        public MappedSWMRStoreCommitBase<Profile>,
        public ISWMRStoreWritableCommit<Profile>,
        public EnableSharedFromThis<MappedSWMRStoreWritableCommit<Profile>>
{
    using Base = MappedSWMRStoreCommitBase<Profile>;

    using typename Base::Store;
    using typename Base::CommitDescriptorT;
    using typename Base::CtrID;
    using typename Base::CtrReferenceableResult;
    using typename Base::AllocatorT;
    using typename Base::CommitID;
    using typename Base::BlockID;
    using typename Base::BlockG;
    using typename Base::BlockType;


    using typename Base::AllocationMapCtr;
    using typename Base::AllocationMapCtrType;

    using typename Base::RefcountersCtr;
    using typename Base::RefcountersCtrType;

    using typename Base::HistoryCtr;
    using typename Base::HistoryCtrType;

    using typename Base::DirectoryCtrType;
    using AllocationMetadataT = AllocationMetadata<Profile>;
    using Superblock          = SWMRSuperblock<Profile>;
    using ParentCommit        = SnpSharedPtr<MappedSWMRStoreReadOnlyCommit<Profile>>;

    struct CounterValue{
        int64_t value;
    };
    using CountersCache       = std::unordered_map<BlockID, CounterValue>;

    static constexpr int32_t BASIC_BLOCK_SIZE            = Store::BASIC_BLOCK_SIZE;
    static constexpr int32_t SUPERBLOCK_SIZE             = BASIC_BLOCK_SIZE;
    static constexpr int32_t ALLOCATION_LEVELS           = Store::ALLOCATION_MAP_LEVELS;
    static constexpr int32_t SUPERBLOCK_ALLOCATION_LEVEL = Log2(SUPERBLOCK_SIZE / BASIC_BLOCK_SIZE);
    static constexpr int32_t SUPERBLOCKS_RESERVED        = Store::HEADER_SIZE / BASIC_BLOCK_SIZE;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP    = Store::ALLOCATION_MAP_SIZE_STEP;


    CtrSharedPtr<AllocationMapCtr> parent_allocation_map_ctr_;
    CtrSharedPtr<AllocationMapCtr> allocation_map_ctr_;
    CtrSharedPtr<RefcountersCtr>   refcounters_ctr_;
    CtrSharedPtr<HistoryCtr>       history_ctr_;

    using Base::directory_ctr_;
    using Base::createCtrName;
    using Base::superblock_;
    using Base::getRootID;
    using Base::getBlock;
    using Base::instance_map_;
    using Base::buffer_;
    using Base::store_;


    AllocationPool<Profile, 9> allocation_pool_;

    bool committed_{false};
    bool allocator_initialization_mode_{false};
    bool refcounter_update_mode_{false};

    ParentCommit parent_commit_;



    CountersCache counters_cache_;

    ArenaBuffer<AllocationMetadataT> awaiting_allocations_;
    ArenaBuffer<AllocationMetadataT> postponed_deallocations_;

    bool persistent_{false};

public:

    MappedSWMRStoreWritableCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CommitDescriptorT* parent_commit_descriptor,
            CommitDescriptorT* commit_descriptor
    ) noexcept:
        Base(maybe_error, store, buffer, commit_descriptor)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            parent_commit_ = snp_make_shared<MappedSWMRStoreReadOnlyCommit<Profile>>(
                maybe_error, store, buffer, parent_commit_descriptor
            );

            MEMORIA_TRY(parent_allocation_map_ctr, parent_commit_->find(AllocationMapCtrID));
            parent_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(parent_allocation_map_ctr);
            return VoidResult::of();
        });

        internal_init_system_ctr<AllocationMapCtrType>(
            maybe_error,
            allocation_map_ctr_,
            superblock_->allocator_root_id(),
            AllocationMapCtrID
        );

        internal_init_system_ctr<RefcountersCtrType>(
            maybe_error,
            refcounters_ctr_,
            superblock_->counters_root_id(),
            RefcountersCtrID
        );

        internal_init_system_ctr<HistoryCtrType>(
            maybe_error,
            history_ctr_,
            superblock_->history_root_id(),
            HistoryCtrID
        );

        internal_init_system_ctr<DirectoryCtrType>(
            maybe_error,
            directory_ctr_,
            superblock_->directory_root_id(),
            HistoryCtrID
        );
    }

    MappedSWMRStoreWritableCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CommitDescriptorT* commit_descriptor,
            InitStoreTag
    ) noexcept:
        Base(maybe_error, store, buffer, commit_descriptor)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {            
            int64_t avaialble = (buffer_.size() / (BASIC_BLOCK_SIZE << (ALLOCATION_LEVELS - 1)));
            allocation_pool_.add(AllocationMetadataT{0, avaialble, ALLOCATION_LEVELS - 1});

            int64_t reserved = SUPERBLOCKS_RESERVED;
            ArenaBuffer<AllocationMetadataT> allocations;
            allocation_pool_.allocate(SUPERBLOCK_ALLOCATION_LEVEL, reserved, allocations);

            CommitID commit_id = 1;
            MEMORIA_TRY(superblock, allocate_superblock(nullptr, commit_id, buffer.size()));
            commit_descriptor->set_superblock(superblock);

            allocator_initialization_mode_ = true;

            auto ctr_ref = this->template internal_create_by_name_typed<AllocationMapCtrType>(AllocationMapCtrID);
            MEMORIA_RETURN_IF_ERROR(ctr_ref);
            allocation_map_ctr_ = ctr_ref.get();

            return VoidResult::of();
        });
    }

    VoidResult finish_store_initialization() noexcept
    {
        MEMORIA_TRY_VOID(allocation_map_ctr_->expand(buffer_.size() / BASIC_BLOCK_SIZE));
        return commit();
    }

    virtual CommitID commit_id() noexcept {
        return Base::commit_id();
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id) noexcept
    {
        MEMORIA_TRY_VOID(checkIfConainersCreationAllowed());
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(self_ptr(), ctr_id, decl);
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> create(const LDTypeDeclarationView& decl) noexcept
    {
        MEMORIA_TRY_VOID(checkIfConainersCreationAllowed());
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());

        MEMORIA_TRY(ctr_name, createCtrName());
        return factory->create_instance(self_ptr(), ctr_name, decl);
    }

    virtual VoidResult commit() noexcept
    {        
        if (this->is_active())
        {
            MEMORIA_TRY_VOID(flush_open_containers());

            if (history_ctr_)
            {
                int64_t sb_pos = superblock_->superblock_file_pos();
                MEMORIA_TRY_VOID(history_ctr_->assign_key(superblock_->commit_id(), sb_pos * (persistent_ ? 1 : -1)));
            }

            while (counters_cache_.size() > 0)
            {
                CountersCache counters_cache = std::move(counters_cache_);
                counters_cache_ = CountersCache{};

                for (auto entry: counters_cache)
                {
                    MEMORIA_TRY_VOID(internal_update_counters_entry(entry.first, entry.second.value));
                }
            }

            if (postponed_deallocations_.size() > 0)
            {
                MEMORIA_TRY_VOID(allocation_map_ctr_->setup_bits(postponed_deallocations_, false)); // clear bits
            }

            MEMORIA_TRY_VOID(store_->flush_data());

            MEMORIA_TRY_VOID(superblock_->build_superblock_description());

            size_t sb_slot = superblock_->sequence_id() % 2;
            std::memcpy(buffer_.data() + sb_slot * BASIC_BLOCK_SIZE, superblock_, BASIC_BLOCK_SIZE);

            MEMORIA_TRY_VOID(store_->flush_header());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Transaction {} has been already committed", commit_id());
        }

        return VoidResult::of();
    }

    virtual VoidResult flush_open_containers() noexcept {
        for (const auto& pair: instance_map_)
        {
            MEMORIA_TRY_VOID(pair.second->flush());
        }

        return VoidResult::of();
    }

    virtual BoolResult drop_ctr(const CtrID& name) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());

        MEMORIA_TRY(root_id, getRootID(name));

        if (root_id.is_set())
        {
            MEMORIA_TRY(block, getBlock(root_id));

            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(block->ctr_type_hash());

            MEMORIA_TRY_VOID(ctr_intf->drop(name, self_ptr()));
            return BoolResult::of(true);
        }
        else {
            return BoolResult::of(false);
        }
    }

    Result<CtrID> clone_ctr(const CtrID& ctr_name) noexcept {
        return clone_ctr(ctr_name, CtrID{});
    }

    Result<CtrID> clone_ctr(const CtrID& ctr_name, const CtrID& new_ctr_name) noexcept
    {
        MEMORIA_TRY(root_id, getRootID(ctr_name));
        MEMORIA_TRY(block, getBlock(root_id));

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->clone_ctr(ctr_name, new_ctr_name, self_ptr());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot {} ", ctr_name, superblock_->commit_uuid());
        }
    }

    virtual VoidResult rollback() noexcept {
        return VoidResult::of();
    }

    virtual VoidResult set_persistent(bool persistent) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());
        persistent_ = persistent;
        return VoidResult::of();
    }

    virtual bool is_persistent() noexcept {
        return persistent_;
    }


    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> find(const CtrID& ctr_id) noexcept {
        return Base::find(ctr_id);
    }

    virtual bool is_committed() const noexcept {
        return committed_;
    }

    virtual bool is_active() const noexcept {
        return !committed_;
    }

    virtual bool is_marked_to_clear() const noexcept {
        return Base::is_marked_to_clear();
    }


    virtual VoidResult dump_open_containers() noexcept {
        return Base::dump_open_containers();
    }

    virtual BoolResult has_open_containers() noexcept {
        return Base::has_open_containers();
    }

    virtual Result<std::vector<CtrID>> container_names() const noexcept {
        return Base::container_names();
    }

    virtual VoidResult drop() noexcept {
        return Base::drop();
    }

    virtual BoolResult check() noexcept {
        return Base::check();
    }

    virtual Result<Optional<U8String>> ctr_type_name_for(const CtrID& name) noexcept {
        return Base::ctr_type_name_for(name);
    }

    virtual VoidResult walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) noexcept {
        return Base::walk_containers(walker, allocator_descr);
    }

    virtual VoidResult setRoot(const CtrID& ctr_id, const BlockID& root) noexcept
    {
        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            if (!root.is_null())
            {
                MEMORIA_TRY(root_block, getBlock(root));
                MEMORIA_TRY_VOID(ref_block(root_block));

                auto prev_id = superblock_->directory_root_id();
                superblock_->directory_root_id() = root;
                if (prev_id.is_set())
                {
                    MEMORIA_TRY_VOID(unref_ctr_root(prev_id));
                }
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Containers directory removal attempted");
            }
        }
        else if (MMA_UNLIKELY(ctr_id == AllocationMapCtrID))
        {
            if (!root.is_null())
            {
                MEMORIA_TRY(root_block, getBlock(root));
                MEMORIA_TRY_VOID(ref_block(root_block));

                auto prev_id = superblock_->allocator_root_id();
                superblock_->allocator_root_id() = root;
                if (prev_id.is_set())
                {
                    MEMORIA_TRY_VOID(unref_ctr_root(prev_id));
                }
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("AllocationMap removal attempted");
            }
        }
        else if (MMA_UNLIKELY(ctr_id == RefcountersCtrID))
        {
            if (!root.is_null())
            {
                MEMORIA_TRY(root_block, getBlock(root));
                MEMORIA_TRY_VOID(ref_block(root_block));

                auto prev_id = superblock_->counters_root_id();
                superblock_->counters_root_id() = root;
                if (prev_id.is_set())
                {
                    MEMORIA_TRY_VOID(unref_ctr_root(prev_id));
                }
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Refcounters removal attempted");
            }
        }
        else if (MMA_UNLIKELY(ctr_id == HistoryCtrID))
        {
            if (!root.is_null())
            {
                MEMORIA_TRY(root_block, getBlock(root));
                MEMORIA_TRY_VOID(ref_block(root_block));

                auto prev_id = superblock_->history_root_id();
                superblock_->history_root_id() = root;
                if (prev_id.is_set())
                {
                    MEMORIA_TRY_VOID(unref_ctr_root(prev_id));
                }
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Commit history removal attempted");
            }
        }
        else {
            if (root.is_set())
            {
                MEMORIA_TRY(root_block, getBlock(root));
                MEMORIA_TRY_VOID(ref_block(root_block));

                MEMORIA_TRY(prev_id, directory_ctr_->replace_and_return(ctr_id, root));
                if (prev_id)
                {
                    MEMORIA_TRY_VOID(unref_ctr_root(prev_id.get()));
                }
            }
            else {
                MEMORIA_TRY(prev_id, directory_ctr_->remove_and_return(ctr_id));
                if (prev_id)
                {
                    MEMORIA_TRY_VOID(unref_ctr_root(prev_id.get()));
                }
            }
        }

        return VoidResult::of();
    }

    virtual VoidResult removeBlock(const BlockID& id) noexcept
    {
        MEMORIA_TRY(block, getBlock(id));

        int32_t block_size = block->memory_block_size();
        int32_t scale_factor = block_size / BASIC_BLOCK_SIZE;
        int32_t level = Log2(scale_factor);

        int64_t block_pos = block->id_value();
        int64_t allocator_block_pos = block_pos / BASIC_BLOCK_SIZE;
        int64_t ll_allocator_block_pos = allocator_block_pos >> level;

        MEMORIA_TRY(status, parent_allocation_map_ctr_->get_allocation_status(level, ll_allocator_block_pos));

        AllocationMetadataT meta[1] = {AllocationMetadataT{allocator_block_pos, 1, level}};
        if ((!status) || status.get() == AllocationMapEntryStatus::FREE)
        {
            MEMORIA_TRY_VOID(allocation_map_ctr_->setup_bits(meta, false)); // clear bits
        }
        else {
            postponed_deallocations_.append_value(meta[0]);
        }

        return VoidResult::of();
    }

    virtual Result<BlockG> createBlock(int32_t initial_size) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());
        using ResultT = Result<BlockG>;

        if (initial_size == -1)
        {
            initial_size = BASIC_BLOCK_SIZE;
        }

        int32_t scale_factor = initial_size / BASIC_BLOCK_SIZE;
        int32_t level = Log2(scale_factor);
        Optional<AllocationMetadataT> allocation = allocation_pool_.allocate_one(level);

        if (!allocation) {
            if (!allocator_initialization_mode_) {
                MEMORIA_TRY_VOID(populate_allocation_pool(level, 1, allocation_prefetch_size(level)));
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Internal block allocation error while initilizing the store");
            }
        }

        awaiting_allocations_.append_value(allocation.get());

        uint64_t id = allocation.get().level0_position();

        uint8_t* block_addr = buffer_.data() + id * BASIC_BLOCK_SIZE;

        std::memset(block_addr, 0, initial_size);

        BlockType* block = new (block_addr) BlockType(BlockID{id});
        block->init();

        block->id_value() = id;
        block->snapshot_id() = superblock_->commit_uuid();
        block->memory_block_size() = initial_size;

        return ResultT::of(BlockG{block});
    }

    virtual Result<BlockG> cloneBlock(const BlockG& block) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());
        using ResultT = Result<BlockG>;

        int32_t block_size = block->memory_block_size();
        int32_t scale_factor = block_size / BASIC_BLOCK_SIZE;
        int32_t level = Log2(scale_factor);

        Optional<AllocationMetadataT> allocation = allocation_pool_.allocate_one(level);

        if (!allocation) {
            if (!allocator_initialization_mode_) {
                MEMORIA_TRY_VOID(populate_allocation_pool(level, 1, allocation_prefetch_size(level)));
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Internal block allocation error while initilizing the store");
            }
        }

        awaiting_allocations_.append_value(allocation.get());

        uint64_t id = allocation.get().level0_position();

        uint8_t* block_addr = buffer_.data() + id * BASIC_BLOCK_SIZE;

        std::memcpy(block_addr, block.block(), block_size);

        BlockType* new_block = new (block_addr) BlockType();
        new_block->id() = BlockID{id};
        new_block->id_value() = id;
        new_block->snapshot_id() = superblock_->commit_uuid();

        return ResultT::of(BlockG{new_block});
    }

    virtual VoidResult ref_block(BlockG block, int64_t amount = 1) noexcept
    {
        BlockID block_id = block->id();

        if (MMA_UNLIKELY(!refcounters_ctr_)) {
            internal_ref_counter_cache(block_id, amount);
        }
        else if (MMA_LIKELY(!refcounter_update_mode_)) {
            return internal_ref_block(block_id, amount);
        }
        else {
            MEMORIA_TRY(success, internal_try_ref_block(block_id, amount));
            if (!success)
            {
                internal_ref_counter_cache(block_id, amount);
            }
        }

        return VoidResult::of();
    }

    virtual BoolResult unref_block(BlockG block) noexcept
    {
        BlockID block_id = block->id();
        auto cached = internal_unref_counter_cache(block_id);
        if (MMA_UNLIKELY((bool)cached)) {
            return BoolResult::of(cached.get());
        }
        else if (MMA_LIKELY(!refcounter_update_mode_))
        {
            return internal_unref_block(block_id, true);
        }
        else if (MMA_LIKELY((bool)refcounters_ctr_))
        {
            MEMORIA_TRY(zero_refs, internal_unref_block(block_id, false));
            if (zero_refs){
                counters_cache_[block_id] = CounterValue{0};
            }
            return zero_refs_result;
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Internal error. Counters container hasn't been initialized yet");
        }
    }

    virtual VoidResult unref_ctr_root(const BlockID& root_block_id) noexcept
    {
        MEMORIA_TRY(block, this->getBlock(root_block_id));
        MEMORIA_TRY(zero_references, unref_block(block));
        if (zero_references)
        {
            auto ctr_hash = block->ctr_type_hash();

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            MEMORIA_TRY(ctr, ctr_intf->new_ctr_instance(block, this));

            MEMORIA_TRY_VOID(ctr->internal_unref_cascade(root_block_id));
        }

        return VoidResult::of();
    }

protected:

    virtual Result<SnpSharedPtr<AllocatorT>> snapshot_ref_creation_allowed() noexcept {
        using ResultT = Result<SnpSharedPtr<AllocatorT>>;
        return ResultT::of();
    }


    virtual Result<SnpSharedPtr<AllocatorT>> snapshot_ref_opening_allowed() noexcept {
        return Base::snapshot_ref_opening_allowed();
    }

    virtual SnpSharedPtr<AllocatorT> self_ptr() noexcept {
        return this->shared_from_this();
    }

private:
    template <typename CtrName, typename CtrInstanceVar>
    void internal_init_system_ctr(
            MaybeError& maybe_error,
            CtrInstanceVar& assign_to,
            const BlockID& root_block_id,
            const CtrID& ctr_id
    ) noexcept
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            if (root_block_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<CtrName>(root_block_id);
                MEMORIA_RETURN_IF_ERROR(ctr_ref);
                assign_to = ctr_ref.get();
            }
            else {
                auto ctr_ref = this->template internal_create_by_name_typed<CtrName>(ctr_id);
                MEMORIA_RETURN_IF_ERROR(ctr_ref);
                assign_to = ctr_ref.get();
            }

            return VoidResult::of();
        });
    }

    VoidResult checkIfConainersCreationAllowed() noexcept {
        return VoidResult::of();
    }

    VoidResult check_updates_allowed() noexcept {
        return VoidResult::of();
    }

private:
    VoidResult populate_allocation_pool(int32_t level, int64_t minimal_amount, int64_t prefetch = 0) noexcept
    {
        MEMORIA_TRY_VOID(flush_awaiting_allocations());

        int64_t ranks[ALLOCATION_LEVELS] = {0,};
        MEMORIA_TRY_VOID(allocation_map_ctr_->unallocated(ranks));

        if (ranks[level] >= minimal_amount)
        {
            int64_t desireable = minimal_amount + prefetch;

            int32_t target_level = level;
            int32_t target_desirable = desireable;


            if (MMA_LIKELY(desireable > 1))
            {
                for (int32_t ll = ALLOCATION_LEVELS - 1; ll >= level; ll--)
                {
                    int64_t scale_factor = 1ll << (ll - level);
                    int64_t ll_desirable = divUp(desireable, scale_factor);
                    int64_t ll_required  = divUp(minimal_amount, scale_factor);

                    if (ranks[ll] >= ll_required && ll_desirable > 1)
                    {
                        target_level = ll;
                        target_desirable = ranks[ll] >= ll_desirable ? ll_desirable : ranks[ll];
                        break;
                    }
                }
            }

            return allocation_map_ctr_->find_unallocated(
                0, target_level, target_desirable, allocation_pool_.level_buffer(target_level)
            );
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "No enough free space among {}K blocks. Requested = {} blocks, available = {}",
                (BASIC_BLOCK_SIZE / 1024) << level,
                minimal_amount,
                ranks[level]
            );
        }
    }

    Result<Superblock*> allocate_superblock(const Superblock* parent_sb, int64_t commit_id, uint64_t file_size = 0) noexcept
    {
        using ResultT = Result<Superblock*>;

        ArenaBuffer<AllocationMetadataT> available;
        allocation_pool_.allocate(SUPERBLOCK_ALLOCATION_LEVEL, 1, available);
        if (available.size() > 0)
        {
            uint64_t pos = (available.tail().position() << SUPERBLOCK_ALLOCATION_LEVEL) * BASIC_BLOCK_SIZE;

            Superblock* superblock = new (buffer_.data() + pos) Superblock();
            if (parent_sb)
            {
                MEMORIA_TRY_VOID(superblock->init_from(*parent_sb, pos, commit_id));
            }
            else {
                MEMORIA_TRY_VOID(superblock->init(file_size, pos, commit_id, SUPERBLOCK_SIZE));
            }

            MEMORIA_TRY_VOID(superblock->build_superblock_description());

            return ResultT::of(superblock);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("No free space for Superblock");
        }
    }

    int32_t allocation_prefetch_size(int32_t level) noexcept
    {
        int32_t l0_prefetch_size = 256 * 4;
        return l0_prefetch_size >> level;
    }


    BoolResult internal_try_ref_block(const BlockID& block_id, int64_t amount = 1) noexcept
    {
        using DatumT = Datum<BigInt>;

        bool success = true;
        auto res = refcounters_ctr_->with_value(block_id.value(), [&](Optional<DatumT> value) noexcept {
            if (value) {
                auto new_value = value.get().view() + amount;
                return Optional<DatumT>{DatumT(new_value)};
            }
            else {
                success = false;
                return Optional<DatumT>{};
            }
        });
        MEMORIA_RETURN_IF_ERROR(res);

        return BoolResult::of(success);
    }

    VoidResult internal_ref_block(const BlockID& block_id, int64_t amount = 1) noexcept
    {
        refcounter_update_mode_ = true;
        using DatumT = Datum<BigInt>;

        auto res = refcounters_ctr_->with_value(block_id.value(), [&](Optional<DatumT> value) noexcept {
            if (value) {
                auto new_value = value.get().view() + amount;
                return Optional<DatumT>{DatumT(new_value)};
            }
            else {
                return Optional<DatumT>{DatumT(1)};
            }
        });

        refcounter_update_mode_ = false;

        MEMORIA_RETURN_IF_ERROR(res);
        return VoidResult::of();
    }

    BoolResult internal_unref_block(const BlockID& block_id, bool delete_on_zero) noexcept
    {
        refcounter_update_mode_ = true;

        bool no_counter = false;
        int64_t counter_value{};

        using DatumT = Datum<BigInt>;
        auto res = refcounters_ctr_->with_value(block_id.value(), [&](Optional<DatumT> value) noexcept {
            if (value)
            {
                counter_value = value.get().view() - 1;

                if (counter_value == 0 && delete_on_zero) {
                    return Optional<DatumT>{};
                }
                else {
                    return Optional<DatumT>{DatumT(counter_value)};
                }
            }
            else {
                no_counter = true;
                return Optional<DatumT>{};
            }
        });
        refcounter_update_mode_ = false;
        MEMORIA_RETURN_IF_ERROR(res);

        if (no_counter) {
            return MEMORIA_MAKE_GENERIC_ERROR("Internal error. No counter for block ID {}", block_id);
        }

        if (counter_value < 0) {
            return MEMORIA_MAKE_GENERIC_ERROR("Internal error. Negative counter for block ID {}: {}", block_id, counter_value);
        }

        return BoolResult::of(counter_value == 0);
    }





    void internal_ref_counter_cache(const BlockID& block_id, int64_t amount) noexcept
    {
        auto ii = counters_cache_.find(block_id);
        if (ii != counters_cache_.end()) {
            ii->second.value += amount;
        }
        else {
            counters_cache_[block_id] = CounterValue{amount};
        }
    }

    Optional<bool> internal_unref_counter_cache(const BlockID& block_id) noexcept
    {
        auto ii = counters_cache_.find(block_id);
        if (ii != counters_cache_.end()) {
            auto res = ii->second.value - 1;
            ii->second.value = res;
            return Optional<bool>{res == 0};
        }
        else {
            return Optional<bool>{};
        }
    }

    VoidResult internal_update_counters_entry(const BlockID& block_id, int64_t refs_value) noexcept
    {
        if (refs_value > 0)
        {
            MEMORIA_TRY_VOID(refcounters_ctr_->assign_key(block_id.value(), refs_value));
        }
        else if (refs_value == 0) {
            MEMORIA_TRY_VOID(refcounters_ctr_->remove_key(block_id.value()));
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Internal error. Negtive refcount for block {}", block_id);
        }

        return VoidResult::of();
    }

    VoidResult flush_awaiting_allocations() noexcept
    {
        if (awaiting_allocations_.size() > 0 && allocation_map_ctr_)
        {
            awaiting_allocations_.sort();
            MEMORIA_TRY_VOID(allocation_map_ctr_->setup_bits(awaiting_allocations_.span(), true)); // set bits
            awaiting_allocations_.clear();
        }

        return VoidResult::of();
    }
};

}
