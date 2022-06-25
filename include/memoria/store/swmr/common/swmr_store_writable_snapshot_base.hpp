
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

#include <memoria/store/swmr/common/swmr_store_snapshot_base.hpp>
#include <memoria/store/swmr/common/allocation_pool.hpp>
#include <memoria/store/swmr/common/swmr_store_counters.hpp>
namespace memoria {

template <typename Profile> class SWMRStoreBase;
template <typename Profile> class SWMRStoreReadOnlySnapshotBase;

struct InitStoreTag{};

struct RWCounter {
    int64_t value{};
    auto inc()  {
        return ++value;
    }

    bool dec(int64_t other) noexcept
    {
        value--;
        return (other + value) == 0;
    }
};

template <typename Profile>
class SWMRStoreWritableSnapshotBase:
        public SWMRStoreSnapshotBase<Profile>,
        public ISWMRStoreWritableSnapshot<ApiProfile<Profile>>
{
protected:
    using Base = SWMRStoreSnapshotBase<Profile>;

    using typename ISWMRStoreWritableSnapshot<ApiProfile<Profile>>::ROStoreSnapshotPtr;
    using MyType = SWMRStoreWritableSnapshotBase;

    using typename Base::Store;
    using typename Base::CDescrPtr;
    using typename Base::SnapshotDescriptorT;
    using typename Base::SnapshotID;
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
    using Base::instance_pool;

    using Base::store_;
    using Base::snapshot_descriptor_;
    using Base::refcounter_delegate_;
    using Base::get_superblock;
    using Base::is_transient;
    using Base::is_system_snapshot;

    using Base::ALLOCATION_MAP_LEVELS;
    using Base::BASIC_BLOCK_SIZE;
    using Base::SUPERBLOCK_ALLOCATION_LEVEL;
    using Base::SUPERBLOCK_SIZE;
    using Base::SUPERBLOCKS_RESERVED;
    using Base::MAX_BLOCK_SIZE;

    using CtrID = ProfileCtrID<Profile>;
    using CtrReferenceableResult = Result<CtrReferenceable<ApiProfile<Profile>>>;

    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;
    using ParentCommit        = SnpSharedPtr<SWMRStoreReadOnlySnapshotBase<Profile>>;

    using BlockCleanupHandler = std::function<void ()>;

    using RemovingBlocksConsumerFn = typename Store::RemovingBlockConsumerFn;

    using CounterBlockT       = SWMRCounterBlock<Profile>;
    using AllocationPoolT = AllocationPool<ApiProfileT, 9>;


    CtrSharedPtr<AllocationMapCtr> consistency_point_allocation_map_ctr_;
    CtrSharedPtr<AllocationMapCtr> head_allocation_map_ctr_;
    AllocationPoolT* allocation_pool_;
    ArenaBuffer<AllocationMetadataT> awaiting_init_allocations_;
    Optional<AllocationMetadataT> preallocated_;

    bool populating_allocation_pool_{};
    int32_t allocate_reserved_{};
    int32_t forbid_allocations_{};
    bool init_store_mode_{};

    bool allocator_map_cloned_{};

    class FlagScope {
        bool& flag_;
    public:
        FlagScope(bool& flag)  : flag_(flag) {
            flag_ = true;
        }
        ~FlagScope() noexcept  {
            flag_ = false;
        }
    };

    template <typename TT>
    class CounterScope {
        TT& cntr_;
    public:
        CounterScope(TT& cntr)  : cntr_(cntr) {
            ++cntr_;
        }
        ~CounterScope() noexcept {
            --cntr_;
        }
    };

    SnapshotID parent_snapshot_id_;

    ArenaBuffer<AllocationMetadataT> postponed_deallocations_;

    RemovingBlocksConsumerFn removing_blocks_consumer_fn_{};
    CDescrPtr head_snapshot_descriptor_{};
    CDescrPtr consistency_point_snapshot_descriptor_{};

    LDDocument metadata_doc_;

    enum class State {
        ACTIVE, PREPARED, COMMITTED, ROLLED_BACK
    };

    State state_{State::ACTIVE};

    bool do_consistency_point_{false};


    using CountersT = std::unordered_map<BlockID, RWCounter>;

    CountersT counters_;
    CountersT* counters_ptr_ {&counters_};

    std::unordered_set<U8String> removing_branches_;
    std::unordered_set<SnapshotID> removing_snapshots_;

    // It's used to accomodate branch removal to keep the number of
    // times the branch node is traversed. If all children are
    // removed, we can proceed proceed removing node's ancestors.
    std::unordered_map<SnapshotID, RWCounter> branch_removal_counters_;

public:
    using Base::check;
    using Base::resolve_block;
    using Base::resolve_block_allocation;
    using Base::CustomLog2;

    SWMRStoreWritableSnapshotBase(
            SharedPtr<Store> store,
            CDescrPtr& snapshot_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate,
            RemovingBlocksConsumerFn removing_blocks_consumer_fn
    ) noexcept :
        Base(store, snapshot_descriptor, refcounter_delegate),
        removing_blocks_consumer_fn_(removing_blocks_consumer_fn)
    {
        state_ = removing_blocks_consumer_fn_ ? State::COMMITTED : State::ACTIVE;
        allocation_pool_ = &store_->allocation_pool();
        this->writable_ = true;
    }

    virtual SnpSharedPtr<StoreT> my_self_ptr()  = 0;
    virtual SnpSharedPtr<StoreT> self_ptr() {
        return my_self_ptr();
    }

    virtual uint64_t get_memory_size() = 0;
    virtual SharedSBPtr<Superblock> new_superblock(uint64_t pos) = 0;

    virtual Shared* allocate_block(uint64_t at, size_t size, bool for_idmap) = 0;
    virtual Shared* allocate_block_from(const BlockType* source, uint64_t at, bool for_idmap) = 0;

    virtual void init_idmap() {}
    virtual void open_idmap() {}
    virtual void drop_idmap() {}

    virtual void init_snapshot(MaybeError& maybe_error)  {}
    virtual void init_store_snapshot(MaybeError& maybe_error)  {}

    auto& removing_branches()  {return removing_branches_;}
    auto& removing_snapshots()  {return removing_snapshots_;}

    CountersT& counters()  {return counters_;}

    AllocationMetadataT do_allocate_reserved(int64_t remainder)
    {
        auto alc = allocation_pool_->allocate_reserved(remainder);
        if (alc) {
            return alc.get();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Empty reserved allocation pool").do_throw();
        }
    }

    AllocationMetadataT allocate_one_or_throw(int32_t level = 0)
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
        else if (MMA_LIKELY(!allocate_reserved_))
        {
            auto alc = allocation_pool_->allocate_one(level);
            if (alc)
            {
                if (MMA_UNLIKELY(init_store_mode_)) {
                    awaiting_init_allocations_.append_value(alc.get());
                }

                return alc.get();
            }
            else if (populating_allocation_pool_) {
                // Level must be 0 here
                return do_allocate_reserved(1);
            }
            else {
                // FIXME: More clever logic is needed here
                for (size_t cc = 0; cc < 10; cc++) {
                    populate_allocation_pool(level);
                    auto alc1 = allocation_pool_->allocate_one(level);
                    if (alc1) {
                        return alc1.get();
                    }
                }

                MEMORIA_MAKE_GENERIC_ERROR("FIXME: Tried multiple times to allocate from the pool and failed.").do_throw();
            }
        }
        else {
            // Level must be 0 here
            return do_allocate_reserved(1);
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

            removing_blocks_consumer_fn_(
                BlockID{BlockIDValueHolder{}},
                AllocationMetadataT(
                    static_cast<int64_t>(sb->superblock_file_pos()/ BASIC_BLOCK_SIZE),
                    1,
                    CustomLog2(sb->superblock_size() / BASIC_BLOCK_SIZE)
                )
            );

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


    void open_snapshot()
    {
        open_idmap();

        auto sb = get_superblock();

        auto directory_root_id = sb->directory_root_id();
        if (directory_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(directory_root_id);

            directory_ctr_ = ctr_ref;
            directory_ctr_->internal_detouch_from_store();
        }

        auto allocator_root_id = sb->allocator_root_id();
        if (allocator_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<AllocationMapCtrType>(allocator_root_id);

            allocation_map_ctr_ = ctr_ref;
            allocation_map_ctr_->internal_detouch_from_store();
        }

        auto history_root_id = sb->history_root_id();
        if (history_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<HistoryCtrType>(history_root_id);

            history_ctr_ = ctr_ref;
            history_ctr_->internal_detouch_from_store();
        }
    }

    void init_snapshot(
            CDescrPtr& consistency_point,
            CDescrPtr& head,
            CDescrPtr& parent_snapshot_descriptor
    )
    {
        snapshot_descriptor_->set_parent(parent_snapshot_descriptor.get());
        parent_snapshot_id_ = parent_snapshot_descriptor->snapshot_id();

        // HEAD is always defined here
        head_snapshot_descriptor_ = head;
        consistency_point_snapshot_descriptor_ = consistency_point;

        auto head_snp = store_->do_open_readonly(head);
        auto head_allocation_map_ctr = head_snp->find(AllocationMapCtrID);
        head_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(head_allocation_map_ctr);

        // HEAD snapshot is not a consistency point
        if (head != consistency_point)
        {
            auto consistency_point_snp = store_->do_open_readonly(consistency_point);
            auto cp_allocation_map_ctr = consistency_point_snp->find(AllocationMapCtrID);
            consistency_point_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(cp_allocation_map_ctr);
        }

        auto head_sb = get_superblock(head_snapshot_descriptor_->superblock_ptr());
        auto superblock = allocate_superblock(
            head_sb.get(),
            ProfileTraits<Profile>::make_random_snapshot_id()
        );

        uint64_t sb_pos = std::get<0>(superblock);
        auto sb = std::get<1>(superblock);

        snapshot_descriptor_->set_superblock(sb_pos, sb.get());

        //println("SeqNum: {} {}", sb->snapshot_id(), sb->sequence_id());

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
            init_snapshot(maybe_error);
        }

        if (maybe_error) {
            std::move(maybe_error.get()).do_throw();
        }
    }

    void init_store_snapshot()
    {
        init_store_mode_ = true;

        // FIXME: need to configure 'emergency' block allocation pool here.
        uint64_t buffer_size = get_memory_size();

        int64_t avaialble = buffer_size / BASIC_BLOCK_SIZE;

        allocation_pool_->add(AllocationMetadataT(0, avaialble, ALLOCATION_MAP_LEVELS - 1));

        awaiting_init_allocations_.append_value(allocation_pool_->allocate_one(0).get());
        awaiting_init_allocations_.append_value(allocation_pool_->allocate_one(0).get());

        SnapshotID snapshot_id = ProfileTraits<Profile>::make_random_snapshot_id();

        auto superblock = allocate_superblock(nullptr, snapshot_id, buffer_size);
        uint64_t sb_pos = std::get<0>(superblock);
        auto sb = std::get<1>(superblock);

        snapshot_descriptor_->set_superblock(sb_pos, sb.get());

        uint64_t total_blocks = buffer_size / BASIC_BLOCK_SIZE;

        constexpr uint64_t ctr_blk_scale = (1 << (ALLOCATION_MAP_LEVELS - 1));
        uint64_t counters_block_capacity = CounterBlockT::capacity_for(BASIC_BLOCK_SIZE * ctr_blk_scale);

        uint64_t counters_blocks = div_up(total_blocks, counters_block_capacity) * ctr_blk_scale;

        uint64_t counters_file_pos {};
        for (uint64_t c = 0; c < counters_blocks; c++)
        {
            auto alc = allocation_pool_->allocate_one(0);
            if (alc){
                if (c == 0) {
                    counters_file_pos = alc.get().position() * BASIC_BLOCK_SIZE;
                }

                awaiting_init_allocations_.append_value(alc.get());
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Failure allocating counters block").do_throw();
            }
        }

        sb->set_global_block_counters_file_pos(counters_file_pos);
        sb->set_global_block_counters_blocks(counters_blocks);

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
            init_store_snapshot(maybe_error);
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

        allocation_pool_->clear();

        init_store_mode_ = false;

        populate_allocation_pool(0);

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

    void finish_snapshot_opening()
    {

    }

    void populate_allocation_pool(int32_t level)
    {
        if (populating_allocation_pool_){
            MEMORIA_MAKE_GENERIC_ERROR("Internal Error: populate_allocation_pool() re-entry.").do_throw();
        }

        FlagScope scope(populating_allocation_pool_);
        if (!allocation_map_ctr_->populate_allocation_pool(*allocation_pool_, level))
        {
            allocation_map_ctr_->populate_allocation_pool(*allocation_pool_, level);

            MEMORIA_MAKE_GENERIC_ERROR(
                        "No enough free space among {}K blocks. Requested = {} blocks.",
                        (BASIC_BLOCK_SIZE / 1024) << level,
                        1
            ).do_throw();
        }
    }

    std::pair<uint64_t, SharedSBPtr<Superblock>> allocate_superblock(
            const Superblock* parent_sb,
            const SnapshotID& snapshot_id,
            uint64_t file_size = 0
    ){
        using ResultT = std::pair<uint64_t, SharedSBPtr<Superblock>>;

        AllocationMetadataT allocation = do_allocate_reserved(0);

        if (MMA_UNLIKELY(init_store_mode_)) {
            awaiting_init_allocations_.append_value(allocation);
        }

        uint64_t pos = (allocation.position() << SUPERBLOCK_ALLOCATION_LEVEL) * BASIC_BLOCK_SIZE;

        LDDocument meta;
        LDDMapView map = meta.set_map();
        map.set_varchar("branch_name", snapshot_descriptor_->branch());

        auto superblock = new_superblock(pos);
        if (parent_sb) {
            superblock->init_from(*parent_sb, pos, snapshot_id, meta);
        }
        else {
            superblock->init(pos, file_size, snapshot_id, SUPERBLOCK_SIZE, 1, 1, meta);
        }
        superblock->build_superblock_description();

        return ResultT{pos, superblock};
    }

    void add_postponed_deallocation(const AllocationMetadataT& meta)  {
        postponed_deallocations_.append_value(meta);
    }

    void add_cp_postponed_deallocation(const AllocationMetadataT& meta)  {
        head_snapshot_descriptor_->postponed_deallocations().append_value(meta);
    }


    void do_postponed_deallocations(bool consistency_point)
    {
        // CoW pages with bits for CP-wide postponed deallocations.
        // Those bits will be cleared later
        if (consistency_point && head_snapshot_descriptor_)
        {
            head_snapshot_descriptor_->postponed_deallocations().sort();
            allocation_map_ctr_->touch_bits(head_snapshot_descriptor_->postponed_deallocations().span());
        }

        // First, pre-build CoW-tree for postopend deallocations
        // made in this snapshot
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
            // Mark blocks as free            
            allocation_map_ctr_->setup_bits(all_postponed_deallocations.span(), false);
        }

        if (consistency_point && head_snapshot_descriptor_) {
            allocation_map_ctr_->setup_bits(head_snapshot_descriptor_->postponed_deallocations().span(), false);
            head_snapshot_descriptor_->clear_postponed_deallocations();
        }
    }

    template <typename Fn>
    auto do_with_reserved_allocator(Fn&& fn) {
        CounterScope<int32_t> scope(allocate_reserved_);
        return fn();
    }

    template <typename Fn>
    auto without_allocations(Fn&& fn) {
        CounterScope<int32_t> scope(forbid_allocations_);
        return fn();
    }

    void register_allocation(const AllocationMetadataT&) {}

    void prepare(ConsistencyPoint cp)
    {
        if (is_active())
        {
            if (consistency_point_snapshot_descriptor_) {
                consistency_point_snapshot_descriptor_->inc_snapshots();
            }

            if (cp == ConsistencyPoint::AUTO) {
                do_consistency_point_ =
                        consistency_point_snapshot_descriptor_
                    ->should_make_consistency_point(snapshot_descriptor_);
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
                    history_ctr_->with_value(update_op.snapshot_id, [&](auto value) {
                        using ResultT = std::decay_t<decltype(value)>;
                        if (value) {
                            auto vv = value.get().view();
                            vv.set_parent_snapshot_id(update_op.new_parent_id);
                            return ResultT{vv};
                        }
                        else {
                            return ResultT{};
                        }
                    });
                }
                else {
                    history_ctr_->remove_key(update_op.snapshot_id);
                }
            }

            SWMRSnapshotMetadata<ApiProfileT> meta;

            uint64_t sb_file_pos = (sb->superblock_file_pos() / BASIC_BLOCK_SIZE);
            meta.set_superblock_file_pos(sb_file_pos);
            meta.set_transient(is_transient());
            meta.set_system_snapshot(is_system_snapshot());

            if (parent_snapshot_id_) {
                meta.set_parent_snapshot_id(parent_snapshot_id_);
            }

            history_ctr_->upsert_key(sb->snapshot_id(), meta);

            ArenaBuffer<AllocationMetadataT> evicting_blocks;
            store_->for_all_evicting_snapshots([&](SnapshotDescriptorT* snapshot_descriptor){
                auto snp = store_->do_open_writable(snapshot_descriptor, [&](const BlockID& id, const AllocationMetadataT& meta){
                    evicting_blocks.append_value(meta);
                }, true);

                snp->remove_all_blocks(&counters_);
            });

            if (evicting_blocks.size() > 0) {
                evicting_blocks.sort();
                allocation_map_ctr_->touch_bits(evicting_blocks.span());
            }

            // Note: All deallocation before this line MUST do 'touch bits' to
            // build corresponding CoW-tree, but not marking blocks
            // ass avalable. Otherwise they can be reused immediately.
            // That will make 'fast' rolling back the last snapshot or
            // conistency point impossible. (Regular snapshot reloval
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
            // and regular snapshot removal (with extra system snapshot) will
            // be necessary.

            // Now it's safe to actually mark blocks as free
            if (evicting_blocks.size()) {
                allocation_map_ctr_->setup_bits(evicting_blocks.span(), false);
            }

            allocation_pool_->store(sb->allocation_pool_data());

            if (MMA_UNLIKELY(allocation_pool_->level0_total() < 1))
            {
                // We must ensure that at the end of a snapshot, at least
                // one preallocated block must be in the superblock's pool.
                MEMORIA_MAKE_GENERIC_ERROR(
                    "Superblock's allocation pool is empty at the end of the snapshot"
                ).do_throw();
            }

            store_->do_prepare(Base::snapshot_descriptor_, cp, do_consistency_point_, sb);

            state_ = State::PREPARED;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been already closed", snapshot_id()).do_throw();
        }
    }

    void commit(ConsistencyPoint cp)
    {
        if (is_active()) {
            prepare(cp);
        }

        store_->do_commit(Base::snapshot_descriptor_, do_consistency_point_, this);
        state_ = State::COMMITTED;
    }

    void rollback()
    {
        store_->do_rollback(Base::snapshot_descriptor_);
        state_ = State::ROLLED_BACK;
    }

    virtual SnapshotID snapshot_id()  {
        return Base::snapshot_id();
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id)
    {
        checkIfConainersCreationAllowed();
        return this->create_ctr_instance(decl, ctr_id);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl)
    {
        checkIfConainersCreationAllowed();
        auto ctr_id = createCtrName();
        return this->create_ctr_instance(decl, ctr_id);
    }

    static constexpr uint64_t block_size_at(int32_t level)  {
        return (1ull << (level - 1)) * BASIC_BLOCK_SIZE;
    }

    virtual void flush_open_containers() {
        this->instance_pool().for_each_open_ctr(this->self_ptr(), [](auto ctr_id, auto ctr){
            ctr->flush();
        });
    }

    virtual bool drop_ctr(const CtrID& ctr_id)
    {
        check_updates_allowed();

        auto ctr = this->find(ctr_id);
        if (ctr) {
            ctr->drop();
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
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot {} ", ctr_name, sb->snapshot_id()).do_throw();
        }
    }


    virtual void set_transient(bool transient)
    {
        check_updates_allowed();
        snapshot_descriptor_->set_transient(transient);
    }




    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id) {
        return Base::find(ctr_id);
    }

    virtual bool is_committed() const  {
        return state_ == State::COMMITTED;
    }

    virtual bool is_active() const  {
        return state_ == State::ACTIVE;
    }

    virtual bool is_marked_to_clear() const  {
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

    virtual void check(const CheckResultConsumerFn& consumer) {
        return Base::check(consumer);
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
                MEMORIA_MAKE_GENERIC_ERROR("Snapshot history removal attempted").do_throw();
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
    )
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
            return VoidResult::of();
        });
    }



    virtual void ref_block(const BlockID& block_id)
    {
        auto ii = counters_ptr_->find(block_id);
        if (ii != counters_ptr_->end()) {
            ii->second.inc();
        }
        else {
            counters_ptr_->insert(std::make_pair(block_id, RWCounter{1}));
        }
    }

    virtual void unref_block(const BlockID& block_id) {
        return unref_block(block_id, [&](){
            auto block = this->getBlock(block_id);

            auto ctr_hash   = block->ctr_type_hash();
            auto block_hash = block->block_type_hash();

            auto blk_intf = ProfileMetadata<Profile>::local()
                    ->get_block_operations(ctr_hash, block_hash);

            blk_intf->for_each_child(block.block(), [&](const BlockID& child_id){
                unref_block(child_id);
            });

            removeBlock(block_id);
        });
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
            RWCounter cntr{0};
            zero = cntr.dec(cnt);
            counters_ptr_->insert(std::make_pair(block_id, cntr));
        }

        if (zero) {
            on_zero();
        }
    }

    virtual void unref_ctr_root(const BlockID& root_block_id)
    {
        return unref_block(root_block_id);
    }


    virtual SnpSharedPtr<IStoreApiBase<ApiProfileT>> snapshot_ref_creation_allowed() {
        return SnpSharedPtr<IStoreApiBase<ApiProfileT>>{};
    }


    virtual SnpSharedPtr<IStoreApiBase<ApiProfileT>> snapshot_ref_opening_allowed() {
        return Base::snapshot_ref_opening_allowed();
    }


    bool is_transient()  {
        return snapshot_descriptor_->is_transient();
    }

    bool is_system_snapshot()  {
        return snapshot_descriptor_->is_system_snapshot();
    }


    virtual void removeBlock(const BlockID& id)
    {
        auto block_alc = resolve_block_allocation(id);

        if (MMA_UNLIKELY((bool)removing_blocks_consumer_fn_)) {
            removing_blocks_consumer_fn_(id, block_alc);
        }
        else
        {
            int32_t level = block_alc.level();
            int64_t ll_allocator_block_pos = block_alc.position() >> block_alc.level();

            if (head_allocation_map_ctr_)
            {
                // Checking first in the head snapshot
                auto head_blk_status = head_allocation_map_ctr_->get_allocation_status(level, ll_allocator_block_pos);
                if ((!head_blk_status) || head_blk_status.get() == AllocationMapEntryStatus::FREE)
                {
                    // If the block is FREE in the HEAD snapshot, it is also FREE
                    // in the CONSISTENCY POINT snapshot. So we are just marking it FREE
                    // in this snapshot. It's immediately reusable.

                    // Returning the block to the pool of available blocks
                    if (!allocation_pool_->add(block_alc)) {
                        // FIXME: Add it into an 'extra' local pool instead of postponing
                        add_postponed_deallocation(block_alc);
                    }
                }
                else if (consistency_point_allocation_map_ctr_)
                {
                    auto cp_blk_status = consistency_point_allocation_map_ctr_->get_allocation_status(level, ll_allocator_block_pos);
                    if ((!cp_blk_status) || cp_blk_status.get() == AllocationMapEntryStatus::FREE)
                    {
                        // The block is allocated in the HEAD snapshot, but free in the
                        // CONSISTENCY POINT snapshot.
                        add_postponed_deallocation(block_alc);
                    }
                    else {
                        // The block is allocated in the HEAD snapshot, AND it's allocated in the
                        // CONSISTENCY POINT snapshot.
                        // This block will be freed at the consistency point snapshot.
                        add_cp_postponed_deallocation(block_alc);
                    }
                }
                else {
                    // The block is allocated in the head snapshot AND there is no
                    // consistency point yet. Can't free the block now.
                    // Postoponing it until the snapshot. The block will be reuable
                    // in the next snapshot
                    add_postponed_deallocation(block_alc);
                }
            }
            else {
                // Returning the block to the pool of available blocks
                if (!allocation_pool_->add(block_alc)) {
                    // FIXME: Add it into an 'extra' local pool instead of postponing
                    add_postponed_deallocation(block_alc);
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

        AllocationMetadataT allocation = allocate_one_or_throw(level);
        uint64_t position = allocation.position();

        auto shared = allocate_block_from(block.block(), position, ctr_id == BlockMapCtrID);        
        BlockType* new_block = shared->get();

        new_block->snapshot_id() = snapshot_id();
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

        if (consistency_point_snapshot_descriptor_) {
            consistency_point_snapshot_descriptor_->add_allocated(scale_factor);
        }

        BlockType* block = shared->get();
        block->snapshot_id() = snapshot_id();

        return shared;
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const LDTypeDeclarationView& decl, const CtrID& ctr_id
    )
    {
        auto ptr = this->create_ctr_instance(decl, ctr_id);
        ptr->internal_detouch_from_store();
        return ptr;
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
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->snapshot_id()).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} already exists in snapshot {}", name, snapshot_id()).do_throw();
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
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->snapshot_id()).do_throw();
            }
        }
        else {
            return import_new_ctr_from(ptr, name);
        }
    }

    bool update_snapshot_metadata(const SnapshotID& snapshot_id, std::function<void (SWMRSnapshotMetadata<ApiProfileT>&)> fn)
    {
        bool exists = true;
        history_ctr_->with_value(snapshot_id, [&](const auto& value) {
            using ResultT = std::decay_t<decltype(value)>;
            if (value) {
                auto vv = value.get().view();
                fn(vv);
                return ResultT{vv};
            }
            else {
                exists = false;
                return ResultT{};
            }
        });
        return exists;
    }

    bool set_transient(const SnapshotID& snapshot_id, bool value) {
        return update_snapshot_metadata(snapshot_id, [](auto& meta){
            meta.set_transient(true);
        });
    }

    bool set_removed_branch(const SnapshotID& snapshot_id, bool value) {
        return update_snapshot_metadata(snapshot_id, [](auto& meta){
            meta.set_removed_branch(true);
        });
    }


    bool visit_snapshot(const SnapshotDescriptorT* descr)
    {
        auto ii = branch_removal_counters_.find(descr->snapshot_id());
        if (ii != branch_removal_counters_.end())
        {
            if (ii->second.inc() < descr->children().size()) {
                return false;
            }
            else {
                return true;
            }
        }
        else {
            branch_removal_counters_.insert(std::make_pair(descr->snapshot_id(), RWCounter{1}));
            return descr->children().size() <= 1;
        }
    }

    bool remove_branch(U8StringView branch_name)
    {
        if (removing_branches_.find(branch_name) != removing_branches_.end()) {
           return true;
        }

        removing_branches_.insert(branch_name);

        using LockGuard = std::lock_guard<std::recursive_mutex>;

        LockGuard rlock(store_->history_mutex());
        auto head = store_->history_tree().get_branch_head(branch_name);
        if (head)
        {
            SnapshotDescriptorT* cd = head.get();

            while (cd && visit_snapshot(cd))
            {
                if (!cd->is_transient()){
                    remove_snapshot(cd->snapshot_id());
                }

                cd = cd->parent();
            }

            set_removed_branch(cd->snapshot_id(), true);
            return true;
        }
        else {
            return false;
        }
    }



    bool remove_snapshot(const SnapshotID& snapshot_id)
    {
        auto ii = removing_snapshots_.find(snapshot_id);
        if (ii == removing_snapshots_.end())
        {
            if (set_transient(snapshot_id, true)) {
                removing_snapshots_.insert(snapshot_id);
            }
            else {
                return false;
            }
        }

        return true;
    }

    void start_no_reentry(const CtrID& ctr_id) {
        if (ctr_id == AllocationMapCtrID) {
            ++allocate_reserved_;
        }
    }
    void finish_no_reentry(const CtrID& ctr_id) noexcept {
        if (ctr_id == AllocationMapCtrID) {
            --allocate_reserved_;
        }
    }
};

}
