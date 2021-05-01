
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

#include <memoria/store/swmr/common/swmr_store_commit_base.hpp>
#include <memoria/store/swmr/common/allocation_pool.hpp>

namespace memoria {

template <typename Profile> class SWMRStoreBase;
template <typename Profile> class SWMRStoreReadOnlyCommitBase;

struct InitStoreTag{};

template <typename Profile>
class SWMRStoreWritableCommitBase:
        public SWMRStoreCommitBase<Profile>,
        public ISWMRStoreWritableCommit<ApiProfile<Profile>>
{
protected:
    using Base = SWMRStoreCommitBase<Profile>;


    using typename Base::Store;
    using typename Base::CommitDescriptorT;
    using typename Base::CommitID;
    using typename Base::BlockID;
    using typename Base::BlockIDValueHolder;
    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockType;

    using typename Base::ApiProfileT;
    using typename Base::StoreT;

    using typename Base::AllocationMapCtr;
    using typename Base::AllocationMapCtrType;

    using typename Base::HistoryCtr;
    using typename Base::HistoryCtrType;
    using typename Base::CounterStorageT;
    using typename Base::Superblock;
    using typename Base::Shared;

    using typename Base::DirectoryCtrType;

    using Base::ref_block;
    using Base::unref_block;
    using Base::unref_ctr_root;

    using Base::directory_ctr_;
    using Base::allocation_map_ctr_;
    using Base::history_ctr_;
    using Base::createCtrName;
    using Base::superblock_;
    using Base::getRootID;
    using Base::getBlock;
    using Base::instance_map_;

    using Base::store_;
    using Base::commit_descriptor_;
    using Base::refcounter_delegate_;

    using Base::ALLOCATION_MAP_LEVELS;
    using Base::BASIC_BLOCK_SIZE;
    using Base::SUPERBLOCK_ALLOCATION_LEVEL;
    using Base::SUPERBLOCK_SIZE;
    using Base::SUPERBLOCKS_RESERVED;
    using Base::MAX_BLOCK_SIZE;

    using CtrID = ProfileCtrID<Profile>;
    using CtrReferenceableResult = Result<CtrReferenceable<ApiProfile<Profile>>>;

    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;
    using ParentCommit        = SnpSharedPtr<SWMRStoreReadOnlyCommitBase<Profile>>;

    using BlockCleanupHandler = std::function<VoidResult ()>;

    using RemovingBlocksConsumerFn = typename Store::RemovingBlockConsumerFn;

    using CountersBlockT      =  CountersBlock<Profile>;

    CtrSharedPtr<AllocationMapCtr> parent_allocation_map_ctr_;

    AllocationPool<ApiProfileT, 9> allocation_pool_;

    bool committed_;
    bool allocator_initialization_mode_{false};

    ParentCommit parent_commit_;

    bool flushing_awaiting_allocations_{false};
    ArenaBuffer<AllocationMetadataT> awaiting_allocations_;
    ArenaBuffer<AllocationMetadataT> awaiting_allocations_reentry_;

    ArenaBuffer<AllocationMetadataT> postponed_deallocations_;

    RemovingBlocksConsumerFn removing_blocks_consumer_fn_{};

    struct Counter {
        int32_t value;
    };

    std::unordered_map<BlockID, Counter> counters_;

public:
    using Base::check;
    using Base::resolve_block;

    SWMRStoreWritableCommitBase(
            SharedPtr<Store> store,
            CommitDescriptorT* commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate,
            RemovingBlocksConsumerFn removing_blocks_consumer_fn
    ) noexcept :
        Base(store, commit_descriptor, refcounter_delegate),
        removing_blocks_consumer_fn_(removing_blocks_consumer_fn)
    {
        committed_ = (bool)removing_blocks_consumer_fn_;
    }

    virtual SnpSharedPtr<StoreT> self_ptr() noexcept = 0;
    virtual uint64_t get_memory_size() = 0;
    virtual Superblock* newSuperblock(uint64_t pos) = 0;
    virtual CountersBlockT* new_counters_block(uint64_t pos) = 0;

    virtual Shared* allocate_block(uint64_t at, size_t size, bool for_idmap) = 0;
    virtual Shared* allocate_block_from(const BlockType* source, uint64_t at, bool for_idmap) = 0;

    virtual void init_idmap() {}
    virtual void open_idmap() {}
    virtual void drop_idmap() {}

    virtual void init_commit(MaybeError& maybe_error) noexcept {}
    virtual void init_store_commit(MaybeError& maybe_error) noexcept {}

    void remove_all_blocks()
    {
        if (superblock_->directory_root_id().is_set()) {
            unref_ctr_root(superblock_->directory_root_id());
        }

        if (superblock_->allocator_root_id().is_set()) {
            unref_ctr_root(superblock_->allocator_root_id());
        }

        if (superblock_->history_root_id().is_set()) {
            unref_ctr_root(superblock_->history_root_id());
        }

        drop_idmap();

        removing_blocks_consumer_fn_(BlockID{BlockIDValueHolder{}}, superblock_->superblock_file_pos(), superblock_->superblock_size());
    }


    void open_commit()
    {
        open_idmap();

        auto directory_root_id = commit_descriptor_->superblock()->directory_root_id();
        if (directory_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(directory_root_id);

            directory_ctr_ = ctr_ref;
            directory_ctr_->internal_reset_allocator_holder();
        }

        auto allocator_root_id = commit_descriptor_->superblock()->allocator_root_id();
        if (allocator_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<AllocationMapCtrType>(allocator_root_id);

            allocation_map_ctr_ = ctr_ref;
            allocation_map_ctr_->internal_reset_allocator_holder();
        }

        auto history_root_id = commit_descriptor_->superblock()->history_root_id();
        if (history_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<HistoryCtrType>(history_root_id);

            history_ctr_ = ctr_ref;
            history_ctr_->internal_reset_allocator_holder();
        }
    }



    void init_commit(CommitDescriptorT* parent_commit_descriptor)
    {
        auto parent_commit = store_->do_open_readonly(parent_commit_descriptor);

        parent_commit_ = parent_commit;

        auto parent_allocation_map_ctr = parent_commit_->find(AllocationMapCtrID);
        parent_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(parent_allocation_map_ctr);

        populate_allocation_pool(parent_allocation_map_ctr_, SUPERBLOCK_ALLOCATION_LEVEL, 1, 64);

        auto superblock = allocate_superblock(
            parent_commit_descriptor->superblock(),
            ProfileTraits<Profile>::make_random_snapshot_id()
        );

        commit_descriptor_->set_superblock(superblock);
        superblock_ = superblock;

        init_idmap();

        MaybeError maybe_error;

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

        if (maybe_error) {
            init_commit(maybe_error);
        }

        if (maybe_error) {
            std::move(maybe_error.get()).do_throw();
        }
    }

    void init_store_commit()
    {
        uint64_t buffer_size = get_memory_size();

        int64_t avaialble = (buffer_size / (BASIC_BLOCK_SIZE << (ALLOCATION_MAP_LEVELS - 1)));
        allocation_pool_.add(AllocationMetadataT{0, avaialble, ALLOCATION_MAP_LEVELS - 1});

        int64_t reserved = SUPERBLOCKS_RESERVED;
        ArenaBuffer<AllocationMetadataT> allocations;
        allocation_pool_.allocate(SUPERBLOCK_ALLOCATION_LEVEL, reserved, allocations);

        for (auto& alc: allocations.span()) {
            add_awaiting_allocation(alc);
        }

        CommitID commit_id = ProfileTraits<Profile>::make_random_snapshot_id();
        auto superblock = allocate_superblock(nullptr, commit_id, buffer_size);
        commit_descriptor_->set_superblock(superblock);

        uint64_t total_blocks = buffer_size / BASIC_BLOCK_SIZE;
        uint64_t counters_blocks = divUp(total_blocks, BASIC_BLOCK_SIZE / sizeof(CounterStorageT));

        ArenaBuffer<AllocationMetadataT> counters;
        allocation_pool_.allocate(0, counters_blocks, counters);

        for (auto& alc: counters.span()) {
            add_awaiting_allocation(alc);
        }

        uint64_t counters_file_pos = counters[0].position() * BASIC_BLOCK_SIZE;
        superblock->global_block_counters_file_pos() = counters_file_pos;

        Base::superblock_ = superblock;

        allocator_initialization_mode_ = true;

        init_idmap();

        allocation_map_ctr_ = this->template internal_create_by_name_typed<AllocationMapCtrType>(AllocationMapCtrID);

        MaybeError maybe_error;

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

        if (maybe_error) {
            init_store_commit(maybe_error);
        }

        if (maybe_error) {
            std::move(maybe_error.get()).do_throw();
        }
    }



    void finish_commit_opening()
    {
        if (superblock_->history_root_id().is_set())
        {
            ref_block(superblock_->history_root_id());
        }

        if (superblock_->directory_root_id().is_set())
        {
            ref_block(superblock_->directory_root_id());
        }

        if (superblock_->allocator_root_id().is_set())
        {
            ref_block(superblock_->allocator_root_id());
        }

        if (superblock_->blockmap_root_id().is_set())
        {
            ref_block(superblock_->blockmap_root_id());
        }
    }

    virtual CommitID commit_id() noexcept {
        return Base::commit_id();
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(self_ptr(), ctr_id, decl);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());

        auto ctr_name = createCtrName();
        return factory->create_instance(self_ptr(), ctr_name, decl);
    }


    Optional<AllocationMetadataT> allocate_largest_block(int32_t level)
    {
        while (level >= 0)
        {
            Optional<AllocationMetadataT> allocation = allocation_pool_.allocate_one(level);

            if (!allocation) {
                populate_allocation_pool(level, 1, allocation_prefetch_size(level));
            }

            Optional<AllocationMetadataT> allocation2 = allocation_pool_.allocate_one(level);

            if (!allocation2) {
                --level;
            }
            else {
                return allocation2;
            }
        }

        return {};
    }

    static constexpr uint64_t block_size_at(int32_t level) noexcept {
        return (1ull << (level - 1)) * BASIC_BLOCK_SIZE;
    }


    int32_t compute_counters_block_allocation_level(uint64_t counters) noexcept
    {
        auto block_size = CountersBlockT::estimate_block_size(counters);

        for (int32_t l = 0; l < ALLOCATION_MAP_LEVELS; l++)
        {
            uint64_t level_block_size = block_size_at(l);
            if (block_size <= level_block_size) {
                return l;
            }
        }

        return ALLOCATION_MAP_LEVELS - 1;
    }



    AllocationMetadataT allocate_counters_block(uint64_t& counters)
    {
        int32_t level = compute_counters_block_allocation_level(counters);
        Optional<AllocationMetadataT> allocation = allocate_largest_block(level);

        if (!allocation) {
            MEMORIA_MAKE_GENERIC_ERROR("Can't allocate a block for counters").do_throw();
        }

        uint64_t capacity = CountersBlockT::estimate_block_capacity(block_size_at(level));

        if (capacity > counters) {
            counters = 0;
        }
        else {
            counters -= capacity;
        }

        return allocation.get();
    }


    void simulate_store_counters(ArenaBuffer<AllocationMetadataT>& counters_allocations)
    {
        uint64_t current_counters = counters_.size();

        if (MMA_LIKELY(current_counters > 0))
        {
            uint64_t embedded_size;
            if (current_counters > Superblock::EMBEDDED_COUNTERS_CAPACITY) {
                embedded_size = Superblock::EMBEDDED_COUNTERS_CAPACITY;
            }
            else {
                embedded_size = current_counters;
            }

            current_counters -= embedded_size;

            for (AllocationMetadataT& alc: counters_allocations.span())
            {
                uint64_t block_size = block_size_at(alc.level());
                uint64_t capacity = CountersBlockT::estimate_block_capacity(block_size);

                if (capacity > current_counters) {
                    return;
                }
                else {
                    current_counters -= capacity;
                }
            }

            while (current_counters > 0)
            {
                AllocationMetadataT alc = allocate_counters_block(current_counters);
                counters_allocations.append_value(alc);
            }
        }
    }


    void store_counters(ArenaBuffer<AllocationMetadataT>& counters_allocations)
    {
        uint64_t current_counters = counters_.size();

        if (MMA_LIKELY(current_counters > 0))
        {
            uint64_t embedded_size;
            if (current_counters > Superblock::EMBEDDED_COUNTERS_CAPACITY) {
                embedded_size = Superblock::EMBEDDED_COUNTERS_CAPACITY;
            }
            else {
                embedded_size = current_counters;
            }

            auto ii = counters_.begin();

            for (uint64_t c = 0; c < embedded_size; ++c, ++ii) {
                superblock_->set_counter(c, ii->first, ii->second.value);
            }

            CountersBlockT* last_block{};

            for (AllocationMetadataT& alc: counters_allocations.span())
            {
                uint64_t block_size = block_size_at(alc.level());
                uint64_t capacity = CountersBlockT::estimate_block_capacity(block_size);
                CountersBlockT* block = new_counters_block(alc.position() * BASIC_BLOCK_SIZE);

                uint64_t c{};
                for (; c < capacity && ii != counters_.end(); c++, ++ii) {
                    block->set(c, ii->first, ii->second.value);
                }

                block->init(block_size, c);

                uint64_t file_pos = alc.position() * BASIC_BLOCK_SIZE;

                if (last_block) {
                    last_block->set_next_block_pos(file_pos);
                }
                else {
                    superblock_->commit_block_counters_file_pos() = file_pos;
                }

                if (c < capacity) {
                    last_block = block;
                }
                else {
                    break;
                }
            }

            if (ii != counters_.end()) {
                MEMORIA_MAKE_GENERIC_ERROR("Now all counters was stored in the commit metadata.").do_throw();
            }
        }
    }


    virtual void commit(bool flush = true)
    {
        if (this->is_active())
        {
            flush_open_containers();

            store_->for_all_evicting_commits([&](CommitDescriptorT* commit_descriptor){
                CommitID commit_id = commit_descriptor->superblock()->commit_id();
                history_ctr_->remove_key(commit_id);
            });

            if (history_ctr_)
            {
                uint64_t commit_data = (superblock_->superblock_file_pos() / BASIC_BLOCK_SIZE) << val(SWMRCommitStateMetadataBits::STATE_BITS);

                if (is_persistent()) {
                    commit_data |= val(SWMRCommitStateMetadataBits::PERSISTENT);
                }

                history_ctr_->assign_key(superblock_->commit_id(), commit_data);
            }

            ArenaBuffer<AllocationMetadataT> evicting_blocks;
            store_->for_all_evicting_commits([&](CommitDescriptorT* commit_descriptor){
                auto snp = store_->do_open_writable(commit_descriptor, [&](const BlockID&, uint64_t block_file_pos, int32_t block_size){
                    evicting_blocks.append_value(AllocationMetadataT(
                        block_file_pos / BASIC_BLOCK_SIZE,
                        block_size / BASIC_BLOCK_SIZE,
                        0
                    ));
                });

                snp->remove_all_blocks();
            });

            if (evicting_blocks.size() > 0) {
                evicting_blocks.sort();
                allocation_map_ctr_->touch_bits(evicting_blocks.span());
            }

            uint64_t counters_num;
            ArenaBuffer<AllocationMetadataT> counters_allocations;

            do {
                counters_num = counters_.size();
                simulate_store_counters(counters_allocations);

                ArenaBuffer<AllocationMetadataT> all_postponed_deallocations;
                while (awaiting_allocations_.size() > 0 || postponed_deallocations_.size() > 0)
                {
                    flush_awaiting_allocations();

                    if (postponed_deallocations_.size() > 0)
                    {
                        ArenaBuffer<AllocationMetadataT> postponed_deallocations;
                        postponed_deallocations.append_values(postponed_deallocations_.span());
                        all_postponed_deallocations.append_values(postponed_deallocations_.span());
                        postponed_deallocations_.clear();

                        // Just CoW allocation map's leafs that bits will be cleared later
                        postponed_deallocations.sort();
                        allocation_map_ctr_->touch_bits(postponed_deallocations.span());
                    }
                }


                if (all_postponed_deallocations.size() > 0)
                {
                    all_postponed_deallocations.sort();
                    // Mark blocks as free
                    allocation_map_ctr_->setup_bits(all_postponed_deallocations.span(), false);
                }
            }
            while (counters_num != counters_.size());

            if (evicting_blocks.size()) {
                allocation_map_ctr_->setup_bits(evicting_blocks.span(), false);
            }

            if (counters_allocations.size() > 0)
            {
                counters_allocations.sort();
                allocation_map_ctr_->setup_bits(counters_allocations.span(), true);
            }

            store_counters(counters_allocations);

            if (flush) {
                store_->flush_data();
            }

            superblock_->build_superblock_description();

            auto sb_slot = superblock_->sequence_id() % 2;

            store_->store_superblock(superblock_, sb_slot);

            if (flush) {
                store_->flush_header();
            }

            store_->finish_commit(Base::commit_descriptor_);

            committed_ = true;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Transaction {} has been already committed", commit_id()).do_throw();
        }
    }

    virtual void flush_open_containers() {
        for (const auto& pair: instance_map_)
        {
            pair.second->flush();
        }
    }

    virtual bool drop_ctr(const CtrID& name)
    {
        check_updates_allowed();

        auto root_id = getRootID(name);

        if (root_id.is_set())
        {
            auto block = getBlock(root_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(block->ctr_type_hash());

            ctr_intf->drop(name, self_ptr());
            return true;
        }
        else {
            return false;
        }
    }

    CtrID clone_ctr(const CtrID& ctr_name) {
        return clone_ctr(ctr_name, CtrID{});
    }

    CtrID clone_ctr(const CtrID& ctr_name, const CtrID& new_ctr_name)
    {
        auto root_id = getRootID(ctr_name);
        auto block = getBlock(root_id);

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->clone_ctr(ctr_name, new_ctr_name, self_ptr());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot {} ", ctr_name, superblock_->commit_uuid()).do_throw();
        }
    }


    virtual void set_persistent(bool persistent)
    {
        check_updates_allowed();
        commit_descriptor_->set_persistent(persistent);
    }

    virtual bool is_persistent() noexcept {
        return commit_descriptor_->is_persistent();
    }


    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id) {
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


    virtual void dump_open_containers() {
        return Base::dump_open_containers();
    }

    virtual bool has_open_containers() {
        return Base::has_open_containers();
    }

    virtual std::vector<CtrID> container_names() const {
        return Base::container_names();
    }

    virtual void drop() {
        return Base::drop();
    }

    virtual bool check() {
        return Base::check();
    }

    virtual Optional<U8String> ctr_type_name_for(const CtrID& name) {
        return Base::ctr_type_name_for(name);
    }

    virtual void walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) {
        return Base::walk_containers(walker, allocator_descr);
    }

    virtual void setRoot(const CtrID& ctr_id, const BlockID& root)
    {
        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            if (!root.is_null())
            {
                ref_block(root);

                auto prev_id = superblock_->directory_root_id();
                superblock_->directory_root_id() = root;
                if (prev_id.is_set())
                {
                    unref_ctr_root(prev_id);
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Containers directory removal attempted").do_throw();
            }
        }
        else if (MMA_UNLIKELY(ctr_id == AllocationMapCtrID))
        {
            if (!root.is_null())
            {
                ref_block(root);

                auto prev_id = superblock_->allocator_root_id();
                superblock_->allocator_root_id() = root;
                if (prev_id.is_set())
                {
                    unref_ctr_root(prev_id);
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("AllocationMap removal attempted").do_throw();
            }
        }
        else if (MMA_UNLIKELY(ctr_id == HistoryCtrID))
        {
            if (!root.is_null())
            {
                ref_block(root);

                auto prev_id = superblock_->history_root_id();
                superblock_->history_root_id() = root;
                if (prev_id.is_set())
                {
                    unref_ctr_root(prev_id);
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Commit history removal attempted").do_throw();
            }
        }
        else if (MMA_UNLIKELY(ctr_id == BlockMapCtrID))
        {
            if (!root.is_null())
            {
                ref_block(root);

                auto prev_id = superblock_->blockmap_root_id();
                superblock_->blockmap_root_id() = root;
                if (prev_id.is_set())
                {
                    unref_ctr_root(prev_id);
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("BlockMap removal attempted").do_throw();
            }
        }
        else {
            if (root.is_set())
            {
                ref_block(root);

                auto prev_id = directory_ctr_->replace_and_return(ctr_id, root);
                if (prev_id)
                {
                    unref_ctr_root(prev_id.get());
                }
            }
            else {
                auto prev_id = directory_ctr_->remove_and_return(ctr_id);
                if (prev_id)
                {
                    unref_ctr_root(prev_id.get());
                }
            }
        }
    }

    void checkIfConainersCreationAllowed() {
    }

    void check_updates_allowed() {
    }

    void flush_awaiting_allocations()
    {
        if (awaiting_allocations_.size() > 0 && allocation_map_ctr_)
        {
            if (flushing_awaiting_allocations_){
                MEMORIA_MAKE_GENERIC_ERROR("Internal error. flush_awaiting_allocations() reentry.").do_throw();
            }

            flushing_awaiting_allocations_ = true;
            do {
                awaiting_allocations_.sort();

                auto res = wrap_throwing([&] {
                    return allocation_map_ctr_->setup_bits(awaiting_allocations_.span(), true); // set bits
                });

                if (res.is_error())
                {
                    flushing_awaiting_allocations_ = false;
                    std::move(res).transfer_error().do_throw();
                }

                awaiting_allocations_.clear();

                awaiting_allocations_.append_values(awaiting_allocations_reentry_.span());
                awaiting_allocations_reentry_.clear();
            }
            while (awaiting_allocations_.size() > 0);
        }
    }


    void populate_allocation_pool(int32_t level, int64_t minimal_amount, int64_t prefetch = 0) {
        return populate_allocation_pool(allocation_map_ctr_, level, minimal_amount, prefetch);
    }

    void populate_allocation_pool(CtrSharedPtr<AllocationMapCtr> allocation_map_ctr, int32_t level, int64_t minimal_amount, int64_t prefetch = 0)
    {
        if (flushing_awaiting_allocations_) {
            MEMORIA_MAKE_GENERIC_ERROR("Internal Error. Invoking populate_allocation_pool() while flushing awaiting allocations.").do_throw();
        }

        flush_awaiting_allocations();

        int64_t ranks[ALLOCATION_MAP_LEVELS] = {0,};
        allocation_map_ctr->unallocated(ranks);

        if (ranks[level] >= minimal_amount)
        {
            int64_t desireable = minimal_amount + prefetch;

            int32_t target_level = level;
            int32_t target_desirable = desireable;

            if (MMA_LIKELY(desireable > 1))
            {
                for (int32_t ll = ALLOCATION_MAP_LEVELS - 1; ll >= level; ll--)
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

            allocation_map_ctr->find_unallocated(
                0, target_level, target_desirable, level_buffer
            );

            allocation_pool_.refresh(target_level);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "No enough free space among {}K blocks. Requested = {} blocks, available = {}",
                (BASIC_BLOCK_SIZE / 1024) << level,
                minimal_amount,
                ranks[level]
            ).do_throw();
        }
    }



    Superblock* allocate_superblock(
            const Superblock* parent_sb,
            const CommitID& commit_id,
            uint64_t file_size = 0
    )
    {
        ArenaBuffer<AllocationMetadataT> available;
        allocation_pool_.allocate(SUPERBLOCK_ALLOCATION_LEVEL, 1, available);
        if (available.size() > 0)
        {
            AllocationMetadataT allocation = available.tail();

            uint64_t pos = (allocation.position() << SUPERBLOCK_ALLOCATION_LEVEL) * BASIC_BLOCK_SIZE;

            Superblock* superblock = newSuperblock(pos);
            if (parent_sb)
            {
                superblock->init_from(*parent_sb, pos, commit_id);
            }
            else {
                superblock->init(pos, file_size, commit_id, SUPERBLOCK_SIZE, 1);
            }

            superblock->build_superblock_description();

            add_awaiting_allocation(allocation);

            return superblock;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("No free space for Superblock").do_throw();
        }
    }

    int32_t allocation_prefetch_size(int32_t level) noexcept
    {
        int32_t l0_prefetch_size = 256 * 4;
        return l0_prefetch_size >> level;
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
                assign_to = ctr_ref;
            }
            else {
                auto ctr_ref = this->template internal_create_by_name_typed<CtrName>(ctr_id);
                assign_to = ctr_ref;
            }

            assign_to->internal_reset_allocator_holder();

            return VoidResult::of();
        });
    }

    void finish_store_initialization()
    {
        allocation_map_ctr_->expand(get_memory_size() / BASIC_BLOCK_SIZE);

        // FIXME: finish_commit_opening should be removed completely?
        // Uncommenting the next line will result in counters values mismatch faulure.

        // MEMORIA_TRY_VOID(finish_commit_opening());

        return commit();
    }

    virtual void ref_block(const BlockID& block_id)
    {
        auto ii = counters_.find(block_id);
        if (ii != counters_.end()) {
            if (ii->second.value > 0) {
                int a = 0; a++;
            }

            ii->second.value++;
        }
        else {
            counters_[block_id] = Counter{1};
        }

        return refcounter_delegate_->ref_block(block_id);
    }

    void unref_counter(const BlockID& block_id) {
        auto ii = counters_.find(block_id);
        if (ii != counters_.end()) {
            ii->second.value--;
            if (ii->second.value == 0) {
                counters_.erase(ii);
            }
        }
        else {
            counters_[block_id] = Counter{-1};
        }
    }

    virtual void unref_block(const BlockID& block_id, BlockCleanupHandler on_zero)
    {
        unref_counter(block_id);

        return refcounter_delegate_->unref_block(block_id, on_zero);
    }

    virtual void unref_ctr_root(const BlockID& root_block_id)
    {
        unref_counter(root_block_id);

        return unref_block(root_block_id, [=]() {
            auto block = this->getBlock(root_block_id);

            auto ctr_hash = block->ctr_type_hash();

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            auto ctr = ctr_intf->new_ctr_instance(block, this);

            ApiProfileBlockID<ApiProfileT> holder = block_id_holder_from(root_block_id);
            ctr->internal_unref_cascade(holder);
        });
    }


    virtual SnpSharedPtr<StoreApiBase<ApiProfileT>> snapshot_ref_creation_allowed() {
        return SnpSharedPtr<StoreApiBase<ApiProfileT>>{};
    }


    virtual SnpSharedPtr<StoreApiBase<ApiProfileT>> snapshot_ref_opening_allowed() {
        return Base::snapshot_ref_opening_allowed();
    }


    virtual void removeBlock(const BlockID& id)
    {
        auto block_data = resolve_block(id);

        if (MMA_UNLIKELY((bool)removing_blocks_consumer_fn_)) {
            removing_blocks_consumer_fn_(id, block_data.file_pos, block_data.block->memory_block_size());
        }
        else
        {
            int32_t block_size = block_data.block->memory_block_size();
            int32_t scale_factor = block_size / BASIC_BLOCK_SIZE;
            int32_t level = CustomLog2(scale_factor);

            int64_t allocator_block_pos = block_data.file_pos;
            int64_t ll_allocator_block_pos = allocator_block_pos >> level;

            AllocationMetadataT meta[1] = {AllocationMetadataT{allocator_block_pos, 1, level}};

            if (parent_allocation_map_ctr_)
            {
                auto status = parent_allocation_map_ctr_->get_allocation_status(level, ll_allocator_block_pos);
                if ((!status) || status.get() == AllocationMapEntryStatus::FREE)
                {
                    if (!flushing_awaiting_allocations_) {
                        flush_awaiting_allocations();
                        allocation_map_ctr_->setup_bits(meta, false); // clear bits
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
                    flush_awaiting_allocations();
                    allocation_map_ctr_->setup_bits(meta, false); // clear bits
                }
                else {
                    add_postponed_deallocation(meta[0]);
                }
            }
        }
    }

    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID& ctr_id)
    {
        check_updates_allowed();

        int32_t block_size = block->memory_block_size();
        int32_t scale_factor = block_size / BASIC_BLOCK_SIZE;
        int32_t level = CustomLog2(scale_factor);

        Optional<AllocationMetadataT> allocation = allocation_pool_.allocate_one(level);

        if (!allocation) {
            if (!allocator_initialization_mode_) {
                populate_allocation_pool(level, 1, allocation_prefetch_size(level));
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Internal block allocation error while initilizing the store").do_throw();
            }
        }

        add_awaiting_allocation(allocation.get());

        uint64_t position = allocation.get().position();

        auto shared = allocate_block_from(block.block(), position, ctr_id == BlockMapCtrID);

        BlockType* new_block = shared->get();

        new_block->snapshot_id() = superblock_->commit_uuid();
        new_block->set_references(0);

        return SharedBlockPtr{shared};
    }

    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID& ctr_id)
    {
        check_updates_allowed();

        if (initial_size == -1)
        {
            initial_size = BASIC_BLOCK_SIZE;
        }

        int32_t scale_factor = initial_size / BASIC_BLOCK_SIZE;
        int32_t level = CustomLog2(scale_factor);
        Optional<AllocationMetadataT> allocation = allocation_pool_.allocate_one(level);

        if (!allocation) {
            if (!allocator_initialization_mode_) {
                populate_allocation_pool(level, 1, allocation_prefetch_size(level));
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Internal block allocation error while initilizing the store").do_throw();
            }
        }

        add_awaiting_allocation(allocation.get());

        uint64_t position = allocation.get().position();

        auto shared = allocate_block(position, initial_size, ctr_id == BlockMapCtrID);

        BlockType* block = shared->get();
        block->snapshot_id() = superblock_->commit_uuid();

        return shared;
    }

    static constexpr int32_t CustomLog2(int32_t value) noexcept {
        return 31 - CtLZ((uint32_t)value);
    }
};

}
