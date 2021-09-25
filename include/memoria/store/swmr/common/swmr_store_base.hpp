
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

#include <memoria/store/swmr/common/swmr_store_counters.hpp>
#include <memoria/store/swmr/common/swmr_store_smart_ptr.hpp>
#include <memoria/store/swmr/common/swmr_store_readonly_snapshot_base.hpp>
#include <memoria/store/swmr/common/swmr_store_writable_snapshot_base.hpp>

#include <memoria/store/swmr/common/swmr_store_history_view.hpp>
#include <memoria/store/swmr/common/swmr_store_history_tree.hpp>

#include <memoria/store/swmr/common/lite_allocation_map.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <mutex>
#include <unordered_set>
#include <functional>

namespace memoria {

namespace detail {

struct FileLockHandler {
    virtual ~FileLockHandler() noexcept;
    virtual void unlock() = 0;

    static Result<std::unique_ptr<FileLockHandler>> lock_file(const char* name, bool create) noexcept;
};

}

template <typename Profile>
class SWMRStoreBase: public ISWMRStore<ApiProfile<Profile>>, public ReferenceCounterDelegate<Profile> {

protected:
    using Base = ISWMRStore<ApiProfile<Profile>>;

    using typename Base::ReadOnlySnapshotPtr;
    using typename Base::WritableSnapshotPtr;
    using typename Base::SnapshotID;
    using typename Base::SequenceID;

    using ApiProfileT = ApiProfile<Profile>;

    using WritableSnapshotT = SWMRStoreWritableSnapshotBase<Profile>;

    using SWMRReadOnlySnapshotPtr     = SnpSharedPtr<SWMRStoreReadOnlySnapshotBase<Profile>>;
    using SWMRWritableSnapshotPtr     = SnpSharedPtr<WritableSnapshotT>;
    using SWMRWritableSnapshotWeakPtr = SnpWeakPtr<SWMRStoreWritableSnapshotBase<Profile>>;

    using SnapshotDescriptorT = SnapshotDescriptor<Profile>;
    using CDescrPtr = typename SnapshotDescriptorT::SharedPtrT;

    using BlockID = ProfileBlockID<Profile>;
    using CtrID = ProfileCtrID<Profile>;

    using SuperblockT = SWMRSuperblock<Profile>;
    using CounterBlockT = SWMRCounterBlock<Profile>;

    using CounterStorageT = CounterStorage<Profile>;
    using RemovingBlockConsumerFn = std::function<void (const BlockID&, const AllocationMetadata<ApiProfileT>&)>;

    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;

public:
    static constexpr size_t  BASIC_BLOCK_SIZE = 4096;

protected:
    static constexpr size_t  HEADER_SIZE = BASIC_BLOCK_SIZE * 2;
    static constexpr int32_t ALLOCATION_MAP_LEVELS    = ICtrApi<AllocationMap, ApiProfileT>::LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP = ICtrApi<AllocationMap, ApiProfileT>::ALLOCATION_SIZE * BASIC_BLOCK_SIZE;
    static constexpr size_t  MB = 1024*1024;

    static_assert(sizeof(SWMRSuperblock<Profile>) <= 4096, "Check superblock size!");

    AllocationPool<ApiProfileT, ALLOCATION_MAP_LEVELS> allocation_pool_;

    mutable std::recursive_mutex history_mutex_;
    mutable std::recursive_mutex writer_mutex_;

    using LockGuard     = std::lock_guard<std::recursive_mutex>;

    HistoryTree<Profile> history_tree_;

    std::unordered_set<SnapshotID> removing_persistent_snapshots_;

    template <typename> friend class SWMRStoreReadonlySnapshotBase;
    template <typename> friend class SWMRStoreWritableSnapshotBase;
    template <typename> friend class SWMRStoreSnapshotBase;

    SWMRBlockCounters<Profile> block_counters_;

    bool read_only_{false};

    uint64_t cp_allocation_threshold_{100 * 1024 * 1024 / 4096};
    uint64_t cp_snapshots_threshold_{10000};
    uint64_t cp_timeout_{1000}; // 1 secons

    LDDocument store_params_;

    bool active_writer_{false};

    std::unique_ptr<LiteAllocationMap<ApiProfileT>> allocations_;

public:
    using Base::flush;

    SWMRStoreBase() noexcept {
        history_tree_.set_superblock_fn([&](uint64_t file_pos){
            return this->get_superblock(file_pos * BASIC_BLOCK_SIZE);
        });
    }

    auto& allocation_pool() noexcept {
        return allocation_pool_;
    }

    auto& history_tree() noexcept {
        return history_tree_;
    }

    auto& history_mutex() noexcept {
        return history_mutex_;
    }

    uint64_t cp_allocation_threshold() const override {
        LockGuard lock(history_mutex_);
        return cp_allocation_threshold_;
    }

    uint64_t cp_snapshot_threshold() const override {
        LockGuard lock(history_mutex_);
        return cp_snapshots_threshold_;
    }

    uint64_t set_cp_allocation_threshold(uint64_t value) override {
        LockGuard lock(history_mutex_);
        auto tmp = cp_allocation_threshold_;
        cp_allocation_threshold_ = value;
        return tmp;
    }

    uint64_t set_cp_snapshot_threshold(uint64_t value) override {
        LockGuard lock(history_mutex_);
        auto tmp = cp_snapshots_threshold_;
        cp_snapshots_threshold_ = value;
        return tmp;
    }

    int64_t cp_timeout() const override {
        LockGuard lock(history_mutex_);
        return cp_timeout_;
    }

    int64_t set_cp_timeout(int64_t value) override
    {
        LockGuard lock(history_mutex_);
        auto tmp = cp_timeout_;
        cp_timeout_ = value;
        return tmp;
    }

    virtual void close() override = 0;

    virtual void flush_data(bool async = false) = 0;
    virtual void flush_header(bool async = false) = 0;

    virtual void check_if_open() = 0;

    virtual SWMRReadOnlySnapshotPtr do_open_readonly(CDescrPtr snapshot_descr) = 0;
    virtual SWMRWritableSnapshotPtr do_create_writable(
            CDescrPtr consistency_point,
            CDescrPtr head,
            CDescrPtr parent,
            CDescrPtr snapshot_descr
    ) = 0;

    virtual SWMRWritableSnapshotPtr do_open_writable(
            CDescrPtr snapshot_descr,
            RemovingBlockConsumerFn fn, bool force = false) = 0;

    virtual SWMRWritableSnapshotPtr do_create_writable_for_init(CDescrPtr snapshot_descr) = 0;
    virtual SharedPtr<SWMRStoreBase<Profile>> self_ptr() noexcept = 0;
    virtual void store_superblock(SuperblockT* superblock, uint64_t sb_slot) = 0;
    virtual SharedSBPtr<SuperblockT> get_superblock(uint64_t file_pos) = 0;
    virtual SharedSBPtr<CounterBlockT> get_counter_block(uint64_t file_pos) = 0;
    virtual uint64_t buffer_size() = 0;

    virtual void do_flush() = 0;

    virtual ReadOnlySnapshotPtr flush(FlushType ft) override
    {
        LockGuard lock(writer_mutex_);
        check_if_open();

        bool clean;
        {
            LockGuard rlock(history_mutex_);
            clean = history_tree_.is_clean();
        }

        if (!clean)
        {
            auto ii = create_system_snapshot();
            ii->commit(ConsistencyPoint::YES);
            return DynamicPointerCast<ISWMRStoreReadOnlySnapshot<ApiProfileT>>(ii);
        }
        else {
            return ReadOnlySnapshotPtr{};
        }
    }

    virtual Optional<std::vector<SnapshotID>> snapshots(U8StringView branch) override
    {
        using ResultT = Optional<std::vector<SnapshotID>>;

        check_if_open();
        LockGuard lock(history_mutex_);

        auto head_opt = history_tree_.get_branch_head(branch);
        if (head_opt)
        {
            std::vector<SnapshotID> snapshots;

            CDescrPtr node = head_opt.get();
            while (node) {
                snapshots.push_back(node->snapshot_id());
                node = node->parent();
            }

            return ResultT{snapshots};
        }
        else {
            return ResultT{};
        }
    }

    virtual Optional<SnapshotID> parent(const SnapshotID& snapshot_id) override
    {
        using ResultT = Optional<SnapshotID>;

        check_if_open();
        LockGuard lock(history_mutex_);

        auto descr_opt = history_tree_.get(snapshot_id);
        if (descr_opt)
        {
            CDescrPtr descr = descr_opt.get();
            if (descr->parent()) {
                return ResultT{descr->parent()->snapshot_id()};
            }
            else {
                return ResultT{};
            }
        }
        else {
            return ResultT{};
        }
    }

    virtual Optional<bool> is_transient(const SnapshotID& snapshot_id) override
    {
        using ResultT = Optional<bool>;

        check_if_open();
        LockGuard lock(history_mutex_);

        auto descr_opt = history_tree_.get(snapshot_id);
        if (descr_opt)
        {
            CDescrPtr descr = descr_opt.get();
            return ResultT{descr->is_transient()};
        }
        else {
            return ResultT{};
        }
    }

    virtual Optional<bool> is_system_snapshot(const SnapshotID& snapshot_id) override
    {
        using ResultT = Optional<bool>;

        check_if_open();
        LockGuard lock(history_mutex_);

        auto descr_opt = history_tree_.get(snapshot_id);
        if (descr_opt)
        {
            CDescrPtr descr = descr_opt.get();
            return ResultT{descr->is_system_snapshot()};
        }
        else {
            return ResultT{};
        }
    }


    virtual Optional<std::vector<SnapshotID>> children(const SnapshotID& snapshot_id) override
    {
        using ResultT = Optional<std::vector<SnapshotID>>;

        check_if_open();
        LockGuard lock(history_mutex_);

        auto descr_opt = history_tree_.get(snapshot_id);
        if (descr_opt)
        {
            std::vector<SnapshotID> snapshots;

            for (auto chl: descr_opt.get()->children()) {
                snapshots.push_back(chl->snapshot_id());
            }

            return ResultT{snapshots};
        }
        else {
            return ResultT{};
        }
    }

    virtual ReadOnlySnapshotPtr open(const SnapshotID& snapshot_id, bool open_transient_snapshots) override
    {
        check_if_open();
        if (!open_transient_snapshots) {
            return open_persistent(snapshot_id);
        }
        else
        {
            LockGuard wlock(writer_mutex_);

            auto snapshot = [&]{
                LockGuard rlock(history_mutex_);
                return history_tree_.get(snapshot_id);
            }();

            if (snapshot) {
                return open_readonly(snapshot.get());
            }
            else {
                make_generic_error_with_source(MA_SRC, "Can't find snapshot {}", snapshot_id).do_throw();
            }
        }
    }



    ReadOnlySnapshotPtr open_persistent(const SnapshotID& snapshot_id)
    {
        auto snapshot = [&]{
            LockGuard lock(history_mutex_);
            return history_tree_.get(snapshot_id);
        }();

        if (snapshot)
        {
            if (!snapshot.get()->is_transient()) {
                return open_readonly(snapshot.get());
            }
            else {
                make_generic_error_with_source(MA_SRC, "Requested snapshot {} is not persistent", snapshot_id).do_throw();
            }
        }
        else {
            make_generic_error_with_source(MA_SRC, "Can't find snapshot {}", snapshot_id).do_throw();
        }
    }



    virtual ReadOnlySnapshotPtr open() override {
        return open("main");
    }


    virtual ReadOnlySnapshotPtr open(U8StringView branch) override
    {
        check_if_open();

        auto descr = get_branch_head_read_sync("main");

        if (descr) {
            return do_open_readonly(descr.get());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Branch '{}' is not found", branch).do_throw();
        }
    }

    virtual std::vector<U8String> branches() override
    {
        check_if_open();
        LockGuard lock(history_mutex_);
        return history_tree_.branch_names();
    }

    virtual int64_t count_volatile_snapshots() override {
        LockGuard lock(history_mutex_);
        return history_tree_.count_volatile_snapshots();
    }

    virtual WritableSnapshotPtr begin() override {
        return begin("main");
    }


    virtual WritableSnapshotPtr begin(U8StringView branch) override {
        return branch_from("main", branch);
    }

    CDescrPtr create_snapshot_descriptor(U8StringView branch_name)
    {
        auto snapshot_descriptor = history_tree_.new_snapshot_descriptor(branch_name);

        snapshot_descriptor->set_start_time(getTimeInMillis());

        {
            LockGuard lock(history_mutex_);
            snapshot_descriptor->set_allocated_basic_blocks_threshold(cp_allocation_threshold_);
            snapshot_descriptor->set_snapshots_threshold(cp_snapshots_threshold_);
            snapshot_descriptor->set_cp_timeout(cp_timeout_);
        }

        return snapshot_descriptor;
    }

    void check_no_writers()
    {
        if (MMA_UNLIKELY(active_writer_)) {
            writer_mutex_.unlock();
            MEMORIA_MAKE_GENERIC_ERROR("Another writer snapshot is still active. Commit/rollback it first.").do_throw();
        }
        active_writer_ = 1;
    }

    void unlock_writer() {
        active_writer_ = 0;
        writer_mutex_.unlock();
    }

    virtual WritableSnapshotPtr branch_from(U8StringView source, U8StringView branch_name) override
    {
        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();

        check_no_writers();

        try {
            auto descr = get_branch_head_read_sync(source);
            if (descr)
            {
                CDescrPtr snapshot_descriptor = create_snapshot_descriptor(branch_name);

                SWMRWritableSnapshotPtr ptr = do_create_writable(
                            history_tree_.consistency_point1(),
                            history_tree_.head(),
                            descr,
                            std::move(snapshot_descriptor)
                );



                ptr->finish_snapshot_opening();
                return ptr;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Requested branch '{}' does not exist", branch_name).do_throw();
            }
        }
        catch (...) {
            unlock_writer();
            throw;
        }
    }

    virtual WritableSnapshotPtr branch_from(const SnapshotID& snapshot_id, U8StringView branch_name) override
    {
        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();
        check_no_writers();

        try {
            auto descr = [&]{
                LockGuard lock(history_mutex_);
                return history_tree_.get(snapshot_id);
            }();

            if (descr)
            {
                if (!descr.get()->is_transient())
                {
                    CDescrPtr snapshot_descriptor = create_snapshot_descriptor(branch_name);

                    SWMRWritableSnapshotPtr ptr = do_create_writable(
                                history_tree_.consistency_point1(),
                                history_tree_.head(),
                                descr,
                                std::move(snapshot_descriptor)
                    );
                    ptr->finish_snapshot_opening();
                    return ptr;
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Requested snapshot '{}' is not persistent", snapshot_id).do_throw();
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Requested branch '{}' does not exist", branch_name).do_throw();
            }
        }
        catch (...) {
            unlock_writer();
            throw;
        }
    }


    // FIXME: needed only for flush
    WritableSnapshotPtr create_system_snapshot()
    {
        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();

        check_no_writers();

        try {
            LockGuard lock(history_mutex_);

            U8String last_branch = history_tree_.head()->branch();
            CDescrPtr snapshot_descriptor = create_snapshot_descriptor(last_branch);
            snapshot_descriptor->set_system_snapshot(true);

            SWMRWritableSnapshotPtr ptr = do_create_writable(
                        history_tree_.consistency_point1(),
                        history_tree_.head(),
                        history_tree_.head(),
                        std::move(snapshot_descriptor)
            );

            ptr->finish_snapshot_opening();
            return ptr;
        }
        catch (...) {
            unlock_writer();
            throw;
        }
    }



    virtual void do_prepare(
            CDescrPtr snapshot_descriptor,
            ConsistencyPoint cp,
            bool do_consistency_point,
            SharedSBPtr<SuperblockT> sb
    )
    {
        if (do_consistency_point)
        {
            sb->inc_consistency_point_sequence_id();

            if (cp == ConsistencyPoint::FULL) {
                store_counters(sb.get());
                sb->set_clean_status();
            }

            sb->build_superblock_description();

            flush_data();

            auto sb_slot = sb->consistency_point_sequence_id() % 2;
            store_superblock(sb.get(), sb_slot);

            auto sb0 = get_superblock(sb_slot * BASIC_BLOCK_SIZE);
            sb0->set_metadata_doc(store_params_);
            store_superblock(sb0.get(), sb_slot);

            flush_header();
        }
        else {
            sb->build_superblock_description();
        }
    }

    virtual void do_commit(
        CDescrPtr snapshot_descriptor,
        bool do_consistency_point,
        WritableSnapshotT* snapshot
    )
    {
        for (const auto& entry: snapshot->counters()) {
            block_counters_.apply(entry.first, entry.second.value);
        }

        {
            LockGuard lock(history_mutex_);

            // We need to cleanup eviction queue before attaching the new snapshot
            // to the history tree. Because attaching may add a new entry to
            // the queue.
            cleanup_eviction_queue();

            history_tree_.attach_snapshot(snapshot_descriptor, do_consistency_point);

            for (const auto& snapshot_id: snapshot->removing_snapshots())
            {
                auto descr = history_tree_.get(snapshot_id);
                if (descr) {
                    descr->set_transient(true);
                }
            }

            for (const auto& branch_name: snapshot->removing_branches()) {
                history_tree_.remove_branch(branch_name);
            }
        }

        unlock_writer();
    }

    virtual void do_rollback(CDescrPtr snapshot_descriptor)
    {
        unlock_writer();
    }


    SharedPtr<ISWMRStoreHistoryView<ApiProfileT>> history_view() override
    {
        check_if_open();

        LockGuard lock(history_mutex_);
        auto head = do_open_readonly(history_tree_.consistency_point1());

        auto view = MakeShared<SWMRStoreHistoryViewImpl<Profile>>();

        head->for_each_history_entry_batch([&](auto snapshot, auto metas){
            view->load(snapshot, metas);
        });

        for (auto& name: history_tree_.branch_names())
        {
            CDescrPtr head = history_tree_.get_branch_head(name);
            view->load_branch(name, head->snapshot_id());
        }

        view->build_tree();

        return view;
    }

    ReadOnlySnapshotPtr open_readonly(CDescrPtr snapshot_descr) {
        return do_open_readonly(snapshot_descr);
    }


    virtual Optional<SequenceID> check(const CheckResultConsumerFn& consumer) override {
        check_if_open();
        return do_check(consumer);
    }

    void for_all_evicting_snapshots(std::function<void (SnapshotDescriptorT*)> fn)
    {
        LockGuard lock(history_mutex_);
        for (auto& descr: history_tree_.eviction_queue()) {
            fn(&descr);
        }
    }

    void register_allocation(const AllocationMetadataT& alc)
    {
        if (allocations_) {
            allocations_->append(alc);
        }
    }

protected:

    std::vector<SWMRReadOnlySnapshotPtr> build_ordered_snapshots_list(){
        std::vector<SWMRReadOnlySnapshotPtr> snapshot;

        history_tree_.traverse_tree_preorder([&](CDescrPtr descr){
            auto ptr = do_open_readonly(descr);
            snapshot.push_back(ptr);
        });

        std::sort(snapshot.begin(), snapshot.end(), [&](const auto& one, const auto& two) -> bool {
            return one->sequence_id() < two->sequence_id();
        });

        return snapshot;
    }

    Optional<SequenceID> do_check(const CheckResultConsumerFn& consumer)
    {
        // FIXME:
        // Writer mutex is needed because we are touching counters here.
        // Need another lightweight method to check the data only without
        // taking writer lock

        LockGuard lock(writer_mutex_);

        allocations_ = make_lite_allocation_map();

        auto snapshots = build_ordered_snapshots_list();

        if (snapshots.size() > 0)
        {
            SnapshotCheckState<Profile> check_state;

            for (auto& snapshot: snapshots)
            {
                snapshot->add_superblock(*allocations_.get());
                snapshot->check(consumer);
                snapshot->build_block_refcounters(check_state.counters);
            }

            check_refcounters(check_state.counters, consumer);

            do_check_allocations(consumer);

            allocations_->close();
            allocations_.reset();
            return snapshots[snapshots.size() - 1]->sequence_id();
        }
        else {
            allocations_->close();
            allocations_.reset();
            return Optional<SequenceID>{};
        }
    }

    std::unique_ptr<LiteAllocationMap<ApiProfileT>> make_lite_allocation_map()
    {
        auto ptr = do_open_readonly(history_tree_.head());
        return std::make_unique<LiteAllocationMap<ApiProfileT>>(ptr->allocation_map_size());
    }

    void check_refcounters(const SWMRBlockCounters<Profile>& counters, const CheckResultConsumerFn& consumer)
    {
        block_counters_.diff(counters);

        if (counters.size() != block_counters_.size())
        {
            LDDocument doc;
            doc.set_varchar(fmt::format(
                        "Check failure: mismatched number of counters. Expected: {}, actual: {}",
                        block_counters_.size(),
                        counters.size()));

            return consumer(CheckSeverity::ERROR, doc);
        }

        counters.for_each([&](const BlockID& block_id, uint64_t counter) {
            auto res = block_counters_.get(block_id);

            if (res) {
                if (res.get() != counter) {
                    LDDocument doc;
                    doc.set_varchar(fmt::format(
                                "Counter values mismatch for the block ID {}. Expected: {}, actual: {}",
                                block_id,
                                res.get(),
                                counter));

                    return consumer(CheckSeverity::ERROR, doc);
                }
            }
            else {
                LDDocument doc;
                doc.set_varchar(fmt::format(
                            "Actual counter for the block ID {} is not found in the store's block_counters structure.",
                            block_id
                            ));

                return consumer(CheckSeverity::ERROR, doc);
            }
        });
    }

    void do_check_allocations(const CheckResultConsumerFn& consumer)
    {
        auto ptr = do_open_readonly(history_tree_.head());
        ptr->add_system_blocks(*allocations_.get());
        ptr->check_allocation_map(*allocations_.get(), consumer);
    }



    virtual void ref_block(const BlockID& block_id) override {
        block_counters_.inc(block_id);
    }

    virtual void unref_block(const BlockID& block_id, const std::function<void ()>& on_zero) override {
        auto zero = block_counters_.dec(block_id);
        if (zero) {
            return on_zero();
        }
    }

    virtual Optional<int64_t> count_refs(const BlockID& block_id) override {
        return block_counters_.get(block_id);
    }


    void cleanup_eviction_queue() {
        history_tree_.cleanup_eviction_queue();
    }

    static bool is_my_block(const uint8_t* mem_block) noexcept {
        const SuperblockT* sb0 = ptr_cast<SuperblockT>(mem_block);
        return sb0->match_magick() && sb0->match_profile_hash();
    }

    void throw_if_read_only() {
        if (read_only_) {
            MEMORIA_MAKE_GENERIC_ERROR("The store is in the read-only mode").do_throw();
        }
    }

    uint64_t count_refs(const AnyID& block_id) override
    {
        LockGuard lock(writer_mutex_);

        BlockID id = cast_to<BlockID>(block_id);
        auto val = block_counters_.get(id);

        if (val) {
            return val.get();
        }
        else {
            return 0;
        }
    }

    void traverse(SWMRStoreGraphVisitor<ApiProfileT>& visitor) override
    {
        LockGuard lock(history_mutex_);

        using VisitedBlocks = std::unordered_set<BlockID>;
        using CtrType = typename SWMRStoreGraphVisitor<ApiProfileT>::CtrType;

        VisitedBlocks visited_blocks;

        visitor.start_graph();

        std::unordered_set<BlockID> visited_nodes;

        history_tree_.traverse_tree_preorder([&](CDescrPtr snapshot_descr){
            visitor.start_snapshot(snapshot_descr->snapshot_id(), snapshot_descr->sequence_id());

            auto snapshot = do_open_readonly(snapshot_descr);
            auto sb = get_superblock(snapshot_descr->superblock_ptr());

            if (sb->blockmap_root_id().is_set()) {
                snapshot->traverse_ctr_cow_tree(sb->blockmap_root_id(), visited_blocks, visitor, CtrType::BLOCKMAP);
            }

            snapshot->traverse_ctr_cow_tree(sb->allocator_root_id(), visited_blocks, visitor, CtrType::ALLOCATOR);
            snapshot->traverse_ctr_cow_tree(sb->history_root_id(), visited_blocks, visitor, CtrType::HISTORY);
            snapshot->traverse_cow_containers(visited_blocks, visitor);

            visitor.end_snapshot();
        });

        visitor.end_graph();
    }


    uint64_t count_blocks(const BlockID& block_id)
    {
        auto ii = block_counters_.get(block_id);
        return boost::get_optional_value_or(ii, 0ull);
    }


    std::vector<typename HistoryTree<Profile>::UpdateOp> prepare_eviction()
    {
        using UpdateOp = typename HistoryTree<Profile>::UpdateOp;

        LockGuard lock(history_mutex_);
        std::vector<UpdateOp> data;

        history_tree_.prepare_eviction([&](const UpdateOp& op){
            data.push_back(op);
        });

        return data;
    }


protected:
    virtual void store_counters(SuperblockT* superblock)
    {
        uint64_t counters_block_pos = superblock->global_block_counters_file_pos();
        uint64_t counter_block_size = BASIC_BLOCK_SIZE * 1 << (ALLOCATION_MAP_LEVELS - 1);

        SharedSBPtr<CounterBlockT> last;
        for (auto cntr_ii = block_counters_.begin(); cntr_ii != block_counters_.end();)
        {
            if (last) {
                last->set_next_block_pos(counters_block_pos);
                last.flush();
            }

            auto blk = get_counter_block(counters_block_pos);
            blk->init(counter_block_size);

            while (blk->available() && cntr_ii != block_counters_.end()) {
                blk->add_counter(CounterStorageT{
                    cntr_ii->first,
                    cntr_ii->second.value
                });

                ++cntr_ii;
            }

            counters_block_pos += counter_block_size;
            last = blk;
        }

        if (last) {
            last.flush();
        }

        superblock->set_global_block_counters_size(block_counters_.size());
    }



    virtual void prepare_to_close()
    {
        if (!this->active_writer_) {

        flush(FlushType::DEFAULT);

        CDescrPtr head_ptr = history_tree_.consistency_point1();
        if (head_ptr && !read_only_)
        {
            auto sb_slot = head_ptr->consistency_point_sequence_id() % 2;
            SharedSBPtr<SuperblockT> sb0 = get_superblock(sb_slot * BASIC_BLOCK_SIZE);
            sb0->set_metadata_doc(store_params_);

            store_counters(sb0.get());

            flush_data();

            sb0->set_clean_status();
            sb0->build_superblock_description();
            store_superblock(sb0.get(), sb_slot);

            flush_header();

            block_counters_.clear();
        }
        }
    }


    virtual void do_open_store()
    {
        SharedSBPtr<SuperblockT> sb0 = get_superblock(0);
        SharedSBPtr<SuperblockT> sb1 = get_superblock(BASIC_BLOCK_SIZE);

        if (!sb0->match_magick()) {
            MEMORIA_MAKE_GENERIC_ERROR("First SWMR store header magick number mismatch: {}, expected {};  {}, expected {}",
                                       sb0->magick1(), SuperblockT::MAGICK1,
                                       sb0->magick2(), SuperblockT::MAGICK2
            ).do_throw();
        }

        if (!sb1->match_magick()) {
            MEMORIA_MAKE_GENERIC_ERROR("Second SWMR store header magick number mismatch: {}, expected {};  {}, expected {}",
                                       sb1->magick1(), SuperblockT::MAGICK1,
                                       sb1->magick2(), SuperblockT::MAGICK2
            ).do_throw();
        }

        if (!sb0->match_profile_hash()) {
            MEMORIA_MAKE_GENERIC_ERROR("First SWMR store header profile hash mismatch: {}, expected {}",
                                       sb0->profile_hash(),
                                       SuperblockT::PROFILE_HASH
            ).do_throw();
        }

        if (!sb1->match_profile_hash()) {
            MEMORIA_MAKE_GENERIC_ERROR("Second SWMR store header profile hash mismatch: {}, expected {}",
                                       sb1->profile_hash(),
                                       SuperblockT::PROFILE_HASH
            ).do_throw();
        }

        if (sb0->snapshot_id().is_null() && sb1->snapshot_id().is_null()) {
            // the file was only partially initialized, continue
            // the process with full initialization.
            return init_store();
        }

        CDescrPtr consistency_point1_ptr;
        CDescrPtr consistency_point2_ptr;

        if (sb0->consistency_point_sequence_id() > sb1->consistency_point_sequence_id())
        {
            consistency_point1_ptr = history_tree_.new_snapshot_descriptor(
                        sb0->superblock_file_pos(),
                        get_superblock(sb0->superblock_file_pos()).get(),
                        ""
            );

            store_params_ = LDDocument{sb0->cmetadata_doc()};

            if (sb1->snapshot_id().is_set())
            {
                consistency_point2_ptr = history_tree_.new_snapshot_descriptor(
                            sb1->superblock_file_pos(),
                            get_superblock(sb1->superblock_file_pos()).get(),
                            ""
                );
            }
        }
        else {
            consistency_point1_ptr = history_tree_.new_snapshot_descriptor(
                        sb1->superblock_file_pos(),
                        get_superblock(sb1->superblock_file_pos()).get(),
                        ""
            );

            store_params_ = LDDocument{sb1->cmetadata_doc()};

            if (sb0->snapshot_id().is_set())
            {
                consistency_point2_ptr = history_tree_.new_snapshot_descriptor(
                            sb0->superblock_file_pos(),
                            get_superblock(sb0->superblock_file_pos()).get(),
                            ""
                );
            }
        }

        auto cp1_sb = get_superblock(consistency_point1_ptr->superblock_ptr());

        // Read snapshot history and
        // preload all transient snapshots into the
        // eviction queue
        auto ptr = do_open_readonly(consistency_point1_ptr.get());

        using MetaT = std::pair<SnapshotID, SWMRSnapshotMetadata<ApiProfile<Profile>>>;
        std::vector<MetaT> metas;

        ptr->for_each_history_entry([&](const auto& snapshot_id, const auto& snapshot_meta) {
            metas.push_back(MetaT(snapshot_id, snapshot_meta));
        });

        history_tree_.load(metas, consistency_point1_ptr.get(), consistency_point2_ptr.get());

        if (!read_only_)
        {
            if (cp1_sb->is_clean()) {
                read_block_counters();
            }
            else {
                rebuild_block_counters();
            }
        }
    }


    void read_block_counters()
    {
        CDescrPtr head_ptr = history_tree_.consistency_point1();
        auto head_sb = get_superblock(head_ptr->superblock_ptr());

        auto ctr_file_pos = head_sb->global_block_counters_file_pos();
        auto size = head_sb->global_block_counters_size();

        for (uint64_t cnt = 0; cnt < size;)
        {
            auto blk = get_counter_block(ctr_file_pos);

            for (const auto& ctr_storage: blk->counters()) {
                block_counters_.set(ctr_storage.block_id, ctr_storage.counter);
            }

            cnt += blk->size();
            ctr_file_pos = blk->next_block_pos();
        }
    }

    void rebuild_block_counters() noexcept {
        auto snapshots = this->build_ordered_snapshots_list();

        for (auto& snapshot: snapshots) {
            snapshot->build_block_refcounters(block_counters_);
        }
    }



    void init_store()
    {
        auto sb0 = get_superblock(0);
        auto sb1 = get_superblock(BASIC_BLOCK_SIZE);

        sb0->init(0, buffer_size(), SnapshotID{}, BASIC_BLOCK_SIZE, 0, 0, store_params_);
        sb0->build_superblock_description();

        sb1->init(BASIC_BLOCK_SIZE, buffer_size(), SnapshotID{}, BASIC_BLOCK_SIZE, 1, 1, store_params_);
        sb1->build_superblock_description();

        auto snapshot_descriptor_ptr = history_tree_.new_snapshot_descriptor("main");

        writer_mutex_.lock();

        auto ptr = do_create_writable_for_init(snapshot_descriptor_ptr);
        ptr->finish_store_initialization();
    }

private:
    CDescrPtr get_branch_head_read_sync(U8StringView name) const noexcept {
        LockGuard lock(history_mutex_);
        return history_tree_.get_branch_head(name);
    }

    std::vector<CDescrPtr> get_last_snapshot_for_rollback_sync() const noexcept
    {
        LockGuard lock(history_mutex_);
        return history_tree_.get_rollback_span();
    }

    template <typename T1, typename T2>
    constexpr static T2 DivUp(T1 v, T2 d) {
        return v / d + (v % d > 0);
    }
};

}
