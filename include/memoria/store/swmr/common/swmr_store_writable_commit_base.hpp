
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
#include <memoria/store/swmr/common/swmr_store_counters.hpp>

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

    using typename ISWMRStoreWritableCommit<ApiProfile<Profile>>::ROStoreSnapshotPtr;
    using MyType = SWMRStoreWritableCommitBase;

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
    using Base::getRootID;
    using Base::getBlock;
    using Base::instance_map_;

    using Base::store_;
    using Base::commit_descriptor_;
    using Base::refcounter_delegate_;
    using Base::get_superblock;

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

    using CounterBlockT       = SWMRCounterBlock<Profile>;

    CtrSharedPtr<AllocationMapCtr> parent_allocation_map_ctr_;
    AllocationPool<ApiProfileT, 9> allocation_pool_;

    bool committed_;
    bool allocator_initialization_mode_{false};

    CommitID parent_commit_id_;

    bool flushing_awaiting_allocations_{false};
    ArenaBuffer<AllocationMetadataT> awaiting_allocations_;
    ArenaBuffer<AllocationMetadataT> awaiting_allocations_reentry_;

    ArenaBuffer<AllocationMetadataT> postponed_deallocations_;

    RemovingBlocksConsumerFn removing_blocks_consumer_fn_{};



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
    virtual SharedSBPtr<Superblock> new_superblock(uint64_t pos) = 0;

    virtual Shared* allocate_block(uint64_t at, size_t size, bool for_idmap) = 0;
    virtual Shared* allocate_block_from(const BlockType* source, uint64_t at, bool for_idmap) = 0;

    virtual void init_idmap() {}
    virtual void open_idmap() {}
    virtual void drop_idmap() {}

    virtual void init_commit(MaybeError& maybe_error) noexcept {}
    virtual void init_store_commit(MaybeError& maybe_error) noexcept {}

    void remove_all_blocks()
    {
        auto sb = get_superblock();

        if (sb->directory_root_id().is_set()) {
            unref_ctr_root(sb->directory_root_id());
        }

        if (sb->allocator_root_id().is_set()) {
            unref_ctr_root(sb->allocator_root_id());
        }

        if (sb->history_root_id().is_set()) {
            unref_ctr_root(sb->history_root_id());
        }

        drop_idmap();

        removing_blocks_consumer_fn_(BlockID{BlockIDValueHolder{}}, sb->superblock_file_pos(), sb->superblock_size());
    }

    void cleanup_data() {
        directory_ctr_->cleanup();
    }


    void open_commit()
    {
        open_idmap();

        auto sb = get_superblock();

        auto directory_root_id = sb->directory_root_id();
        if (directory_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(directory_root_id);

            directory_ctr_ = ctr_ref;
            directory_ctr_->internal_reset_allocator_holder();
        }

        auto allocator_root_id = sb->allocator_root_id();
        if (allocator_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<AllocationMapCtrType>(allocator_root_id);

            allocation_map_ctr_ = ctr_ref;
            allocation_map_ctr_->internal_reset_allocator_holder();
        }

        auto history_root_id = sb->history_root_id();
        if (history_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<HistoryCtrType>(history_root_id);

            history_ctr_ = ctr_ref;
            history_ctr_->internal_reset_allocator_holder();
        }
    }



    void init_commit(
            CommitDescriptorT* consistency_point,
            CommitDescriptorT* parent_commit_descriptor
    )
    {
        commit_descriptor_->set_parent(parent_commit_descriptor);
        parent_commit_id_ = parent_commit_descriptor->commit_id();

        auto consistency_point_snp = store_->do_open_readonly(consistency_point);
        auto parent_allocation_map_ctr = consistency_point_snp->find(AllocationMapCtrID);

        parent_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(parent_allocation_map_ctr);

        populate_allocation_pool(parent_allocation_map_ctr_, SUPERBLOCK_ALLOCATION_LEVEL, 64);

        auto parent_sb = get_superblock(parent_commit_descriptor->superblock_ptr());

        auto superblock = allocate_superblock(
            parent_sb.get(),
            ProfileTraits<Profile>::make_random_snapshot_id()
        );

        uint64_t sb_pos = std::get<0>(superblock);
        auto sb = std::get<1>(superblock);

        commit_descriptor_->set_superblock(sb_pos, sb.get());

        init_idmap();

        MaybeError maybe_error;

        if (!maybe_error) {
            internal_init_system_ctr<AllocationMapCtrType>(
                maybe_error,
                allocation_map_ctr_,
                sb->allocator_root_id(),
                AllocationMapCtrID
            );
        }

        if (!maybe_error) {
            internal_init_system_ctr<HistoryCtrType>(
                maybe_error,
                history_ctr_,
                sb->history_root_id(),
                HistoryCtrID
            );
        }

        if (!maybe_error) {
            internal_init_system_ctr<DirectoryCtrType>(
                maybe_error,
                directory_ctr_,
                sb->directory_root_id(),
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
        uint64_t sb_pos = std::get<0>(superblock);
        auto sb = std::get<1>(superblock);

        commit_descriptor_->set_superblock(sb_pos, sb.get());

        uint64_t total_blocks = buffer_size / BASIC_BLOCK_SIZE;
        uint64_t counters_block_capacity = CounterBlockT::capacity_for(BASIC_BLOCK_SIZE * (1 << (ALLOCATION_MAP_LEVELS - 1)));

        uint64_t counters_blocks = divUp(total_blocks, counters_block_capacity);

        ArenaBuffer<AllocationMetadataT> counters;
        allocation_pool_.allocate(ALLOCATION_MAP_LEVELS - 1, counters_blocks, counters);

        for (auto& alc: counters.span()) {
            add_awaiting_allocation(alc);
        }

        uint64_t counters_file_pos = counters[0].position() * BASIC_BLOCK_SIZE;
        sb->set_global_block_counters_file_pos(counters_file_pos);

        allocator_initialization_mode_ = true;

        init_idmap();

        allocation_map_ctr_ = this->template internal_create_by_name_typed<AllocationMapCtrType>(AllocationMapCtrID);

        MaybeError maybe_error;

        if (!maybe_error) {
            internal_init_system_ctr<HistoryCtrType>(
                maybe_error,
                history_ctr_,
                sb->history_root_id(),
                HistoryCtrID
            );
        }

        if (!maybe_error) {
            internal_init_system_ctr<DirectoryCtrType>(
                maybe_error,
                directory_ctr_,
                sb->directory_root_id(),
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
        auto sb = get_superblock();

        if (sb->history_root_id().is_set())
        {
            ref_block(sb->history_root_id());
        }

        if (sb->directory_root_id().is_set())
        {
            ref_block(sb->directory_root_id());
        }

        if (sb->allocator_root_id().is_set())
        {
            ref_block(sb->allocator_root_id());
        }

        if (sb->blockmap_root_id().is_set())
        {
            ref_block(sb->blockmap_root_id());
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

    static constexpr uint64_t block_size_at(int32_t level) noexcept {
        return (1ull << (level - 1)) * BASIC_BLOCK_SIZE;
    }


    void commit(ConsistencyPoint cp)
    {
        if (this->is_active())
        {
            auto sb = get_superblock();

            flush_open_containers();

            auto eviction_ops = store_->prepare_eviction(); // reader synchronous

            for (const auto& update_op: eviction_ops)
            {
                if (update_op.reparent)
                {
                    history_ctr_->with_value(update_op.commit_id, [&](auto value) {
                        using ResultT = std::decay_t<decltype(value)>;
                        auto vv = value.get().view();
                        vv.set_parent_commit_id(update_op.new_parent_id);
                        return ResultT{vv};
                    });
                }
                else {
                    history_ctr_->remove_key(update_op.commit_id);
                }
            }

            CommitMetadata<ApiProfileT> meta;

            uint64_t sb_file_pos = (sb->superblock_file_pos() / BASIC_BLOCK_SIZE);
            meta.set_superblock_file_pos(sb_file_pos);
            meta.set_persistent(is_persistent());

            if (parent_commit_id_) {
                meta.set_parent_commit_id(parent_commit_id_);
            }

            history_ctr_->assign_key(sb->commit_id(), meta);

            ArenaBuffer<AllocationMetadataT> evicting_blocks;
            store_->for_all_evicting_commits([&](CommitDescriptorT* commit_descriptor){
                auto snp = store_->do_open_writable(commit_descriptor, [&](const BlockID& id, uint64_t block_file_pos, int32_t block_size){
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

            do {
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
            while (false);

            if (evicting_blocks.size()) {
                allocation_map_ctr_->setup_bits(evicting_blocks.span(), false);
            }

            store_->finish_commit(Base::commit_descriptor_, cp, sb);

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
            auto sb = get_superblock();
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot {} ", ctr_name, sb->commit_id()).do_throw();
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
        auto sb = get_superblock();
        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            if (!root.is_null())
            {
                ref_block(root);

                auto prev_id = sb->directory_root_id();
                sb->directory_root_id() = root;
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

                auto prev_id = sb->allocator_root_id();
                sb->allocator_root_id() = root;
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

                auto prev_id = sb->history_root_id();
                sb->history_root_id() = root;
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

                auto prev_id = sb->blockmap_root_id();
                sb->blockmap_root_id() = root;
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


    void populate_allocation_pool(int32_t level, int64_t minimal_amount) {
        return populate_allocation_pool(allocation_map_ctr_, level, minimal_amount);
    }

    void populate_allocation_pool(CtrSharedPtr<AllocationMapCtr> allocation_map_ctr, int32_t level, int64_t minimal_amount)
    {
        if (flushing_awaiting_allocations_) {
            MEMORIA_MAKE_GENERIC_ERROR("Internal Error. Invoking populate_allocation_pool() while flushing awaiting allocations.").do_throw();
        }

        flush_awaiting_allocations();

        allocation_pool_.clear();

        int64_t ranks[ALLOCATION_MAP_LEVELS] = {0,};
        allocation_map_ctr->unallocated(ranks);

        if (ranks[level] >= minimal_amount)
        {
            auto& level_buffer = allocation_pool_.level_buffer(level);

            // check the scale of position value across the pool!
            auto exists = allocation_map_ctr->find_unallocated(
                level, minimal_amount, level_buffer
            );

            if (exists < minimal_amount) {
                MEMORIA_MAKE_GENERIC_ERROR(
                    "No enough free space among {}K blocks. Requested = {} blocks, available = {}",
                    (BASIC_BLOCK_SIZE / 1024) << level,
                    minimal_amount,
                    ranks[level]
                ).do_throw();
            }

            allocation_pool_.refresh(level);
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



    std::pair<uint64_t, SharedSBPtr<Superblock>> allocate_superblock(
            const Superblock* parent_sb,
            const CommitID& commit_id,
            uint64_t file_size = 0
    )
    {
        using ResultT = std::pair<uint64_t, SharedSBPtr<Superblock>>;

        ArenaBuffer<AllocationMetadataT> available;
        allocation_pool_.allocate(SUPERBLOCK_ALLOCATION_LEVEL, 1, available);
        if (available.size() > 0)
        {
            AllocationMetadataT allocation = available.tail();

            // FIXME: <<SUPERBLOCK_ALLOCATION_LEVEL is not needed below
            uint64_t pos = (allocation.position() << SUPERBLOCK_ALLOCATION_LEVEL) * BASIC_BLOCK_SIZE;

            //println("Allocating superblock at {} :: {}", allocation.position(), pos);

            auto superblock = new_superblock(pos);
            if (parent_sb)
            {
                superblock->init_from(*parent_sb, pos, commit_id);
            }
            else {
                superblock->init(pos, file_size, commit_id, SUPERBLOCK_SIZE, 1);
            }

            superblock->build_superblock_description();

            add_awaiting_allocation(allocation);

            return ResultT{pos, superblock};
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("No free space for Superblock").do_throw();
        }
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

        return commit(ConsistencyPoint::YES);
    }

    virtual void ref_block(const BlockID& block_id)
    {
        return refcounter_delegate_->ref_block(block_id);
    }

    virtual void unref_block(const BlockID& block_id, BlockCleanupHandler on_zero) {
        return refcounter_delegate_->unref_block(block_id, on_zero);
    }

    virtual void unref_ctr_root(const BlockID& root_block_id)
    {
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


    virtual SnpSharedPtr<IStoreApiBase<ApiProfileT>> snapshot_ref_creation_allowed() {
        return SnpSharedPtr<IStoreApiBase<ApiProfileT>>{};
    }


    virtual SnpSharedPtr<IStoreApiBase<ApiProfileT>> snapshot_ref_opening_allowed() {
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
                populate_allocation_pool(level, 1);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Internal block allocation error while initilizing the store").do_throw();
            }
        }

        add_awaiting_allocation(allocation.get());

        uint64_t position = allocation.get().position();

        auto shared = allocate_block_from(block.block(), position, ctr_id == BlockMapCtrID);        
        BlockType* new_block = shared->get();

        new_block->snapshot_id() = commit_id();
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
                populate_allocation_pool(level, 1);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Internal block allocation error while initilizing the store").do_throw();
            }
        }

        add_awaiting_allocation(allocation.get());

        uint64_t position = allocation.get().position();

        auto shared = allocate_block(position, initial_size, ctr_id == BlockMapCtrID);

        BlockType* block = shared->get();
        block->snapshot_id() = commit_id();

        return shared;
    }

    static constexpr int32_t CustomLog2(int32_t value) noexcept {
        return 31 - CtLZ((uint32_t)value);
    }

    CtrSharedPtr<CtrReferenceable<ApiProfileT>> new_ctr_instance(
            ContainerOperationsPtr<Profile> ctr_intf,
            SharedBlockConstPtr block
    )
    {
        return ctr_intf->new_ctr_instance(block, self_ptr());
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const LDTypeDeclarationView& decl, const CtrID& ctr_id
    )
    {
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(this, ctr_id, decl);
    }

    void import_new_ctr_from(ROStoreSnapshotPtr ptr, const CtrID& name)
    {
        check_updates_allowed();

        MyType* txn = memoria_dynamic_pointer_cast<MyType>(ptr).get();

        auto root_id = getRootID(name);
        if (root_id)
        {
            auto root_id = txn->getRootID(name);

            if (root_id)
            {
                setRoot(name, root_id);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->commit_id()).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} already exists in snapshot {}", name, commit_id()).do_throw();
        }
    }

    void import_ctr_from(ROStoreSnapshotPtr ptr, const CtrID& name)
    {
        check_updates_allowed();

        MyType* txn = memoria_dynamic_pointer_cast<MyType>(ptr).get();

        auto root_id = getRootID(name);
        if (root_id)
        {
            auto root_id = txn->getRootID(name);
            if (root_id)
            {
                setRoot(name, root_id);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->commit_id()).do_throw();
            }
        }
        else {
            return import_new_ctr_from(ptr, name);
        }
    }
};

}
