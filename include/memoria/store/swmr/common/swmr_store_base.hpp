
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

    using SWMRReadOnlyCommitPtr     = SnpSharedPtr<SWMRStoreReadOnlyCommitBase<Profile>>;
    using SWMRWritableCommitPtr     = SnpSharedPtr<SWMRStoreWritableCommitBase<Profile>>;
    using SWMRWritableCommitWeakPtr = SnpWeakPtr<SWMRStoreWritableCommitBase<Profile>>;

    using CommitDescriptorT = CommitDescriptor<Profile>;
    using BlockID = ProfileBlockID<Profile>;
    using CtrID = ProfileCtrID<Profile>;
    using ApiBlockID = ApiProfileBlockID<ApiProfileT>;

    using CounterStorageT = CounterStorage<Profile>;
    using RemovingBlockConsumerFn = std::function<void (const BlockID&, uint64_t, int32_t)>;

    static constexpr size_t  BASIC_BLOCK_SIZE = 4096;
    static constexpr size_t  HEADER_SIZE = BASIC_BLOCK_SIZE * 2;
    static constexpr int32_t ALLOCATION_MAP_LEVELS    = ICtrApi<AllocationMap, ApiProfileT>::LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP = ICtrApi<AllocationMap, ApiProfileT>::ALLOCATION_SIZE * BASIC_BLOCK_SIZE;
    static constexpr size_t  MB = 1024*1024;

    static_assert(sizeof(SWMRSuperblock<Profile>) <= 4096, "Check superblock size!");

    mutable std::recursive_mutex reader_mutex_;
    mutable std::recursive_mutex writer_mutex_;

    using LockGuard     = std::lock_guard<std::recursive_mutex>;
    using Superblock    = SWMRSuperblock<Profile>;

    HistoryTree<Profile> history_tree_;

    std::unordered_set<CommitID> removing_persistent_commits_;

    template <typename> friend class SWMRStoreReadonlyCommitBase;
    template <typename> friend class SWMRStoreWritableCommitBase;
    template <typename> friend class SWMRStoreCommitBase;

    SWMRWritableCommitWeakPtr current_writer_;
    SWMRBlockCounters<Profile> block_counters_;

    bool read_only_{false};

public:
    SWMRStoreBase() noexcept {
        history_tree_.set_superblock_fn([&](uint64_t file_pos){
            return this->get_superblock(file_pos * BASIC_BLOCK_SIZE);
        });
    }

    virtual void close() override = 0;

    virtual void flush_data(bool async = false) = 0;
    virtual void flush_header(bool async = false) = 0;

    virtual void flush() override = 0;

    virtual void check_if_open() = 0;

    virtual SWMRReadOnlyCommitPtr do_open_readonly(CommitDescriptorT* commit_descr) = 0;
    virtual SWMRWritableCommitPtr do_create_writable(CommitDescriptorT* head, CommitDescriptorT* commit_descr) = 0;

    virtual SWMRWritableCommitPtr do_open_writable(CommitDescriptorT* commit_descr, RemovingBlockConsumerFn fn) = 0;

    virtual SWMRWritableCommitPtr do_create_writable_for_init(CommitDescriptorT* commit_descr) = 0;
    virtual SharedPtr<SWMRStoreBase<Profile>> self_ptr() noexcept = 0;
    virtual void store_superblock(Superblock* superblock, uint64_t sb_slot) = 0;
    virtual Superblock* get_superblock(uint64_t file_pos) = 0;

    virtual Optional<std::vector<CommitID>> commits(U8StringView branch) override
    {
        using ResultT = Optional<std::vector<CommitID>>;

        check_if_open();
        LockGuard lock(reader_mutex_);

        auto head_opt = history_tree_.get_branch_head(branch);
        if (head_opt)
        {
            std::vector<CommitID> commits;

            CommitDescriptorT* node = head_opt.get();
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
            CommitDescriptorT* descr = descr_opt.get();
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

    virtual Optional<bool> is_persistent(const CommitID& commit_id) override
    {
        using ResultT = Optional<bool>;

        check_if_open();
        LockGuard lock(reader_mutex_);

        auto descr_opt = history_tree_.get(commit_id);
        if (descr_opt)
        {
            CommitDescriptorT* descr = descr_opt.get();
            return ResultT{descr->is_persistent()};
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
            if (commit.get()->is_persistent()) {
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

    virtual bool can_rollback_last_commit() noexcept override {
        LockGuard lock(reader_mutex_);
        return (!read_only_) && history_tree_.can_rollback_last_commit();
    }

    virtual void rollback_last_commit() override
    {
        throw_if_read_only();
        LockGuard wlock(writer_mutex_);
        LockGuard rlock(reader_mutex_);

        check_if_open();
        CommitDescriptorT* former_head_ptr = history_tree_.previous_last_commit();

        if (former_head_ptr)
        {
            CommitDescriptorT* head_ptr = history_tree_.last_commit();

            println("Rolling back to: {}", former_head_ptr->commit_id());

            // decrementing block counters
            auto ptr = do_open_writable(head_ptr, [&](const BlockID& block_id, uint64_t, uint64_t){
                // no need to do anything here
            });

            // Decrementing counters
            ptr->remove_all_blocks();

            head_ptr->superblock()->mark_for_rollback();
            head_ptr->superblock()->build_superblock_description();

            auto sb_slot = head_ptr->superblock()->sequence_id() % 2;
            store_superblock(head_ptr->superblock(), sb_slot);

            flush_header();

            history_tree_.do_rollback_last_commit();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Can't rollback the last commit").do_throw();
        }
    }

    virtual WritableCommitPtr begin() override {
        return begin("main");
    }


    virtual WritableCommitPtr begin(U8StringView branch) override {
        return branch_from("main", branch);
    }

    virtual WritableCommitPtr branch_from(U8StringView source, U8StringView branch_name) override
    {
        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();

        try {
            auto descr = get_branch_head_read_sync(source);
            if (descr)
            {
                std::unique_ptr<CommitDescriptorT> commit_descriptor = std::make_unique<CommitDescriptorT>(branch_name);

                SWMRWritableCommitPtr ptr = do_create_writable(descr.get(), commit_descriptor.get());
                ptr->finish_commit_opening();

                current_writer_ = ptr;
                commit_descriptor.release();
                return ptr;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Requested branch '{}' does not exist", branch_name).do_throw();
            }
        }
        catch (...) {
            writer_mutex_.unlock();
            throw;
        }
    }

    virtual WritableCommitPtr branch_from(const CommitID& commit_id, U8StringView branch_name) override
    {
        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();

        try {
            auto descr = [&]{
                LockGuard lock(reader_mutex_);
                return history_tree_.get(commit_id);
            }();

            if (descr)
            {
                if (descr.get()->is_persistent())
                {
                    std::unique_ptr<CommitDescriptorT> commit_descriptor = std::make_unique<CommitDescriptorT>(branch_name);

                    SWMRWritableCommitPtr ptr = do_create_writable(descr.get(), commit_descriptor.get());
                    ptr->finish_commit_opening();

                    current_writer_ = ptr;
                    commit_descriptor.release();
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
            writer_mutex_.unlock();
            throw;
        }
    }

    virtual Optional<CommitID> remove_branch(U8StringView branch_name) override
    {
        using Result = Optional<CommitID>;

        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();

        try {
            auto result_opt = [&]{
                using TupleT = std::tuple<CommitDescriptorT*, std::unique_ptr<CommitDescriptorT>, bool>;

                LockGuard lock(reader_mutex_);
                auto descr = history_tree_.get_branch_head(branch_name);
                if (descr)
                {
                    U8String last_branch = history_tree_.last_commit()->branch();
                    U8String new_branch = history_tree_.mark_branch_not_persistent(descr.get(), branch_name);
                    bool do_cleanup_data = history_tree_.branches_size() == 1;

                    std::unique_ptr<CommitDescriptorT> commit_descriptor = std::make_unique<CommitDescriptorT>(new_branch);

                    return Optional<TupleT>{TupleT{descr.get(), std::move(commit_descriptor), do_cleanup_data}};
                }
                else {
                    return Optional<TupleT>{};
                }
            }();

            if (result_opt)
            {
                auto& result = result_opt.get();

                CommitDescriptorT* descr = std::get<0>(result);
                std::unique_ptr<CommitDescriptorT> commit_descriptor = std::move(std::get<1>(result));
                bool do_cleanup_data = std::get<2>(result);

                SWMRWritableCommitPtr ptr = do_create_writable(descr, commit_descriptor.get());
                commit_descriptor.release();
                ptr->finish_commit_opening();

                // If we are removing the only branch, so we need to clean also the data,
                // because the 'main' branch should be recreated with empty state.
                if (do_cleanup_data) {
                    ptr->cleanup_data();
                }

                // This will remove all non-persistent commits from the history.
                ptr->commit();
                return ptr->commit_id();
            }
            else {
                return Result{};
            }
        }
        catch (...) {
            writer_mutex_.unlock();
            throw;
        }
    }


    virtual Optional<CommitID> remove_commit(const CommitID& commit_id) override
    {
        using Result = Optional<CommitID>;

        check_if_open();
        throw_if_read_only();

        writer_mutex_.lock();

        try {
            LockGuard lock(reader_mutex_);

            if (history_tree_.mark_commit_not_persistent(commit_id))
            {
                U8String last_branch = history_tree_.last_commit()->branch();

                std::unique_ptr<CommitDescriptorT> commit_descriptor = std::make_unique<CommitDescriptorT>(last_branch);

                SWMRWritableCommitPtr ptr = do_create_writable(history_tree_.last_commit(), commit_descriptor.get());
                ptr->finish_commit_opening();
                commit_descriptor.release();

                // This will remove all non-persistent commits from the history.
                ptr->commit();

                return ptr->commit_id();
            }
            else {
                return Result{};
            }
        }
        catch (...) {
            writer_mutex_.unlock();
            throw;
        }
    }


    virtual void finish_commit(CommitDescriptorT* commit_descriptor)
    {
        {
            LockGuard lock(reader_mutex_);
            cleanup_eviction_queue();
            history_tree_.attach_commit(commit_descriptor);
        }

        current_writer_.reset();
        writer_mutex_.unlock();
    }

    virtual void finish_rollback(CommitDescriptorT* commit_descriptor) {
        writer_mutex_.unlock();
    }

    SharedPtr<ISWMRStoreHistoryView<ApiProfileT>> history_view() override
    {
        check_if_open();

        LockGuard lock(reader_mutex_);
        auto head = do_open_readonly(history_tree_.last_commit());

        auto view = MakeShared<SWMRStoreHistoryViewImpl<Profile>>();

        head->for_each_history_entry_batch([&](auto commits, auto metas){
            view->load(commits, metas);
        });

        for (auto& name: history_tree_.branch_names())
        {
            Optional<CommitDescriptorT*> head = history_tree_.get_branch_head(name);
            view->load_branch(name, head.get()->commit_id());
        }

        view->build_tree();

        return view;
    }

    void unlock_writer() {
        writer_mutex_.unlock();
    }

    ReadOnlyCommitPtr open_readonly(CommitDescriptorT* commit_descr) {
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

        history_tree_.traverse_tree_preorder([&](CommitDescriptorT* descr){
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

            SWMRWritableCommitPtr head = current_writer_.lock();

            if (head) {
                head->check(callback);
                if (!from) {
                    head->build_block_refcounters(check_state.counters);
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

    void check_refcounters(const SWMRBlockCounters<Profile>& counters, StoreCheckCallbackFn callback) {
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

    virtual void unref_ctr_root(const BlockID&) override {
        return make_generic_error("SWMRStoreBase::unref_ctr_root() should not be called").do_throw();
    }



    void cleanup_eviction_queue() {
        history_tree_.cleanup_eviction_queue();
    }

    static bool is_my_block(const uint8_t* mem_block) noexcept {
        const Superblock* sb0 = ptr_cast<Superblock>(mem_block);
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

        history_tree_.traverse_tree_preorder([&](CommitDescriptorT* commit_descr){

            visitor.start_commit(commit_descr->commit_id(), commit_descr->sequence_id());

            auto commit = do_open_readonly(commit_descr);
            Superblock* sb = commit_descr->superblock();

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

private:
    Optional<CommitDescriptorT*> get_branch_head_read_sync(U8StringView name) const noexcept {
        LockGuard lock(reader_mutex_);
        return history_tree_.get_branch_head(name);
    }
};

}
