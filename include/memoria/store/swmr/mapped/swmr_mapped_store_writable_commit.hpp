
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

#include <memoria/core/datatypes/type_registry.hpp>

#include <type_traits>

template <typename Profile>
class MappedSWMRStoreReadOnlyCommit;

namespace memoria {

struct InitStoreTag{};

template <typename Profile>
class MappedSWMRStoreWritableCommit:
        public MappedSWMRStoreCommitBase<Profile>,
        public ISWMRStoreWritableCommit<ApiProfile<Profile>>,
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
    using typename Base::ApiProfileT;


    using typename Base::AllocationMapCtr;
    using typename Base::AllocationMapCtrType;

    using typename Base::HistoryCtr;
    using typename Base::HistoryCtrType;

    using typename Base::DirectoryCtrType;
    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;
    using Superblock          = SWMRSuperblock<Profile>;
    using ParentCommit        = SnpSharedPtr<MappedSWMRStoreReadOnlyCommit<Profile>>;

    using CounterStorageT     = typename Store::CounterStorageT;

    using BlockCleanupHandler = std::function<VoidResult ()>;

    static constexpr int32_t BASIC_BLOCK_SIZE            = Store::BASIC_BLOCK_SIZE;
    static constexpr int32_t SUPERBLOCK_SIZE             = BASIC_BLOCK_SIZE;
    static constexpr int32_t ALLOCATION_LEVELS           = Store::ALLOCATION_MAP_LEVELS;
    static constexpr int32_t SUPERBLOCK_ALLOCATION_LEVEL = Log2(SUPERBLOCK_SIZE / BASIC_BLOCK_SIZE) - 1;
    static constexpr int32_t SUPERBLOCKS_RESERVED        = Store::HEADER_SIZE / BASIC_BLOCK_SIZE;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP    = Store::ALLOCATION_MAP_SIZE_STEP;


    CtrSharedPtr<AllocationMapCtr> parent_allocation_map_ctr_;

    using Base::directory_ctr_;
    using Base::allocation_map_ctr_;
    using Base::history_ctr_;
    using Base::createCtrName;
    using Base::superblock_;
    using Base::getRootID;
    using Base::getBlock;
    using Base::instance_map_;
    using Base::buffer_;
    using Base::store_;
    using Base::commit_descriptor_;
    using Base::refcounter_delegate_;


    AllocationPool<ApiProfileT, 9> allocation_pool_;

    bool committed_{false};
    bool allocator_initialization_mode_{false};

    ParentCommit parent_commit_;

    bool flushing_awaiting_allocations_{false};
    ArenaBuffer<AllocationMetadataT> awaiting_allocations_;
    ArenaBuffer<AllocationMetadataT> awaiting_allocations_reentry_;

    ArenaBuffer<AllocationMetadataT> postponed_deallocations_;

    template <typename>
    friend class MappedSWMRStore;

public:
    using Base::check;

    MappedSWMRStoreWritableCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CommitDescriptorT* parent_commit_descriptor,
            CommitDescriptorT* commit_descriptor
    ) noexcept:
        Base(maybe_error, store, buffer, commit_descriptor, store.get())
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            parent_commit_ = snp_make_shared<MappedSWMRStoreReadOnlyCommit<Profile>>(
                maybe_error, store, buffer, parent_commit_descriptor
            );

            MEMORIA_TRY(parent_allocation_map_ctr, parent_commit_->find(AllocationMapCtrID));
            parent_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(parent_allocation_map_ctr);

            MEMORIA_TRY_VOID(populate_allocation_pool(parent_allocation_map_ctr_, SUPERBLOCK_ALLOCATION_LEVEL, 1, 64));

            CommitID parent_commit_id = parent_commit_descriptor->superblock()->commit_id();

            MEMORIA_TRY(
                superblock,
                allocate_superblock(
                    parent_commit_descriptor->superblock(),
                    parent_commit_id + 1
                )
            );

            commit_descriptor->set_superblock(superblock);
            superblock_ = superblock;

            return VoidResult::of();
        });

        if (!maybe_error) {
            internal_init_system_ctr<AllocationMapCtrType>(
                maybe_error,
                allocation_map_ctr_,
                superblock_->allocator_root_id(),
                AllocationMapCtrID
            );
        }

        if (!maybe_error) {
            internal_init_system_ctr<HistoryCtrType>(
                maybe_error,
                history_ctr_,
                superblock_->history_root_id(),
                HistoryCtrID
            );
        }

        if (!maybe_error) {
            internal_init_system_ctr<DirectoryCtrType>(
                maybe_error,
                directory_ctr_,
                superblock_->directory_root_id(),
                DirectoryCtrID
            );
        }
    }

    MappedSWMRStoreWritableCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CommitDescriptorT* commit_descriptor,
            InitStoreTag
    ) noexcept:
        Base(maybe_error, store, buffer, commit_descriptor, store.get())
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            int64_t avaialble = (buffer_.size() / (BASIC_BLOCK_SIZE << (ALLOCATION_LEVELS - 1)));
            allocation_pool_.add(AllocationMetadataT{0, avaialble, ALLOCATION_LEVELS - 1});

            int64_t reserved = SUPERBLOCKS_RESERVED;
            ArenaBuffer<AllocationMetadataT> allocations;
            allocation_pool_.allocate(SUPERBLOCK_ALLOCATION_LEVEL, reserved, allocations);

            for (auto& alc: allocations.span()) {
                add_awaiting_allocation(alc);
            }

            CommitID commit_id = 1;
            MEMORIA_TRY(superblock, allocate_superblock(nullptr, commit_id, buffer.size()));
            commit_descriptor->set_superblock(superblock);

            uint64_t total_blocks = buffer_.size() / BASIC_BLOCK_SIZE;
            uint64_t counters_blocks = divUp(total_blocks, BASIC_BLOCK_SIZE / sizeof(CounterStorageT));

            ArenaBuffer<AllocationMetadataT> counters;
            allocation_pool_.allocate(0, counters_blocks, counters);

            for (auto& alc: counters.span()) {
                add_awaiting_allocation(alc);
            }

            uint64_t counters_file_pos = counters[0].position() * BASIC_BLOCK_SIZE;
            superblock->block_counters_file_pos() = counters_file_pos;

            Base::superblock_ = superblock;

            allocator_initialization_mode_ = true;

            auto ctr_ref = this->template internal_create_by_name_typed<AllocationMapCtrType>(AllocationMapCtrID);
            MEMORIA_RETURN_IF_ERROR(ctr_ref);
            allocation_map_ctr_ = ctr_ref.get();

            return VoidResult::of();
        });

        if (!maybe_error) {
            internal_init_system_ctr<HistoryCtrType>(
                maybe_error,
                history_ctr_,
                superblock_->history_root_id(),
                HistoryCtrID
            );
        }

        if (!maybe_error) {
            internal_init_system_ctr<DirectoryCtrType>(
                maybe_error,
                directory_ctr_,
                superblock_->directory_root_id(),
                DirectoryCtrID
            );
        }
    }

    virtual ~MappedSWMRStoreWritableCommit() noexcept {
        if (!committed_) {
            store_->unlock_writer();
            delete Base::commit_descriptor_;
        }
    }


    VoidResult finish_store_initialization() noexcept
    {
        MEMORIA_TRY_VOID(allocation_map_ctr_->expand(buffer_.size() / BASIC_BLOCK_SIZE));
        //MEMORIA_TRY_VOID(finish_commit_opening());
        return commit();
    }

    VoidResult finish_commit_opening() noexcept
    {
        if (superblock_->counters_root_id().is_set())
        {
            MEMORIA_TRY_VOID(ref_block(superblock_->counters_root_id()));
        }

        if (superblock_->history_root_id().is_set())
        {
            MEMORIA_TRY_VOID(ref_block(superblock_->history_root_id()));
        }

        if (superblock_->directory_root_id().is_set())
        {
            MEMORIA_TRY_VOID(ref_block(superblock_->directory_root_id()));
        }

        if (superblock_->allocator_root_id().is_set())
        {
            MEMORIA_TRY_VOID(ref_block(superblock_->allocator_root_id()));
        }

        return VoidResult::of();
    }

    virtual CommitID commit_id() noexcept {
        return Base::commit_id();
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id) noexcept
    {
        MEMORIA_TRY_VOID(checkIfConainersCreationAllowed());
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(self_ptr(), ctr_id, decl);
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> create(const LDTypeDeclarationView& decl) noexcept
    {
        MEMORIA_TRY_VOID(checkIfConainersCreationAllowed());
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());

        MEMORIA_TRY(ctr_name, createCtrName());
        return factory->create_instance(self_ptr(), ctr_name, decl);
    }

    virtual VoidResult commit(bool flush = true) noexcept
    {
        if (this->is_active())
        {
            MEMORIA_TRY_VOID(flush_open_containers());

            if (history_ctr_)
            {
                int64_t sb_pos = superblock_->superblock_file_pos();
                MEMORIA_TRY_VOID(history_ctr_->assign_key(superblock_->commit_id(), sb_pos * (
                                                              is_persistent() ? 1 : -1)));
            }

            ArenaBuffer<AllocationMetadataT> all_postponed_deallocations;

            while (awaiting_allocations_.size() > 0 || postponed_deallocations_.size() > 0)
            {
                MEMORIA_TRY_VOID(flush_awaiting_allocations());

                if (postponed_deallocations_.size() > 0)
                {
                    ArenaBuffer<AllocationMetadataT> postponed_deallocations;
                    postponed_deallocations.append_values(postponed_deallocations_.span());
                    all_postponed_deallocations.append_values(postponed_deallocations_.span());
                    postponed_deallocations.sort();
                    postponed_deallocations_.clear();

                    // Just CoW allocation map's leafs that bits will be cleared later
                    auto res = allocation_map_ctr_->touch_bits(postponed_deallocations.span());
                    MEMORIA_RETURN_IF_ERROR(res);
                }
            }

            if (all_postponed_deallocations.size() > 0)
            {
                all_postponed_deallocations.sort();
                // Mark blocks as free
                auto res = allocation_map_ctr_->setup_bits(all_postponed_deallocations.span(), false);
                MEMORIA_RETURN_IF_ERROR(res);
            }

            if (flush) {
                MEMORIA_TRY_VOID(store_->flush_data());
            }

            MEMORIA_TRY_VOID(superblock_->build_superblock_description());

            size_t sb_slot = superblock_->sequence_id() % 2;
            std::memcpy(buffer_.data() + sb_slot * BASIC_BLOCK_SIZE, superblock_, BASIC_BLOCK_SIZE);

            if (flush) {
                MEMORIA_TRY_VOID(store_->flush_header());
            }

            MEMORIA_TRY_VOID(store_->finish_commit(Base::commit_descriptor_));

            committed_ = true;
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


    virtual VoidResult set_persistent(bool persistent) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());
        commit_descriptor_->set_persistent(persistent);
        return VoidResult::of();
    }

    virtual bool is_persistent() noexcept {
        return commit_descriptor_->is_persistent();
    }


    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> find(const CtrID& ctr_id) noexcept {
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
                MEMORIA_TRY_VOID(ref_block(root));

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
                MEMORIA_TRY_VOID(ref_block(root));

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
        else if (MMA_UNLIKELY(ctr_id == HistoryCtrID))
        {
            if (!root.is_null())
            {
                MEMORIA_TRY_VOID(ref_block(root));

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
                MEMORIA_TRY_VOID(ref_block(root));

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
        int32_t level = CustomLog2(scale_factor);

        int64_t block_pos = block->id_value();
        int64_t allocator_block_pos = block_pos;
        int64_t ll_allocator_block_pos = allocator_block_pos >> level;

        AllocationMetadataT meta[1] = {AllocationMetadataT{allocator_block_pos, 1, level}};

        if (parent_allocation_map_ctr_)
        {
            MEMORIA_TRY(status, parent_allocation_map_ctr_->get_allocation_status(level, ll_allocator_block_pos));
            if ((!status) || status.get() == AllocationMapEntryStatus::FREE)
            {
                if (!flushing_awaiting_allocations_) {
                    MEMORIA_TRY_VOID(flush_awaiting_allocations());
                    MEMORIA_TRY_VOID(allocation_map_ctr_->setup_bits(meta, false)); // clear bits
                }
                else {
                    add_postponed_deallocation(meta[0]);
                }
            }
            else {
                add_postponed_deallocation(meta[0]);
            }
        }
        else {
            if (!flushing_awaiting_allocations_) {
                MEMORIA_TRY_VOID(flush_awaiting_allocations());
                MEMORIA_TRY_VOID(allocation_map_ctr_->setup_bits(meta, false)); // clear bits
            }
            else {
                add_postponed_deallocation(meta[0]);
            }
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
        int32_t level = CustomLog2(scale_factor);
        Optional<AllocationMetadataT> allocation = allocation_pool_.allocate_one(level);

        if (!allocation) {
            if (!allocator_initialization_mode_) {
                MEMORIA_TRY_VOID(populate_allocation_pool(level, 1, allocation_prefetch_size(level)));
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Internal block allocation error while initilizing the store");
            }
        }

        add_awaiting_allocation(allocation.get());

        uint64_t id = allocation.get().position();
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
        int32_t level = CustomLog2(scale_factor);

        Optional<AllocationMetadataT> allocation = allocation_pool_.allocate_one(level);

        if (!allocation) {
            if (!allocator_initialization_mode_) {
                MEMORIA_TRY_VOID(populate_allocation_pool(level, 1, allocation_prefetch_size(level)));
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Internal block allocation error while initilizing the store");
            }
        }

        add_awaiting_allocation(allocation.get());

        uint64_t id = allocation.get().position();
        uint8_t* block_addr = buffer_.data() + id * BASIC_BLOCK_SIZE;

        std::memcpy(block_addr, block.block(), block_size);

        BlockType* new_block = ptr_cast<BlockType>(block_addr);
        new_block->id() = BlockID{id};
        new_block->id_value() = id;
        new_block->snapshot_id() = superblock_->commit_uuid();
        new_block->set_references(0);

        return ResultT::of(BlockG{new_block});
    }

    virtual VoidResult ref_block(const BlockID& block_id) noexcept
    {
        return refcounter_delegate_->ref_block(block_id);
    }

    virtual VoidResult unref_block(const BlockID& block_id, BlockCleanupHandler on_zero) noexcept
    {
        return refcounter_delegate_->unref_block(block_id, on_zero);
    }

    virtual VoidResult unref_ctr_root(const BlockID& root_block_id) noexcept
    {
        return unref_block(root_block_id, [=]() noexcept -> VoidResult {
            MEMORIA_TRY(block, this->getBlock(root_block_id));

            auto ctr_hash = block->ctr_type_hash();

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            MEMORIA_TRY(ctr, ctr_intf->new_ctr_instance(block, this));

            ApiProfileBlockID<ApiProfileT> holder = block_id_holder_from(root_block_id);
            MEMORIA_TRY_VOID(ctr->internal_unref_cascade(holder));

            return VoidResult::of();
        });
    }

protected:

    virtual Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>> snapshot_ref_creation_allowed() noexcept {
        using ResultT = Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>>;
        return ResultT::of();
    }


    virtual Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>> snapshot_ref_opening_allowed() noexcept {
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

            assign_to->internal_reset_allocator_holder();

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
    VoidResult populate_allocation_pool(int32_t level, int64_t minimal_amount, int64_t prefetch = 0) noexcept {
        return populate_allocation_pool(allocation_map_ctr_, level, minimal_amount, prefetch);
    }

    VoidResult populate_allocation_pool(CtrSharedPtr<AllocationMapCtr> allocation_map_ctr, int32_t level, int64_t minimal_amount, int64_t prefetch = 0) noexcept
    {
        if (flushing_awaiting_allocations_) {
            return MEMORIA_MAKE_GENERIC_ERROR("Internal Error. Invoking populate_allocation_pool() while flushing awaiting allocations.");
        }

        MEMORIA_TRY_VOID(flush_awaiting_allocations());

        int64_t ranks[ALLOCATION_LEVELS] = {0,};
        MEMORIA_TRY_VOID(allocation_map_ctr->unallocated(ranks));

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

            auto& level_buffer = allocation_pool_.level_buffer(target_level);

            MEMORIA_TRY_VOID(allocation_map_ctr->find_unallocated(
                0, target_level, target_desirable, level_buffer
            ));

            allocation_pool_.refresh(target_level);

            return VoidResult::of();
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

    Result<Superblock*> allocate_superblock(
            const Superblock* parent_sb,
            int64_t commit_id,
            uint64_t
            file_size = 0
    ) noexcept
    {
        using ResultT = Result<Superblock*>;

        ArenaBuffer<AllocationMetadataT> available;
        allocation_pool_.allocate(SUPERBLOCK_ALLOCATION_LEVEL, 1, available);
        if (available.size() > 0)
        {
            AllocationMetadataT allocation = available.tail();

            uint64_t pos = (allocation.position() << SUPERBLOCK_ALLOCATION_LEVEL) * BASIC_BLOCK_SIZE;

            Superblock* superblock = new (buffer_.data() + pos) Superblock();
            if (parent_sb)
            {
                MEMORIA_TRY_VOID(superblock->init_from(*parent_sb, pos, commit_id));
            }
            else {
                MEMORIA_TRY_VOID(superblock->init(pos, file_size, commit_id, SUPERBLOCK_SIZE, 1));
            }

            MEMORIA_TRY_VOID(superblock->build_superblock_description());

            add_awaiting_allocation(allocation);

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



    VoidResult flush_awaiting_allocations() noexcept
    {
        if (awaiting_allocations_.size() > 0 && allocation_map_ctr_)
        {
            if (flushing_awaiting_allocations_){
                return MEMORIA_MAKE_GENERIC_ERROR("Internal error. flush_awaiting_allocations() reentry.");
            }

            flushing_awaiting_allocations_ = true;
            do {
                awaiting_allocations_.sort();
                auto res = allocation_map_ctr_->setup_bits(awaiting_allocations_.span(), true); // set bits
                if (res.is_error())
                {
                    flushing_awaiting_allocations_ = false;
                    return std::move(res).transfer_error();
                }

                awaiting_allocations_.clear();

                awaiting_allocations_.append_values(awaiting_allocations_reentry_.span());
                awaiting_allocations_reentry_.clear();
            }
            while (awaiting_allocations_.size() > 0);
        }

        return VoidResult::of();
    }


    VoidResult for_each_root_block(const std::function<VoidResult (int64_t)>& fn) const noexcept
    {
        MEMORIA_TRY(scanner, history_ctr_->scanner_from(history_ctr_->iterator()));

        bool has_next;
        do {

            for (auto superblock_ptr: scanner.values())
            {
                MEMORIA_TRY_VOID(fn(superblock_ptr));
            }

            MEMORIA_TRY(has_next_res, scanner.next_leaf());
            has_next = has_next_res;
        }
        while (has_next);

        return VoidResult::of();
    }

    static constexpr int32_t CustomLog2(int32_t value) noexcept {
        return 31 - CtLZ((uint32_t)value);
    }

    void add_awaiting_allocation(const AllocationMetadataT& meta) noexcept {
        if (!flushing_awaiting_allocations_) {
            awaiting_allocations_.append_value(meta);
        }
        else {
            awaiting_allocations_reentry_.append_value(meta);
        }
    }

    void add_postponed_deallocation(const AllocationMetadataT& meta) noexcept {
        postponed_deallocations_.append_value(meta);
    }
};

}
