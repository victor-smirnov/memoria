
// Copyright 2020-2025 Victor Smirnov
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

#include <memoria/store/oltp/oltp_store_writable_snapshot_base.hpp>
#include <memoria/store/swmr/common/allocation_pool.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <memoria/store/oltp/block_id_set.hpp>

namespace memoria {

template <typename Profile>
class BlockIOOLTPStoreWritableSnapshotBase:
        public OLTPStoreWritableSnapshotBase<Profile>
{
protected:
    using Base = OLTPStoreWritableSnapshotBase<Profile>;
    using MyType = BlockIOOLTPStoreWritableSnapshotBase;


    using typename Base::ROStoreSnapshotPtr;
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

    using typename Base::EvcQueueCtr;
    using typename Base::EvcQueueCtrType;
    using typename Base::Superblock;
    using typename Base::Shared;
    using typename Base::CtrSizeT;


    using typename Base::DirectoryCtrType;


    using AllocationMapCtrType = AllocationMap;
    using AllocationMapCtr  = ICtrApi<AllocationMapCtrType, ApiProfileT>;

    using Base::ref_block;
    using Base::unref_block;
    using Base::unref_ctr_root;

    using Base::directory_ctr_;
    using Base::evc_queue_ctr_;
    using Base::createCtrName;
    using Base::getRootID;
    using Base::getBlock;
    using Base::instance_pool;

    using Base::store_;
    using Base::snapshot_descriptor_;
    using Base::get_superblock;
    using Base::is_transient;
    using Base::is_system_snapshot;
    using Base::commit;
    using Base::flush_eviction_queue_buffer;
    using Base::find_oldest_reader;

    using Base::HEADER_SIZE;
    using Base::BASIC_BLOCK_SIZE;
    using Base::SUPERBLOCK_SIZE;

    static constexpr int32_t ALLOCATION_MAP_LEVELS    = ICtrApi<AllocationMap, ApiProfileT>::LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP = ICtrApi<AllocationMap, ApiProfileT>::ALLOCATION_SIZE * BASIC_BLOCK_SIZE;


    static constexpr size_t LAST_ALLOCATION_LEVEL       = ALLOCATION_MAP_LEVELS - 1;
    static constexpr size_t SUPERBLOCK_ALLOCATION_LEVEL = Log2(SUPERBLOCK_SIZE / BASIC_BLOCK_SIZE) - 1;
    static constexpr size_t SUPERBLOCKS_RESERVED        = HEADER_SIZE / BASIC_BLOCK_SIZE;

    static constexpr size_t MAX_BLOCK_SIZE               = (1ull << (ALLOCATION_MAP_LEVELS - 1)) * BASIC_BLOCK_SIZE;

    using CtrID = ProfileCtrID<Profile>;
    using CtrReferenceableResult = Result<CtrReferenceable<ApiProfile<Profile>>>;

    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;
    using ParentCommit        = SnpSharedPtr<OLTPStoreReadOnlySnapshotBase<Profile>>;

    using BlockCleanupHandler = std::function<void ()>;

    using CounterBlockT       = SWMRCounterBlock<Profile>;
    using AllocationPoolT     = AllocationPool<ApiProfileT, 9>;

    CtrSharedPtr<AllocationMapCtr> consistency_point_allocation_map_ctr_;
    CtrSharedPtr<AllocationMapCtr> head_allocation_map_ctr_;
    AllocationPoolT* allocation_pool_;
    ArenaBuffer<AllocationMetadataT> awaiting_init_allocations_;
    Optional<AllocationMetadataT> preallocated_;

    bool populating_allocation_pool_{};
    int32_t allocate_reserved_{};
    int32_t forbid_allocations_{};
    //bool init_store_mode_{};

    bool allocator_map_cloned_{};

    CtrSharedPtr<AllocationMapCtr>  allocation_map_ctr_;

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
    ArenaBuffer<BlockID> postponed_evictions_;

    CDescrPtr head_snapshot_descriptor_{};
    CDescrPtr consistency_point_snapshot_descriptor_{};

    hermes::HermesCtr metadata_doc_;

    enum class State {
        ACTIVE, PREPARED, COMMITTED, ROLLED_BACK
    };

    State state_{State::ACTIVE};

    bool do_consistency_point_{false};

    std::unordered_map<BlockID, SharedBlockPtr> my_blocks_;
    std::vector<uint64_t> eviction_queue_buf_;

    static constexpr io::DevSizeT SIZE_INCREMENT_UNIT = 1ull << (ALLOCATION_MAP_LEVELS - 1); // 1M in 4K blocks
    static constexpr io::DevSizeT SIZE_INCREMENT_UNIT_BYTES = SIZE_INCREMENT_UNIT * BASIC_BLOCK_SIZE; // 1M
    static constexpr io::DevSizeT INITIAL_STORE_SIZE = 1; // In units, equivalent to 1MB

    io::DevSizeT store_size_increment_{16}; // in size increment units

    std::shared_ptr<io::BlockIOProvider> blockio_;

public:
    using Base::check;
    using Base::resolve_block;
    using Base::resolve_block_allocation;
    using Base::CustomLog2;


    BlockIOOLTPStoreWritableSnapshotBase(
        SharedPtr<Store> store,
        std::shared_ptr<io::BlockIOProvider> blockio,
        CDescrPtr& snapshot_descriptor
    )
    noexcept :
        Base(store, snapshot_descriptor),
        blockio_(blockio)
    {
        state_ = State::ACTIVE;
        //allocation_pool_ = &store_->allocation_pool();
        this->writable_ = true;
    }

    static void init_profile_metadata() {
        Base::init_profile_metadata();
        AllocationMapCtr::template init_profile_metadata<Profile>();
    }

    virtual io::BlockPtr<Superblock> new_superblock(io::DevSizeT pos) = 0;

    virtual Shared* allocate_block(io::DevSizeT at, size_t size) {
        return nullptr;
    }
    virtual Shared* allocate_block_from(const BlockType* source, io::DevSizeT at) {
        return nullptr;
    }



    bool find_unallocated(
            size_t level,
            ProfileCtrSizeT<Profile> size,
            ArenaBuffer<AllocationMetadata<ApiProfileT>>& arena
    ) {
        init_allocator_ctr();
        return allocation_map_ctr_->find_unallocated(0, level, size, arena);
    }

    void scan_unallocated(const std::function<bool (Span<AllocationMetadataT>)>& fn) {
        init_allocator_ctr();
        return allocation_map_ctr_->scan(fn);
    }

    void check_allocation_pool(const CheckResultConsumerFn& consumer)
    {
        auto sb = get_superblock();
        AllocationPool<ApiProfileT, ALLOCATION_MAP_LEVELS> pool(0);
        pool.load(sb->allocation_pool_data());

        init_allocator_ctr();
        pool.for_each([&](const AllocationMetadataT& meta) {
            if (!allocation_map_ctr_->check_allocated(meta))
            {
                consumer(
                    CheckSeverity::ERROR,
                    make_string_document("AllocationMap entry check failure: {}", meta)
                );
            }
        });
    }


    void init_allocator_ctr()
    {
        if (!allocation_map_ctr_)
        {
            auto root_block_id = get_superblock()->allocator_root_id();
            if (root_block_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<AllocationMapCtrType>(root_block_id);

                allocation_map_ctr_ = ctr_ref;
                allocation_map_ctr_->internal_detouch_from_store();
            }
        }
    }

    void dump_allocation_map(int64_t num = -1) {
        init_allocator_ctr();
        allocation_map_ctr_->dump_leafs(num);
    }

    void check_allocation_map(
        const LiteAllocationMap<ApiProfileT>& allocations,
        const CheckResultConsumerFn& consumer
    )
    {
        init_allocator_ctr();
        allocation_map_ctr_->compare_with(allocations.ctr(), [&](auto& helper){
            consumer(CheckSeverity::ERROR, make_string_document(
                         "Allocation map mismatch at {} :: {}/{} :: {}, level {}. Expected: {}, actual: {}",
                         helper.my_base(),
                         helper.my_idx(),
                         helper.other_base(),
                         helper.other_idx(),
                         helper.level(),
                         helper.other_bit(), helper.my_bit()
            ));

            return true; // continue
        });
    }

    virtual bool is_allocated(const BlockID& block_id)
    {
        init_allocator_ctr();
        auto alc = this->get_allocation_metadata(block_id);
        return allocation_map_ctr_->check_allocated(alc);
    }

    CtrSizeT allocation_map_size() {
        init_allocator_ctr();
        return allocation_map_ctr_->size();
    }

    virtual void describe_to_cout() {
        Base::describe_to_cout();

        if (allocation_map_ctr_) {
            auto ii = allocation_map_ctr_->iterator();
            return ii->dump();
        }
    }

    virtual void check_allocation_map(const CheckResultConsumerFn& consumer) {
        init_allocator_ctr();

        allocation_map_ctr_->check(consumer);
        check_allocation_pool(consumer);

    }


    void check_storage(SharedBlockConstPtr block, const CheckResultConsumerFn& consumer)
    {
        init_allocator_ctr();
        //check_storage_specific(block, consumer);

        AllocationMetadataT alc = this->get_allocation_metadata(block->id());
        Optional<AllocationMapEntryStatus> status = allocation_map_ctr_->get_allocation_status(alc.level(), alc.position());

        if (status.has_value())
        {
            if (status.value() == AllocationMapEntryStatus::FREE) {
                consumer(
                    CheckSeverity::ERROR,
                    make_string_document("Missing allocation info for {}", alc));
            }
        }
        else {
            consumer(
                CheckSeverity::ERROR,
                make_string_document("Missing allocation info for {}", alc));
        }

        register_allocation(alc);
    }


    void add_superblock(LiteAllocationMap<ApiProfileT>& allocations)
    {
        uint64_t sb_ptr = snapshot_descriptor_->superblock_ptr() / BASIC_BLOCK_SIZE;
        allocations.append(AllocationMetadataT((CtrSizeT)sb_ptr, 1, SUPERBLOCK_ALLOCATION_LEVEL));
    }

    void add_system_blocks(LiteAllocationMap<ApiProfileT>& allocations)
    {
        auto sb = get_superblock();
        allocations.append(AllocationMetadataT{0, 2, SUPERBLOCK_ALLOCATION_LEVEL});

        allocations.append(AllocationMetadataT{
            (CtrSizeT)sb->global_block_counters_file_pos() / BASIC_BLOCK_SIZE,
            (CtrSizeT)sb->global_block_counters_blocks(),
            0
        });

        AllocationPool<ApiProfileT, ALLOCATION_MAP_LEVELS> pool;
        pool.load(sb->allocation_pool_data());

        pool.for_each([&](const AllocationMetadataT& alc){
            allocations.append(alc);
        });
    }

    virtual void handle_open_snapshot(io::BlockPtr<Superblock> sb)
    {
        auto allocator_root_id = sb->allocator_root_id();
        if (allocator_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<AllocationMapCtrType>(allocator_root_id);

            allocation_map_ctr_ = ctr_ref;
            allocation_map_ctr_->internal_detouch_from_store();
        }

    }


    virtual void handle_init_snapshot1(io::BlockPtr<Superblock> sb)
    {
        this->template internal_init_system_ctr<AllocationMapCtrType>(
            allocation_map_ctr_,
            sb->allocator_root_id(),
            AllocationMapCtrID
        );
    }




    /// Returns the number of 4K blocks we need to enlarge the bitmap to
    io::DevSizeT enlarge_file(io::DevSizeT blocks)
    {
        // Enlarging is always in the increment of highest allocation level's block size.
        io::DevSizeT blocks_4K = blocks << LAST_ALLOCATION_LEVEL;
        io::DevSizeT map_size_4K = allocation_map_ctr_ ? allocation_map_ctr_->size() : 0;

        constexpr io::DevSizeT largest_block_size = 1ull << LAST_ALLOCATION_LEVEL;
        io::DevSizeT tgt_file_size_4K = div_up(map_size_4K + blocks_4K, largest_block_size);
        io::DevSizeT new_size_4K = blockio_->resize(tgt_file_size_4K * BASIC_BLOCK_SIZE) / BASIC_BLOCK_SIZE;

        io::DevSizeT delta = new_size_4K - map_size_4K;
        return new_size_4K;
    }

    struct PreallocatedScope {
        Optional<AllocationMetadataT>& meta;
        ~PreallocatedScope () noexcept {
            meta = {};
        }
    };


    void with_preallocated(io::DevSizeT blocks_4K, size_t level)
    {
        // The following algorithm will be allocating blocks
        // for the bitmap in the newly extended portion of the file.

        PreallocatedScope scope1{preallocated_};
        io::DevSizeT base_pos = allocation_map_ctr_->size();
        preallocated_ = AllocationMetadataT::from_l0(base_pos, blocks_4K, 0);
        allocation_map_ctr_->expand(blocks_4K);

        if (!allocation_map_ctr_->populate_allocation_pool(*allocation_pool_, level)) {
            MEMORIA_MAKE_GENERIC_ERROR("Can't populate allocation pool.").do_throw();
        }

        allocation_map_ctr_->setup_bits(awaiting_init_allocations_.span(), 1);
        awaiting_init_allocations_.clear();
    }

    void populate_allocation_pool(size_t level)
    {
        if (populating_allocation_pool_) {
            MEMORIA_MAKE_GENERIC_ERROR("Internal Error: populate_allocation_pool() re-entry.").do_throw();
        }

        FlagScope scope0(populating_allocation_pool_);

        if (!allocation_map_ctr_->populate_allocation_pool(*allocation_pool_, level))
        {
            io::DevSizeT base_pos = allocation_map_ctr_->size();
            io::DevSizeT blocks_4K = enlarge_file(
                this->store_size_increment_
            );

            if (MMA_LIKELY(level == 0)) {
                with_preallocated(blocks_4K, level);
            }
            else {
                io::DevSizeT available_4K = allocation_map_ctr_->unallocated_at(0);
                if (available_4K < 1024) {
                    with_preallocated(blocks_4K, level);
                }
                else {
                    allocation_map_ctr_->expand(blocks_4K);

                    if (!allocation_map_ctr_->populate_allocation_pool(*allocation_pool_, level)) {
                        MEMORIA_MAKE_GENERIC_ERROR("Can't populate allocation pool.").do_throw();
                    }
                }
            }
        }
    }

    io::BlockPtr<Superblock> allocate_superblock(
            const Superblock* parent_sb,
            const SnapshotID& snapshot_id
    ){
        AllocationMetadataT allocation;

        if (MMA_LIKELY(!preallocated_)) {
            allocation = do_allocate_reserved(0);
        }
        else {
            // Will be taken from the preallocated pool
            allocation = allocate_one_or_throw(0);
        }

        io::DevSizeT pos = (allocation.position() << SUPERBLOCK_ALLOCATION_LEVEL) * BASIC_BLOCK_SIZE;

        auto superblock = new_superblock(pos);
        if (parent_sb) {
            superblock->init_from(*parent_sb, snapshot_id);
        }
        else {
            superblock->init(snapshot_id, 1, 1);
        }
        superblock->build_superblock_description();

        return superblock;
    }

    AllocationMetadataT do_allocate_reserved(int64_t remainder)
    {
        auto alc = allocation_pool_->allocate_reserved(remainder);
        if (alc) {
            return alc.value();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Empty reserved allocation pool").do_throw();
        }
    }


    virtual AllocationMetadataT allocate_one_or_throw(size_t level = 0)
    {
        if (MMA_UNLIKELY(forbid_allocations_)) {
            MEMORIA_MAKE_GENERIC_ERROR("Internal error. Block allocations are fordidden at this stage.").do_throw();
        }
        else if (MMA_UNLIKELY(preallocated_ && level == 0))
        {
            AllocationMetadataT& alc = preallocated_.value();
            if (alc.size1())
            {
                auto alc0 = alc.take(1);
                awaiting_init_allocations_.push_back(alc0);
                return alc0;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Preallocated pool is empty").do_throw();
            }
        }
        else if (MMA_LIKELY(!allocate_reserved_))
        {
            auto alc = allocation_pool_->allocate_one(level);
            if (alc)
            {
                return alc.value();
            }
            else if (populating_allocation_pool_) {
                // Level must be 0 here
                return do_allocate_reserved(1);
            }
            else {
                // FIXME: More clever logic is needed here
                // Current pool population algirithm is suboptimal.
                // In case when some blocks at the current level
                // are reserved (like 32 blocks for the level 0),
                // and we are populating the pool,
                // populate_allocation_pool() may return with only
                // reserved blocks are populated-in, so subsequent
                // allocate_one() will return empty elment.

                // It's safe to call populate_allocation_pool(lvl)
                // mutiple times in a row, so it will be filling
                // the pool each time. But, ideally, this logic should be
                // in the populate_allocation_pool(lvl);

                for (size_t cc = 0; cc < 256; cc++) {
                    populate_allocation_pool(level);
                    auto alc1 = allocation_pool_->allocate_one(level);
                    if (alc1) {
                        return alc1.value();
                    }
                }

                // This situation sould be extremly unlikely. But it doesn't mean
                // the we have no space in the data file.
                MEMORIA_MAKE_GENERIC_ERROR("FIXME: Tried multiple times to allocate from the pool and failed.").do_throw();
            }
        }
        else {
            // Level must be 0 here
            return do_allocate_reserved(1);
        }
    }

    virtual void flush_buffers()
    {
        // FIXME: "touching" deallocation's bits is not necessary here.
        ArenaBuffer<AllocationMetadataT> all_postponed_deallocations;
        while (postponed_deallocations_.size() > 0 || eviction_queue_buf_.size() > 0)
        {
            flush_eviction_queue_buffer();

            if (postponed_deallocations_.size())
            {
                ArenaBuffer<AllocationMetadataT> postponed_deallocations_tmp;
                postponed_deallocations_tmp.push_back(postponed_deallocations_.span());

                all_postponed_deallocations.push_back(postponed_deallocations_.span());
                postponed_deallocations_.clear();

                // Just CoW allocation map's leafs that bits will be cleared later
                postponed_deallocations_tmp.sort();
                allocation_map_ctr_->touch_bits(postponed_deallocations_tmp.span());
            }
        }

        if (all_postponed_deallocations.size() > 0)
        {
            // Mark blocks as free
            allocation_map_ctr_->setup_bits(all_postponed_deallocations.span(), false);
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



    virtual void trim_eviction_queue()
    {
        uint64_t oldest_reader = find_oldest_reader();

        ArenaBuffer<AllocationMetadataT> buffer;

        auto ii = this->evc_queue_ctr_->first_entry();
        uint64_t entries{};
        while(is_valid_chunk(ii))
        {
            for (auto val: ii->keys())
            {
                if (MMA_LIKELY(!this->is_txnid(val)))
                {
                    buffer.push_back(AllocationMetadataT::from_raw(val));
                    if (MMA_UNLIKELY(buffer.size() > 10000)) {
                        flush_buffer(buffer);
                    }
                }
                else {
                    auto txn_id = this->dencode_txnid(val);
                    if (MMA_UNLIKELY(txn_id >= oldest_reader)) {
                        break;
                    }
                }

                entries++;
            }

            ii = ii->next_chunk();
        }

        flush_buffer(buffer);

        evc_queue_ctr_->remove_up_to(entries);
    }

    void flush_buffer(ArenaBuffer<AllocationMetadataT>& buffer)
    {
        size_t c;
        for (c = 0; c < buffer.size(); c++) {
            if (!allocation_pool_->add(buffer[buffer.size() - c - 1])) {
                break;
            }
        }

        buffer.erase(buffer.begin() + c, buffer.end());
        allocation_map_ctr_->setup_bits(buffer.span(), true);
        buffer.clear();
    }

    void register_allocation(const AllocationMetadataT&) {}

    void add_postponed_deallocation(const AllocationMetadataT& meta)  {
        postponed_deallocations_.push_back(meta);
    }

    virtual void init_store_snapshot()
    {
        io::DevSizeT blocks_4K = enlarge_file(1 << LAST_ALLOCATION_LEVEL);

        preallocated_ = AllocationMetadataT::from_l0(0, blocks_4K, 0);

        //allocation_pool_->add(AllocationMetadataT(0, file_size, ALLOCATION_MAP_LEVELS - 1));

        // History (2 4K blocks at the start of the store)
        allocate_one_or_throw(0);
        allocate_one_or_throw(0);

        SnapshotID snapshot_id = ProfileTraits<Profile>::make_random_snapshot_id();

        auto sb = allocate_superblock(nullptr, snapshot_id);

        snapshot_descriptor_->set_superblock(sb.get());

        this->template internal_init_system_ctr<EvcQueueCtrType>(
            evc_queue_ctr_,
            sb->evc_queue_root_id(),
            EvcQueueCtrID
        );

        this->template internal_init_system_ctr<DirectoryCtrType>(
            directory_ctr_,
            sb->directory_root_id(),
            DirectoryCtrID
        );

        finish_store_initialization(blocks_4K);
    }

    virtual void finish_store_initialization(io::DevSizeT blocks_4K)
    {
        allocation_map_ctr_ = this->template internal_create_by_name_typed<AllocationMapCtrType>(AllocationMapCtrID);

        allocation_map_ctr_->expand(blocks_4K);

        // FIXME: This should not trigger any block allocation
        allocation_map_ctr_->setup_bits(awaiting_init_allocations_.span(), true);

        allocation_pool_->clear();

        preallocated_ = {};

        populate_allocation_pool(0);

        return commit(ConsistencyPoint::YES);
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

    virtual SharedBlockPtr allocate_block(size_t level)
    {
        AllocationMetadataT allocation = allocate_one_or_throw(level);
        uint64_t position = allocation.position();

        return {};
    }

    virtual SharedBlockPtr clone_block(const SharedBlockConstPtr& block) {

        size_t block_size = block->memory_block_size();
        size_t scale_factor = block_size / BASIC_BLOCK_SIZE;
        size_t level = CustomLog2(scale_factor);

        AllocationMetadataT allocation = allocate_one_or_throw(level);
        uint64_t position = allocation.position();

        auto shared = allocate_block_from(block.block(), position);
        BlockType* new_block = shared->get();
        return shared;
    }


    virtual void remove_block(const BlockID& id)
    {
        // FIXME: Reenterabilty!
        // We are updting either allocation map or eviction queue
        // Put ID into postponed deallocations if we
        // have to put the id there but any update operation on
        // those containers are currently in flight.

        // Putting the ide to postponed_deallocations
        // and postponed_evictions are OK

        auto block_alc = resolve_block_allocation(id);
        size_t level = block_alc.level();
        io::DevSizeT ll_allocator_block_pos = block_alc.position() >> block_alc.level();

        auto ii = my_blocks_.find(id);
        if (ii != my_blocks_.end()) {
            my_blocks_.erase(ii);
            if (!allocation_pool_->add(block_alc)) {
                // TODO: put the block directly into the allocation_map_ctr_
            }
        }
        else {
            this->push_to_eviction_queue(id);
        }
    }


    virtual void flush_allocations(io::BlockPtr<Superblock> sb)
    {
        allocation_pool_->store(sb->allocation_pool_data());

        if (MMA_UNLIKELY(allocation_pool_->level0_total() < 1))
        {
            // We must ensure that at the end of a snapshot, at least
            // one preallocated block must be in the superblock's pool.
            MEMORIA_MAKE_GENERIC_ERROR(
                "Superblock's allocation pool is empty at the end of the snapshot"
            ).do_throw();
        }
    }

    // =================================================================================
};

}
