
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
#include <memoria/store/swmr/common/swmr_store_commit_id_proxy.hpp>
namespace memoria {

template <typename Profile> class SWMRStoreBase;
template <typename Profile> class SWMRStoreReadOnlyCommitBase;

struct InitStoreTag{};

struct RWBlkCounter {
    int64_t value{};
    void inc() {
        value++;
    }

    bool dec(int64_t other)
    {
        value--;
        return (other + value) == 0;
    }
};

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
    using typename Base::CDescrPtr;
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
    using Base::is_transient;
    using Base::is_system_commit;

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

    using BlockCleanupHandler = std::function<void ()>;

    using RemovingBlocksConsumerFn = typename Store::RemovingBlockConsumerFn;

    using CounterBlockT       = SWMRCounterBlock<Profile>;
    using AllocationPoolT = AllocationPool<ApiProfileT, 9>;


    CtrSharedPtr<AllocationMapCtr> consistency_point_allocation_map_ctr_;
    CtrSharedPtr<AllocationMapCtr> head_allocation_map_ctr_;
    AllocationPoolT allocation_pool_;
    ArenaBuffer<AllocationMetadataT> awaiting_init_allocations_;
    Optional<AllocationMetadataT> preallocated_;

    bool populating_allocation_pool_{};
    bool allocate_from_superblock_{};
    bool forbid_allocations_{};

    bool allocator_map_cloned_{};

    class FlagScope {
        bool& flag_;
    public:
        FlagScope(bool& flag) noexcept : flag_(flag) {
            flag_ = true;
        }
        ~FlagScope() noexcept {
            flag_ = false;
        }
    };

    CommitID parent_commit_id_;

    ArenaBuffer<AllocationMetadataT> postponed_deallocations_;

    RemovingBlocksConsumerFn removing_blocks_consumer_fn_{};
    CDescrPtr head_commit_descriptor_{};
    CDescrPtr consistency_point_commit_descriptor_{};

    LDDocument metadata_doc_;

    enum class State {
        ACTIVE, PREPARED, COMMITTED, ROLLED_BACK
    };

    State state_{State::ACTIVE};

    bool do_consistency_point_{false};


    using CountersT = std::unordered_map<BlockID, RWBlkCounter>;

    CountersT counters_;
    CountersT* counters_ptr_ {&counters_};

public:
    using Base::check;
    using Base::resolve_block;

    SWMRStoreWritableCommitBase(
            SharedPtr<Store> store,
            CDescrPtr& commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate,
            RemovingBlocksConsumerFn removing_blocks_consumer_fn
    ) noexcept :
        Base(store, commit_descriptor, refcounter_delegate),
        removing_blocks_consumer_fn_(removing_blocks_consumer_fn)
    {
        state_ = removing_blocks_consumer_fn_ ? State::COMMITTED : State::ACTIVE;
    }


    virtual SnpSharedPtr<StoreT> my_self_ptr() noexcept = 0;
    virtual SnpSharedPtr<StoreT> self_ptr() noexcept
    {
        return my_self_ptr();
    }

    virtual uint64_t get_memory_size() = 0;
    virtual SharedSBPtr<Superblock> new_superblock(uint64_t pos) = 0;

    virtual Shared* allocate_block(uint64_t at, size_t size, bool for_idmap) = 0;
    virtual Shared* allocate_block_from(const BlockType* source, uint64_t at, bool for_idmap) = 0;

    virtual void init_idmap() {}
    virtual void open_idmap() {}
    virtual void drop_idmap() {}

    virtual void init_commit(MaybeError& maybe_error) noexcept {}
    virtual void init_store_commit(MaybeError& maybe_error) noexcept {}

    AllocationMetadataT do_allocate_from_superblock(size_t reserve = 1)
    {
        auto alc = get_superblock()->allocate_one(reserve);
        if (alc) {
            return alc.get();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Internal error. Superblock's allocation pool is empty.").do_throw();
        }
    }

    AllocationMetadataT allocate_one_or_throw(int32_t level = 0, size_t reserve = 1) {
        auto alc = allocate_one_or_throw_(level, reserve);
        return alc;
    }

    AllocationMetadataT allocate_one_or_throw_(int32_t level = 0, size_t reserve = 1)
    {
        if (MMA_UNLIKELY(forbid_allocations_))
        {
            MEMORIA_MAKE_GENERIC_ERROR("Internal error. Block allocations are fordidden at this stage.").do_throw();
        }
        else if (MMA_UNLIKELY((bool)preallocated_))
        {
            AllocationMetadataT alc = preallocated_.get();
            preallocated_ = Optional<AllocationMetadataT>{};
            return alc;
        }
        else if (MMA_LIKELY(!allocate_from_superblock_))
        {
            auto alc = allocation_pool_.allocate_one(level);
            if (alc) {
                return alc.get();
            }
            else if (populating_allocation_pool_) {
                // Level must be 0 here
                return do_allocate_from_superblock(reserve);
            }
            else {
                populate_allocation_pool(level, 1);
                auto alc1 = allocation_pool_.allocate_one(level);
                if (alc1) {
                    return alc1.get();
                }
                else {
                    populate_allocation_pool(level, 1);
                    auto alc2 = allocation_pool_.allocate_one(level);
                    if (alc2) {
                        return alc1.get();
                    }
                    else {
                        MEMORIA_MAKE_GENERIC_ERROR("FIXME: Tried multiple times to allocate from the pool and failed.").do_throw();
                    }
                }
            }
        }
        else {
            // Level must be 0 here
            return do_allocate_from_superblock(reserve);
        }
    }

    void remove_all_blocks(CountersT* counters)
    {
        try {
            counters_ptr_ = counters;

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

            counters_ptr_ = &counters_;
        }
        catch (...) {
            counters_ptr_ = &counters_;
            throw;
        }
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
            CDescrPtr& consistency_point,
            CDescrPtr& head,
            CDescrPtr& parent_commit_descriptor
    )
    {
        commit_descriptor_->set_parent(parent_commit_descriptor.get());
        parent_commit_id_ = parent_commit_descriptor->commit_id();

        // HEAD is always defined here
        head_commit_descriptor_ = head;
        consistency_point_commit_descriptor_ = consistency_point;

        auto head_snp = store_->do_open_readonly(head);
        auto head_allocation_map_ctr = head_snp->find(AllocationMapCtrID);
        head_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(head_allocation_map_ctr);

        // HEAD commit is not a consistency point
        if (head != consistency_point)
        {
            auto consistency_point_snp = store_->do_open_readonly(consistency_point);
            auto cp_allocation_map_ctr = consistency_point_snp->find(AllocationMapCtrID);
            consistency_point_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(cp_allocation_map_ctr);
        }

        auto head_sb = get_superblock(head_commit_descriptor_->superblock_ptr());

        AllocationMetadataT sb_alc;
        size_t alc_idx;
        std::tie(alc_idx, sb_alc) = head_sb->get_allocation();

        preallocated_ = sb_alc;
        auto superblock = allocate_superblock(
            head_sb.get(),
            ProfileTraits<Profile>::make_random_snapshot_id()
        );

        uint64_t sb_pos = std::get<0>(superblock);
        auto sb = std::get<1>(superblock);

        commit_descriptor_->set_superblock(sb_pos, sb.get());

        // Marking that the new SB as allocated in the new SB.
        sb->remove_block_from_pool(alc_idx);

        do_ref_system_containers();

        init_idmap();

        MaybeError maybe_error;

        if (!maybe_error)
        {
            internal_init_system_ctr<AllocationMapCtrType>(
                        maybe_error,
                        allocation_map_ctr_,
                        sb->allocator_root_id(),
                        AllocationMapCtrID
            );
        }

        populate_allocation_pool(allocation_map_ctr_, SUPERBLOCK_ALLOCATION_LEVEL, 64);
        sb->preallocate_from_pool(allocation_pool_);

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
        // FIXME: need to configure 'emergency' block allocation pool here.
        uint64_t buffer_size = get_memory_size();

        int64_t avaialble = buffer_size / BASIC_BLOCK_SIZE;

        AllocationPool<ApiProfileT, 1> init_allocation_pool;

        //int64_t reserved = SUPERBLOCKS_RESERVED;
        init_allocation_pool.add(AllocationMetadataT{0, avaialble, 0});

        awaiting_init_allocations_.append_value(init_allocation_pool.allocate_one(0).get());
        awaiting_init_allocations_.append_value(init_allocation_pool.allocate_one(0).get());

        CommitID commit_id = ProfileTraits<Profile>::make_random_snapshot_id();

        preallocated_ = init_allocation_pool.allocate_one(0);
        awaiting_init_allocations_.append_value(preallocated_.get());

        auto superblock = allocate_superblock(nullptr, commit_id, buffer_size);
        uint64_t sb_pos = std::get<0>(superblock);
        auto sb = std::get<1>(superblock);

        commit_descriptor_->set_superblock(sb_pos, sb.get());

        uint64_t total_blocks = buffer_size / BASIC_BLOCK_SIZE;

        constexpr uint64_t ctr_blk_scale = (1 << (ALLOCATION_MAP_LEVELS - 1));
        uint64_t counters_block_capacity = CounterBlockT::capacity_for(BASIC_BLOCK_SIZE * ctr_blk_scale);

        uint64_t counters_blocks = divUp(total_blocks, counters_block_capacity) * ctr_blk_scale;

        ArenaBuffer<AllocationMetadataT> counters;
        init_allocation_pool.allocate(0, counters_blocks, counters);
        awaiting_init_allocations_.append_values(counters.span());

        uint64_t counters_file_pos = counters[0].position() * BASIC_BLOCK_SIZE;
        sb->set_global_block_counters_file_pos(counters_file_pos);

        if (!sb->preallocate_from_fn([&]{
            auto alc = init_allocation_pool.allocate_one(0);
            if (alc) {
                awaiting_init_allocations_.append_value(alc.get());
            }
            return alc;
        })) {
            MEMORIA_MAKE_GENERIC_ERROR("Internal Error. InitAllocationPool is empty").do_throw();
        }
        allocate_from_superblock_ = true;

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

    void finish_store_initialization()
    {
        allocation_map_ctr_->expand(get_memory_size() / BASIC_BLOCK_SIZE);

        // FIXME: This should not trigger any block allocation
        allocation_map_ctr_->setup_bits(awaiting_init_allocations_.span(), true);

        allocate_from_superblock_ = false;

        return commit(ConsistencyPoint::YES);
    }

    void do_ref_system_containers()
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

    void finish_commit_opening()
    {

    }

    void populate_allocation_pool(int32_t level, int64_t minimal_amount)
    {
        populate_allocation_pool(allocation_map_ctr_, level, minimal_amount);

        auto sb = get_superblock();
        if (sb->preallocated_pool_capacity() > 0) {
            sb->preallocate_from_pool(allocation_pool_);
        }
    }

    void populate_allocation_pool(CtrSharedPtr<AllocationMapCtr> allocation_map_ctr, int32_t level, int64_t minimal_amount)
    {
        if (populating_allocation_pool_){
            MEMORIA_MAKE_GENERIC_ERROR("Internal Error: populate_allocation_pool() re-entry.").do_throw();
        }

        FlagScope scope(populating_allocation_pool_);

        int64_t ranks[ALLOCATION_MAP_LEVELS] = {0,};
        allocation_map_ctr->unallocated(ranks);

        if (ranks[level] >= minimal_amount)
        {
            auto& level_buffer = allocation_pool_.level_buffer(level);

            // check the scale of position value across the pool!
            auto exists = allocation_map_ctr->allocate(
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

        AllocationMetadataT allocation = allocate_one_or_throw(0, 0);

        uint64_t pos = (allocation.position() << SUPERBLOCK_ALLOCATION_LEVEL) * BASIC_BLOCK_SIZE;

        LDDocument meta;
        LDDMapView map = meta.set_map();
        map.set_varchar("branch_name", commit_descriptor_->branch());

        auto superblock = new_superblock(pos);
        if (parent_sb) {
            superblock->init_from(*parent_sb, pos, commit_id, meta);
        }
        else {
            superblock->init(pos, file_size, commit_id, SUPERBLOCK_SIZE, 1, 1, meta);
        }
        superblock->build_superblock_description();

        return ResultT{pos, superblock};
    }

    void add_postponed_deallocation(const AllocationMetadataT& meta) noexcept {
        postponed_deallocations_.append_value(meta);
    }

    void add_cp_postponed_deallocation(const AllocationMetadataT& meta) noexcept {
        head_commit_descriptor_->postponed_deallocations().append_value(meta);
    }


    void do_postponed_deallocations(bool consistency_point)
    {
        // CoW pages with bits for CP-wide postponed deallocations.
        // Those bits will be cleared later
        if (consistency_point && head_commit_descriptor_)
        {
            head_commit_descriptor_->postponed_deallocations().sort();
            allocation_map_ctr_->touch_bits(head_commit_descriptor_->postponed_deallocations().span());
        }

        // First, pre-build CoW-tree for postopend deallocations
        // made in this commit
        ArenaBuffer<AllocationMetadataT> all_postponed_deallocations;
        while (postponed_deallocations_.size() > 0)
        {
            ArenaBuffer<AllocationMetadataT> postponed_deallocations;
            postponed_deallocations.append_values(postponed_deallocations_.span());
            all_postponed_deallocations.append_values(postponed_deallocations_.span());
            postponed_deallocations_.clear();

            // Just CoW allocation map's leafs that bits will be cleared later
            postponed_deallocations.sort();
            allocation_map_ctr_->touch_bits(postponed_deallocations.span());
        }

        while (postponed_deallocations_.size() > 0)
        {
            ArenaBuffer<AllocationMetadataT> postponed_deallocations;
            postponed_deallocations.append_values(postponed_deallocations_.span());
            all_postponed_deallocations.append_values(postponed_deallocations_.span());
            postponed_deallocations_.clear();

            // Just CoW allocation map's leafs that bits will be cleared later
            postponed_deallocations.sort();
            allocation_map_ctr_->touch_bits(postponed_deallocations.span());
        }

        if (all_postponed_deallocations.size() > 0)
        {
            all_postponed_deallocations.sort();
            // Mark blocks as free
            allocation_map_ctr_->setup_bits(all_postponed_deallocations.span(), false);
        }

        if (consistency_point && head_commit_descriptor_) {
            allocation_map_ctr_->setup_bits(head_commit_descriptor_->postponed_deallocations().span(), false);
            head_commit_descriptor_->clear_postponed_deallocations();
        }
    }

    template <typename Fn>
    auto do_with_sb_allocator(Fn&& fn) {
        FlagScope scope(allocate_from_superblock_);
        return fn();
    }

    template <typename Fn>
    auto without_allocations(Fn&& fn) {
        FlagScope scope(forbid_allocations_);
        return fn();
    }

    void prepare(ConsistencyPoint cp)
    {
        if (is_active())
        {
            if (consistency_point_commit_descriptor_) {
                consistency_point_commit_descriptor_->inc_commits();
            }

            if (cp == ConsistencyPoint::AUTO) {
                do_consistency_point_ =
                        consistency_point_commit_descriptor_
                    ->should_make_consistency_point(commit_descriptor_);
            }
            else {
                do_consistency_point_ = cp == ConsistencyPoint::YES || cp == ConsistencyPoint::FULL;
            }

            auto sb = get_superblock();

            flush_open_containers();

            auto eviction_ops = store_->prepare_eviction(); // reader synchronous

            for (const auto& update_op: eviction_ops)
            {
                if (update_op.reparent)
                {
                    history_ctr_->with_value(update_op.commit_id, [&](auto value) {
                        using ResultT = std::decay_t<decltype(value)>;
                        if (value) {
                            auto vv = value.get().view();
                            vv.set_parent_commit_id(update_op.new_parent_id);
                            return ResultT{vv};
                        }
                        else {
                            return ResultT{};
                        }
                    });
                }
                else {
                    history_ctr_->remove_key(update_op.commit_id);
                }
            }

            CommitMetadata<ApiProfileT> meta;

            uint64_t sb_file_pos = (sb->superblock_file_pos() / BASIC_BLOCK_SIZE);
            meta.set_superblock_file_pos(sb_file_pos);
            meta.set_transient(is_transient());
            meta.set_system_commit(is_system_commit());

            if (parent_commit_id_) {
                meta.set_parent_commit_id(parent_commit_id_);
            }

            history_ctr_->assign_key(sb->commit_id(), meta);

            ArenaBuffer<AllocationMetadataT> evicting_blocks;
            store_->for_all_evicting_commits([&](CommitDescriptorT* commit_descriptor){
                //println("    ############# Removing commit: {}", commit_descriptor->commit_id());
                auto snp = store_->do_open_writable(commit_descriptor, [&](const BlockID& id, uint64_t block_file_pos, int32_t block_size){
                    evicting_blocks.append_value(AllocationMetadataT(
                        block_file_pos / BASIC_BLOCK_SIZE,
                        block_size / BASIC_BLOCK_SIZE,
                        0
                    ));
                });

                snp->remove_all_blocks(&counters_);
            });

            if (evicting_blocks.size() > 0) {
                evicting_blocks.sort();
                allocation_map_ctr_->touch_bits(evicting_blocks.span());
            }

            // Note: All deallocation before this line MUST do 'touch bits' to
            // build corresponding CoW-tree, but not marking blocks
            // ass avalable. Otherwise they can be reused immediately.
            // That will make 'fast' rolling back the last commit or
            // conistency point impossible. (Regular commit reloval
            // will still be possible though).

            // TODO: Collect all allocated but unused blocks here
            // and 'touch_bits' them.

            // All 'touch bits' calls must be done before this line.
            do_postponed_deallocations(do_consistency_point_);

            // Note that it's not possible to extend CoW-tree further
            // afther the do_postponed_deallocations() is done IF
            // we want the to be able to do the fast rollback.
            //
            // For exabple, when the store is closeing, we have to
            // make flush, to force the conistency point. The flush()
            // will call do_postponed_deallocations() again, and it _may_
            // allocate memory. If so, fast rollback will not be possible
            // and regular commit removal (with extra system commit) will
            // be necessary.

            // Now it's safe to actually mark blocks as free
            if (evicting_blocks.size()) {
                allocation_map_ctr_->setup_bits(evicting_blocks.span(), false);
            }

            // Moving some preallocated blocks into the superblock, if any.
            sb->preallocate_from_pool(allocation_pool_);

            // All preallocated blocks which are still in the allocation pool,
            // we have to return them back to the allocation map.
            // It's safe to do here, because on the pre-allocation all
            // blocks have been already CoW-ed in this commit.
            // So, we are just setting all the allocations them to false.

            // But to ensure that the following procedure do not actually make
            // new allocations, we are douing it in a special closure.

            without_allocations([&]{
                for (int32_t ll = 0; ll < ALLOCATION_MAP_LEVELS; ll++) {
                    allocation_map_ctr_->setup_bits(allocation_pool_.level_buffer(ll).span(), false);
                }
            });

            allocation_pool_.reset();

            if (MMA_UNLIKELY(sb->preallocated_pool_size() == 0))
            {
                // We must ensure that at the end of a commit, at least
                // one preallocated block must be in the superblock's pool.
                MEMORIA_MAKE_GENERIC_ERROR(
                    "Superblock's allocation pool is empty at the end of the commit"
                ).do_throw();
            }

            store_->do_prepare(Base::commit_descriptor_, cp, do_consistency_point_, sb);

            state_ = State::PREPARED;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Commit {} has been already closed", commit_id()).do_throw();
        }
    }

    void commit(ConsistencyPoint cp)
    {
        if (is_active()) {
            prepare(cp);
        }

//        for (const auto& entry: counters_) {
//            println("    ********* Counter: {} {} {}", commit_id(), entry.first, entry.second.value);
//        }

        store_->do_commit(Base::commit_descriptor_, do_consistency_point_, counters_);
        state_ = State::COMMITTED;
    }

    void rollback()
    {
        store_->do_rollback(Base::commit_descriptor_);
        state_ = State::ROLLED_BACK;
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


    virtual void set_transient(bool transient)
    {
        check_updates_allowed();
        commit_descriptor_->set_transient(transient);
    }




    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id) {
        return Base::find(ctr_id);
    }

    virtual bool is_committed() const noexcept {
        return state_ == State::COMMITTED;
    }

    virtual bool is_active() const noexcept {
        return state_ == State::ACTIVE;
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
                allocator_map_cloned_ = true;

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



    virtual void ref_block(const BlockID& block_id)
    {
//        if (block_id.value() == 614) {
//            int a = 0;
//            a++;
//        }

        auto ii = counters_ptr_->find(block_id);
        if (ii != counters_ptr_->end()) {
            ii->second.inc();
        }
        else {
            counters_ptr_->insert(std::make_pair(block_id, RWBlkCounter{1}));
        }

//        println("RefBlock: {} {} {}", commit_id(), block_id, (*counters_ptr_)[block_id].value);
    }

    virtual void unref_block(const BlockID& block_id, BlockCleanupHandler on_zero)
    {
        auto refs = refcounter_delegate_->count_refs(block_id);
        auto cnt = get_optional_value_or(refs, 0);

        bool zero = false;

        auto ii = counters_ptr_->find(block_id);
        if (ii != counters_ptr_->end()) {
            zero = ii->second.dec(cnt);
        }
        else {
            RWBlkCounter cntr{0};
            zero = cntr.dec(cnt);
            counters_ptr_->insert(std::make_pair(block_id, cntr));
        }

//        println("UnRefBlock: {} {} {}", commit_id(), block_id, (*counters_ptr_)[block_id].value);

        if (zero) {
            on_zero();
        }
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


    bool is_transient() noexcept {
        return commit_descriptor_->is_transient();
    }

    bool is_system_commit() noexcept {
        return commit_descriptor_->is_system_commit();
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

            int64_t allocator_block_pos = block_data.file_pos / BASIC_BLOCK_SIZE;
            int64_t ll_allocator_block_pos = allocator_block_pos >> level;

            AllocationMetadataT meta{allocator_block_pos, 1, level};

            if (head_allocation_map_ctr_)
            {
                // Checking first in the head commit
                auto head_blk_status = head_allocation_map_ctr_->get_allocation_status(level, ll_allocator_block_pos);
                if ((!head_blk_status) || head_blk_status.get() == AllocationMapEntryStatus::FREE)
                {
                    // If the block is FREE in the HEAD commit, it is also FREE
                    // in the CONSISTENCY POINT commit. So we are just marking it FREE
                    // in this commit. It's immediately reusable.

                    // Returning the block to the pool of available blocks
                    allocation_pool_.add(meta);
                }
                else if (consistency_point_allocation_map_ctr_)
                {
                    auto cp_blk_status = consistency_point_allocation_map_ctr_->get_allocation_status(level, ll_allocator_block_pos);
                    if ((!cp_blk_status) || cp_blk_status.get() == AllocationMapEntryStatus::FREE)
                    {
                        // The block is allocated in the HEAD commit, but free in the
                        // CONSISTENCY POINT commit.
                        add_postponed_deallocation(meta);
                    }
                    else {
                        // The block is allocated in the HEAD commit, AND it's allocated in the
                        // CONSISTENCY POINT commit.
                        // This block will be freed at the consistency point commit.
                        add_cp_postponed_deallocation(meta);
                    }
                }
                else {
                    // The block is allocated in the head commit AND there is no
                    // consistency point yet. Can't free the block now.
                    // Postoponing it until the commit. The block will be reuable
                    // in the next commit
                    add_postponed_deallocation(meta);
                }
            }
            else {
                // Returning the block to the pool of available blocks
                allocation_pool_.add(meta);
            }
        }
    }



    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID& ctr_id)
    {
        check_updates_allowed();

        int32_t block_size = block->memory_block_size();
        int32_t scale_factor = block_size / BASIC_BLOCK_SIZE;
        int32_t level = CustomLog2(scale_factor);

        AllocationMetadataT allocation = allocate_one_or_throw(level);
        uint64_t position = allocation.position();

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

        AllocationMetadataT allocation = allocate_one_or_throw(level);
        uint64_t position = allocation.position();

        auto shared = allocate_block(position, initial_size, ctr_id == BlockMapCtrID);

        if (consistency_point_commit_descriptor_) {
            consistency_point_commit_descriptor_->add_allocated(scale_factor);
        }

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


    virtual bool remove_branch(U8StringView branch_name)
    {
        return true;



//        try {
//            auto result_opt = [&]{
//                using TupleT = std::tuple<CDescrPtr, CDescrPtr, bool>;

//                LockGuard lock(reader_mutex_);
//                auto descr = history_tree_.get_branch_head(branch_name);
//                if (descr)
//                {
//                    U8String last_branch = history_tree_.consistency_point1()->branch();
//                    U8String new_branch = history_tree_.mark_branch_transient(descr.get(), branch_name);
//                    bool do_cleanup_data = history_tree_.branches_size() == 1;

//                    CDescrPtr commit_descriptor = create_commit_descriptor(new_branch);

//                    return Optional<TupleT>{TupleT{descr, std::move(commit_descriptor), do_cleanup_data}};
//                }
//                else {
//                    return Optional<TupleT>{};
//                }
//            }();

//            if (result_opt)
//            {
//                auto& result = result_opt.get();

//                CDescrPtr descr = std::get<0>(result);
//                CDescrPtr commit_descriptor = std::move(std::get<1>(result));
//                bool do_cleanup_data = std::get<2>(result);

//                SWMRWritableCommitPtr ptr = do_create_writable(
//                            history_tree_.consistency_point1(),
//                            history_tree_.head(),
//                            descr,
//                            std::move(commit_descriptor)
//                );
//                ptr->finish_commit_opening();

//                // If we are removing the only branch, so we need to clean also the data,
//                // because the 'main' branch should be recreated with empty state.
//                if (do_cleanup_data) {
//                    ptr->cleanup_data();
//                }

//                // This will remove all non-persistent commits from the history.
//                ptr->commit(ConsistencyPoint::AUTO);
//                return ptr->commit_id();
//            }
//            else {
//                return Result{};
//            }
//        }
//        catch (...) {
//            writer_mutex_.unlock();
//            throw;
//        }
    }




    bool remove_commit(const CommitID& commit_id)
    {


        return true;
    }
};

}
