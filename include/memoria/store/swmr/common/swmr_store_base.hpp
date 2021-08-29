
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
#include <memoria/store/swmr/common/swmr_store_readonly_commit_base.hpp>
#include <memoria/store/swmr/common/swmr_store_writable_commit_base.hpp>

#include <memoria/store/swmr/common/swmr_store_history_view.hpp>
#include <memoria/store/swmr/common/swmr_store_history_tree.hpp>

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

    using typename Base::ReadOnlyCommitPtr;
    using typename Base::WritableCommitPtr;
    using typename Base::CommitID;
    using typename Base::SequenceID;

    using ApiProfileT = ApiProfile<Profile>;

    using WritableCommitT = SWMRStoreWritableCommitBase<Profile>;

    using SWMRReadOnlyCommitPtr     = SnpSharedPtr<SWMRStoreReadOnlyCommitBase<Profile>>;
    using SWMRWritableCommitPtr     = SnpSharedPtr<WritableCommitT>;
    using SWMRWritableCommitWeakPtr = SnpWeakPtr<SWMRStoreWritableCommitBase<Profile>>;

    using CommitDescriptorT = CommitDescriptor<Profile>;
    using CDescrPtr = typename CommitDescriptorT::SharedPtrT;

    using BlockID = ProfileBlockID<Profile>;
    using CtrID = ProfileCtrID<Profile>;
    using ApiBlockID = ApiProfileBlockID<ApiProfileT>;
    using SuperblockT = SWMRSuperblock<Profile>;
    using CounterBlockT = SWMRCounterBlock<Profile>;

    using CounterStorageT = CounterStorage<Profile>;
    using RemovingBlockConsumerFn = std::function<void (const BlockID&, uint64_t, int32_t)>;

public:
    static constexpr size_t  BASIC_BLOCK_SIZE = 4096;

protected:
    static constexpr size_t  HEADER_SIZE = BASIC_BLOCK_SIZE * 2;
    static constexpr int32_t ALLOCATION_MAP_LEVELS    = ICtrApi<AllocationMap, ApiProfileT>::LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP = ICtrApi<AllocationMap, ApiProfileT>::ALLOCATION_SIZE * BASIC_BLOCK_SIZE;
    static constexpr size_t  MB = 1024*1024;

    static_assert(sizeof(SWMRSuperblock<Profile>) <= 4096, "Check superblock size!");

    mutable std::recursive_mutex reader_mutex_;
    mutable std::recursive_mutex writer_mutex_;

    using LockGuard     = std::lock_guard<std::recursive_mutex>;

    HistoryTree<Profile> history_tree_;

    std::unordered_set<CommitID> removing_persistent_commits_;

    template <typename> friend class SWMRStoreReadonlyCommitBase;
    template <typename> friend class SWMRStoreWritableCommitBase;
    template <typename> friend class SWMRStoreCommitBase;

    SWMRBlockCounters<Profile> block_counters_;

    bool read_only_{false};

    uint64_t cp_allocation_threshold_{100 * 1024 * 1024 / 4096};
    uint64_t cp_commits_threshold_{10000};
    uint64_t cp_timeout_{1000}; // 1 secons

    LDDocument store_params_;

    bool active_writer_{false};

public:
    using Base::flush;

    SWMRStoreBase() noexcept {
        history_tree_.set_superblock_fn([&](uint64_t file_pos){
            return this->get_superblock(file_pos * BASIC_BLOCK_SIZE);
        });
    }

    uint64_t cp_allocation_threshold() const override {
        LockGuard lock(reader_mutex_);
        return cp_allocation_threshold_;
    }

    uint64_t cp_commit_threshold() const override {
        LockGuard lock(reader_mutex_);
        return cp_commits_threshold_;
    }

    uint64_t set_cp_allocation_threshold(uint64_t value) override {
        LockGuard lock(reader_mutex_);
        auto tmp = cp_allocation_threshold_;
        cp_allocation_threshold_ = value;
        return tmp;
    }

    uint64_t set_cp_commit_threshold(uint64_t value) override {
        LockGuard lock(reader_mutex_);
        auto tmp = cp_commits_threshold_;
        cp_commits_threshold_ = value;
        return tmp;
    }

    int64_t cp_timeout() const override {
        LockGuard lock(reader_mutex_);
        return cp_timeout_;
    }

    int64_t set_cp_timeout(int64_t value) override
    {
        LockGuard lock(reader_mutex_);
        auto tmp = cp_timeout_;
        cp_timeout_ = value;
        return tmp;
    }

    virtual void close() override = 0;

    virtual void flush_data(bool async = false) = 0;
    virtual void flush_header(bool async = false) = 0;

    virtual void check_if_open() = 0;

    virtual SWMRReadOnlyCommitPtr do_open_readonly(CDescrPtr commit_descr) = 0;
    virtual SWMRWritableCommitPtr do_create_writable(
            CDescrPtr consistency_point,
            CDescrPtr head,
            CDescrPtr parent,
            CDescrPtr commit_descr
    ) = 0;

    virtual SWMRWritableCommitPtr do_open_writable(
            CDescrPtr commit_descr,
            RemovingBlockConsumerFn fn, bool force = false) = 0;

    virtual SWMRWritableCommitPtr do_create_writable_for_init(CDescrPtr commit_descr) = 0;
    virtual SharedPtr<SWMRStoreBase<Profile>> self_ptr() noexcept = 0;
    virtual void store_superblock(SuperblockT* superblock, uint64_t sb_slot) = 0;
    virtual SharedSBPtr<SuperblockT> get_superblock(uint64_t file_pos) = 0;
    virtual SharedSBPtr<CounterBlockT> get_counter_block(uint64_t file_pos) = 0;
    virtual uint64_t buffer_size() = 0;

    virtual void do_flush() = 0;

    virtual ReadOnlyCommitPtr flush(FlushType ft) override
    {
        LockGuard lock(writer_mutex_);
        check_if_open();

        bool clean;
        {
            LockGuard rlock(reader_mutex_);
            clean = history_tree_.is_clean();
        }

        if (!clean)
        {
            auto ii = create_system_commit();
            ii->commit(ConsistencyPoint::YES);
            return DynamicPointerCast<ISWMRStoreReadOnlyCommit<ApiProfileT>>(ii);
        }
        else {
            return ReadOnlyCommitPtr{};
        }
    }

    virtual Optional<std::vector<CommitID>> commits(U8StringView branch) override
    {
        using ResultT = Optional<std::vector<CommitID>>;

        check_if_open();
        LockGuard lock(reader_mutex_);

        auto head_opt = history_tree_.get_branch_head(branch);
        if (head_opt)
        {
            std::vector<CommitID> commits;

            CDescrPtr node = head_opt.get();
            while (node) {
                commits.push_back(node->commit_id());
                node = node->parent();
            }

            return ResultT{commits};
        }
        else {
            return ResultT{};
        }
    }

    virtual Optional<CommitID> parent(const CommitID& commit_id) override
    {
        using ResultT = Optional<CommitID>;

        check_if_open();
        LockGuard lock(reader_mutex_);

        auto descr_opt = history_tree_.get(commit_id);
        if (descr_opt)
        {
            CDescrPtr descr = descr_opt.get();
            if (descr->parent()) {
                return ResultT{descr->parent()->commit_id()};
            }
            else {
                return ResultT{};
            }
        }
        else {
            return ResultT{};
        }
    }

    virtual Optional<bool> is_transient(const CommitID& commit_id) override
    {
        using ResultT = Optional<bool>;

        check_if_open();
        LockGuard lock(reader_mutex_);

        auto descr_opt = history_tree_.get(commit_id);
        if (descr_opt)
        {
            CDescrPtr descr = descr_opt.get();
            return ResultT{descr->is_transient()};
        }
        else {
            return ResultT{};
        }
    }

    virtual Optional<bool> is_system_commit(const CommitID& commit_id) override
    {
        using ResultT = Optional<bool>;

        check_if_open();
        LockGuard lock(reader_mutex_);

        auto descr_opt = history_tree_.get(commit_id);
        if (descr_opt)
        {
            CDescrPtr descr = descr_opt.get();
            return ResultT{descr->is_system_commit()};
        }
        else {
            return ResultT{};
        }
    }


    virtual Optional<std::vector<CommitID>> children(const CommitID& commit_id) override
    {
        using ResultT = Optional<std::vector<CommitID>>;

        check_if_open();
        LockGuard lock(reader_mutex_);

        auto descr_opt = history_tree_.get(commit_id);
        if (descr_opt)
        {
            std::vector<CommitID> commits;

            for (auto chl: descr_opt.get()->children()) {
                commits.push_back(chl->commit_id());
            }

            return ResultT{commits};
        }
        else {
            return ResultT{};
        }
    }

    virtual ReadOnlyCommitPtr open(const CommitID& commit_id, bool open_transient_commits) override
    {
        check_if_open();
        if (!open_transient_commits) {
            return open_persistent(commit_id);
        }
        else
        {
            LockGuard wlock(writer_mutex_);

            auto commit = [&]{
                LockGuard rlock(reader_mutex_);
                return history_tree_.get(commit_id);
            }();

            if (commit) {
                return open_readonly(commit.get());
            }
            else {
                make_generic_error_with_source(MA_SRC, "Can't find commit {}", commit_id).do_throw();
            }
        }
    }



    ReadOnlyCommitPtr open_persistent(const CommitID& commit_id)
    {
        auto commit = [&]{
            LockGuard lock(reader_mutex_);
            return history_tree_.get(commit_id);
        }();

        if (commit)
        {
            if (!commit.get()->is_transient()) {
                return open_readonly(commit.get());
            }
            else {
                make_generic_error_with_source(MA_SRC, "Requested commit {} is not persistent", commit_id).do_throw();
            }
        }
        else {
            make_generic_error_with_source(MA_SRC, "Can't find commit {}", commit_id).do_throw();
        }
    }



    virtual ReadOnlyCommitPtr open() override {
        return open("main");
    }


    virtual ReadOnlyCommitPtr open(U8StringView branch) override
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
        LockGuard lock(reader_mutex_);
        return history_tree_.branch_names();
    }

    virtual int64_t count_volatile_commits() override {
        LockGuard lock(reader_mutex_);
        return history_tree_.count_volatile_commits();
    }

    virtual WritableCommitPtr begin() override {
        return begin("main");
    }


    virtual WritableCommitPtr begin(U8StringView branch) override {
        return branch_from("main", branch);
    }

    CDescrPtr create_commit_descriptor(U8StringView branch_name)
    {
        auto commit_descriptor = history_tree_.new_commit_descriptor(branch_name);

        commit_descriptor->set_start_time(getTimeInMillis());

        {
            LockGuard lock(reader_mutex_);
            commit_descriptor->set_allocated_basic_blocks_threshold(cp_allocation_threshold_);
            commit_descriptor->set_commits_threshold(cp_commits_threshold_);
            commit_descriptor->set_cp_timeout(cp_timeout_);
        }

        return commit_descriptor;
    }

    void check_no_writers()
    {
        if (MMA_UNLIKELY(active_writer_)) {
            writer_mutex_.unlock();
            MEMORIA_MAKE_GENERIC_ERROR("Another writer commit is still active. Commit/rollback it first.").do_throw();
        }
        active_writer_ = 1;
    }

    void unlock_writer() {
        active_writer_ = 0;
        writer_mutex_.unlock();
    }

    virtual WritableCommitPtr branch_from(U8StringView source, U8StringView branch_name) override
    {
        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();

        check_no_writers();

        try {
            auto descr = get_branch_head_read_sync(source);
            if (descr)
            {
                CDescrPtr commit_descriptor = create_commit_descriptor(branch_name);

                SWMRWritableCommitPtr ptr = do_create_writable(
                            history_tree_.consistency_point1(),
                            history_tree_.head(),
                            descr,
                            std::move(commit_descriptor)
                );



                ptr->finish_commit_opening();
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

    virtual WritableCommitPtr branch_from(const CommitID& commit_id, U8StringView branch_name) override
    {
        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();
        check_no_writers();

        try {
            auto descr = [&]{
                LockGuard lock(reader_mutex_);
                return history_tree_.get(commit_id);
            }();

            if (descr)
            {
                if (!descr.get()->is_transient())
                {
                    CDescrPtr commit_descriptor = create_commit_descriptor(branch_name);

                    SWMRWritableCommitPtr ptr = do_create_writable(
                                history_tree_.consistency_point1(),
                                history_tree_.head(),
                                descr,
                                std::move(commit_descriptor)
                    );
                    ptr->finish_commit_opening();
                    return ptr;
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Requested commit '{}' is not persistent", commit_id).do_throw();
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



    bool mark_branch_transient(
            const U8String& branch_name,
            std::function<void (const CommitID&)> set_transient_fn,
            std::function<void (const CommitID&)> set_removed_fn
    )
    {
        LockGuard rlock(reader_mutex_);
        auto head = history_tree_.get_branch_head(branch_name);

        if (head)
        {
            CommitDescriptorT* cd = head.get();

            while (cd && cd->children().size() == 1)
            {
                if (!cd->is_transient()) {
                    set_transient_fn(cd->commit_id());
                }

                cd = cd->parent();
            }

            set_removed_fn(head->commit_id());
            return true;
        }
        else {
            return false;
        }
    }


    // FIXME: needed only for flush
    WritableCommitPtr create_system_commit()
    {
        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();

        check_no_writers();

        try {
            LockGuard lock(reader_mutex_);

            U8String last_branch = history_tree_.head()->branch();
            CDescrPtr commit_descriptor = create_commit_descriptor(last_branch);
            commit_descriptor->set_system_commit(true);

            SWMRWritableCommitPtr ptr = do_create_writable(
                        history_tree_.consistency_point1(),
                        history_tree_.head(),
                        history_tree_.head(),
                        std::move(commit_descriptor)
            );

            ptr->finish_commit_opening();
            return ptr;
        }
        catch (...) {
            unlock_writer();
            throw;
        }
    }



    virtual void do_prepare(
            CDescrPtr commit_descriptor,
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
        CDescrPtr commit_descriptor,
        bool do_consistency_point,
        WritableCommitT* commit
    )
    {
        for (const auto& entry: commit->counters()) {
            block_counters_.apply(entry.first, entry.second.value);
        }

        {
            LockGuard lock(reader_mutex_);

            // We need to cleanup eviction queue before attaching the new commit
            // to the history tree. Because attaching may add a new entry to
            // the queue.
            cleanup_eviction_queue();

            history_tree_.attach_commit(commit_descriptor, do_consistency_point);

            for (const auto& commit_id: commit->removing_commits())
            {
                auto descr = history_tree_.get(commit_id);
                if (descr) {
                    descr->set_transient(true);
                }
            }

            for (const auto& branch_name: commit->removing_branches()) {
                history_tree_.remove_branch(branch_name);
            }
        }

        unlock_writer();
    }

    virtual void do_rollback(CDescrPtr commit_descriptor)
    {
        unlock_writer();
    }


    SharedPtr<ISWMRStoreHistoryView<ApiProfileT>> history_view() override
    {
        check_if_open();

        LockGuard lock(reader_mutex_);
        auto head = do_open_readonly(history_tree_.consistency_point1());

        auto view = MakeShared<SWMRStoreHistoryViewImpl<Profile>>();

        head->for_each_history_entry_batch([&](auto commits, auto metas){
            view->load(commits, metas);
        });

        for (auto& name: history_tree_.branch_names())
        {
            CDescrPtr head = history_tree_.get_branch_head(name);
            view->load_branch(name, head->commit_id());
        }

        view->build_tree();

        return view;
    }

    ReadOnlyCommitPtr open_readonly(CDescrPtr commit_descr) {
        return do_open_readonly(commit_descr);
    }


    virtual Optional<SequenceID> check(const Optional<SequenceID>& from, StoreCheckCallbackFn callback) override {
        check_if_open();
        return do_check(from, callback);
    }

    void for_all_evicting_commits(std::function<void (CommitDescriptorT*)> fn)
    {
        LockGuard lock(reader_mutex_);
        for (auto& descr: history_tree_.eviction_queue()) {
            fn(&descr);
        }
    }

protected:

    std::vector<SWMRReadOnlyCommitPtr> build_ordered_commits_list(
            const Optional<SequenceID>& from = Optional<SequenceID>{}
    ){
        std::vector<SWMRReadOnlyCommitPtr> commits;

        history_tree_.traverse_tree_preorder([&](CDescrPtr descr){
            auto ptr = do_open_readonly(descr);
            commits.push_back(ptr);
        });

        std::sort(commits.begin(), commits.end(), [&](const auto& one, const auto& two) -> bool {
            return one->sequence_id() < two->sequence_id();
        });

        return commits;
    }

    Optional<SequenceID> do_check(const Optional<SequenceID>& from, StoreCheckCallbackFn callback)
    {
        // FIXME:
        // Writer mutex is needed because we are touching counters here.
        // Need another lightweight method to check the data only without
        // taking writer lock

        LockGuard lock(writer_mutex_);

        auto commits = build_ordered_commits_list(from);

        if (commits.size() > 0)
        {
            CommitCheckState<Profile> check_state;

            for (auto& commit: commits) {
                commit->check(callback);
                if (!from) {
                    commit->build_block_refcounters(check_state.counters);
                }
            }

            if (!from) {
                check_refcounters(check_state.counters, callback);
            }

            return commits[commits.size() - 1]->sequence_id();
        }
        else {
            return Optional<SequenceID>{};
        }
    }

    void check_refcounters(const SWMRBlockCounters<Profile>& counters, StoreCheckCallbackFn callback)
    {
        block_counters_.diff(counters);

        if (counters.size() != block_counters_.size())
        {
            LDDocument doc;
            doc.set_varchar(fmt::format(
                        "Check failure: mismatched number of counters. Expected: {}, actual: {}",
                        block_counters_.size(),
                        counters.size()));

            return callback(doc);
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

                    return callback(doc);
                }
            }
            else {
                LDDocument doc;
                doc.set_varchar(fmt::format(
                            "Actual counter for the block ID {} is not found in the store's block_counters structure.",
                            block_id
                            ));

                return callback(doc);
            }
        });
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

    uint64_t count_refs(const ApiProfileBlockID<ApiProfile<Profile>>& block_id) override
    {
        LockGuard lock(writer_mutex_);

        BlockID id;
        block_id_holder_to(block_id, id);

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
        LockGuard lock(reader_mutex_);

        using VisitedBlocks = std::unordered_set<BlockID>;
        using CtrType = typename SWMRStoreGraphVisitor<ApiProfileT>::CtrType;

        VisitedBlocks visited_blocks;

        visitor.start_graph();

        std::unordered_set<BlockID> visited_nodes;

        history_tree_.traverse_tree_preorder([&](CDescrPtr commit_descr){
            visitor.start_commit(commit_descr->commit_id(), commit_descr->sequence_id());

            auto commit = do_open_readonly(commit_descr);
            auto sb = get_superblock(commit_descr->superblock_ptr());

            if (sb->blockmap_root_id().is_set()) {
                commit->traverse_ctr_cow_tree(sb->blockmap_root_id(), visited_blocks, visitor, CtrType::BLOCKMAP);
            }

            commit->traverse_ctr_cow_tree(sb->allocator_root_id(), visited_blocks, visitor, CtrType::ALLOCATOR);
            commit->traverse_ctr_cow_tree(sb->history_root_id(), visited_blocks, visitor, CtrType::HISTORY);
            commit->traverse_cow_containers(visited_blocks, visitor);

            visitor.end_commit();
        });

        visitor.end_graph();
    }

    virtual U8String to_string(const ApiBlockID& block_id) override {
        BlockID id;
        block_id_holder_to(block_id, id);
        return format_u8("{}", id);
    }

    uint64_t count_blocks(const BlockID& block_id)
    {
        auto ii = block_counters_.get(block_id);
        return boost::get_optional_value_or(ii, 0ull);
    }

    HistoryTree<Profile>& history_tree() noexcept {
        return history_tree_;
    }

    std::vector<typename HistoryTree<Profile>::UpdateOp> prepare_eviction()
    {
        using UpdateOp = typename HistoryTree<Profile>::UpdateOp;

        LockGuard lock(reader_mutex_);
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

        if (sb0->commit_id().is_null() && sb1->commit_id().is_null()) {
            // the file was only partially initialized, continue
            // the process with full initialization.
            return init_store();
        }

        CDescrPtr consistency_point1_ptr;
        CDescrPtr consistency_point2_ptr;

        if (sb0->consistency_point_sequence_id() > sb1->consistency_point_sequence_id())
        {
            consistency_point1_ptr = history_tree_.new_commit_descriptor(
                        sb0->superblock_file_pos(),
                        get_superblock(sb0->superblock_file_pos()).get(),
                        ""
            );

            store_params_ = LDDocument{sb0->cmetadata_doc()};

            if (sb1->commit_id().is_set())
            {
                consistency_point2_ptr = history_tree_.new_commit_descriptor(
                            sb1->superblock_file_pos(),
                            get_superblock(sb1->superblock_file_pos()).get(),
                            ""
                );
            }
        }
        else {
            consistency_point1_ptr = history_tree_.new_commit_descriptor(
                        sb1->superblock_file_pos(),
                        get_superblock(sb1->superblock_file_pos()).get(),
                        ""
            );

            store_params_ = LDDocument{sb1->cmetadata_doc()};

            if (sb0->commit_id().is_set())
            {
                consistency_point2_ptr = history_tree_.new_commit_descriptor(
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

        using MetaT = std::pair<CommitID, CommitMetadata<ApiProfile<Profile>>>;
        std::vector<MetaT> metas;

        ptr->for_each_history_entry([&](const auto& commit_id, const auto& commit_meta) {
            metas.push_back(MetaT(commit_id, commit_meta));
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
        auto commits = this->build_ordered_commits_list();

        for (auto& commit: commits) {
            commit->build_block_refcounters(block_counters_);
        }
    }



    void init_store()
    {
        auto sb0 = get_superblock(0);
        auto sb1 = get_superblock(BASIC_BLOCK_SIZE);

        sb0->init(0, buffer_size(), CommitID{}, BASIC_BLOCK_SIZE, 0, 0, store_params_);
        sb0->build_superblock_description();

        sb1->init(BASIC_BLOCK_SIZE, buffer_size(), CommitID{}, BASIC_BLOCK_SIZE, 1, 1, store_params_);
        sb1->build_superblock_description();

        auto commit_descriptor_ptr = history_tree_.new_commit_descriptor("main");

        writer_mutex_.lock();

        auto ptr = do_create_writable_for_init(commit_descriptor_ptr);
        ptr->finish_store_initialization();
    }

private:
    CDescrPtr get_branch_head_read_sync(U8StringView name) const noexcept {
        LockGuard lock(reader_mutex_);
        return history_tree_.get_branch_head(name);
    }

    std::vector<CDescrPtr> get_last_commit_for_rollback_sync() const noexcept
    {
        LockGuard lock(reader_mutex_);
        return history_tree_.get_rollback_span();
    }

    template <typename T1, typename T2>
    constexpr static T2 DivUp(T1 v, T2 d) {
        return v / d + (v % d > 0);
    }
};

}
