
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

    CommitDescriptorT* head_ptr_{};
    CommitDescriptorT* former_head_ptr_{};

    CommitDescriptorsList<Profile> eviction_queue_;
    std::unordered_set<CommitID> removing_persistent_commits_;

    std::unordered_map<CommitID, CommitDescriptorT*> persistent_commits_;

    template <typename> friend class SWMRStoreReadonlyCommitBase;
    template <typename> friend class SWMRStoreWritableCommitBase;
    template <typename> friend class SWMRStoreCommitBase;

    SWMRWritableCommitWeakPtr current_writer_;
    SWMRBlockCounters<Profile> block_counters_;

public:
    SWMRStoreBase() noexcept {}

    virtual ~SWMRStoreBase() noexcept {
        if (head_ptr_) {
            delete head_ptr_;
        }

        if (former_head_ptr_) {
            delete former_head_ptr_;
        }

        eviction_queue_.erase_and_dispose(
            eviction_queue_.begin(),
            eviction_queue_.end(),
            [](CommitDescriptorT* commit_descr) noexcept {
                delete commit_descr;
            }
        );

        for (auto commit: persistent_commits_) {
            if (commit.second != head_ptr_ && commit.second != former_head_ptr_) {
                delete commit.second;
            }
        }

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


    virtual std::vector<CommitID> persistent_commits() override
    {
        check_if_open();

        LockGuard lock(reader_mutex_);
        std::vector<CommitID> commits;

        for (const auto& descr: persistent_commits_) {
            commits.push_back(descr.first);
        }

        return std::move(commits);
    }

    virtual ReadOnlyCommitPtr open(const CommitID& commit_id) override
    {
        check_if_open();
        LockGuard lock(reader_mutex_);

        if (MMA_UNLIKELY(head_ptr_->superblock()->commit_id() == commit_id)) {
            return open();
        }
        else if (MMA_UNLIKELY(former_head_ptr_->superblock()->commit_id() == commit_id)) {
            return open_readonly(former_head_ptr_);
        }

        auto ii = persistent_commits_.find(commit_id);
        if (ii != persistent_commits_.end())
        {
            return do_open_readonly(ii->second);
        }
        else {
            make_generic_error_with_source(MA_SRC, "Can't find commit {}", commit_id).do_throw();
        }
    }

    virtual ReadOnlyCommitPtr open() override
    {
        check_if_open();

        LockGuard lock(reader_mutex_);
        return do_open_readonly(head_ptr_);
    }

    virtual bool drop_persistent_commit(const CommitID& commit_id) override {
        check_if_open();
        return false;
    }

    virtual bool can_rollback_last_commit() noexcept override {
        return former_head_ptr_ != nullptr;
    }

    virtual void rollback_last_commit() override
    {
        LockGuard lock(writer_mutex_);

        check_if_open();
        if (former_head_ptr_)
        {
            // decrementing block counters
            auto ptr = do_open_writable(head_ptr_, [&](const BlockID& block_id, uint64_t, uint64_t){});
            ptr->remove_all_blocks();

            head_ptr_->superblock()->mark_for_rollback();
            head_ptr_->superblock()->build_superblock_description();

            auto sb_slot = head_ptr_->superblock()->sequence_id() % 2;
            store_superblock(head_ptr_->superblock(), sb_slot);

            delete head_ptr_;
            head_ptr_ = former_head_ptr_;
            former_head_ptr_ = nullptr;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Can't rollback the last commit").do_throw();
        }
    }



    virtual WritableCommitPtr begin() override
    {
        check_if_open();
        writer_mutex_.lock();

        try {
            std::unique_ptr<CommitDescriptorT> commit_descriptor = std::make_unique<CommitDescriptorT>();

            SWMRWritableCommitPtr ptr = do_create_writable(head_ptr_, commit_descriptor.get());

            ptr->finish_commit_opening();

            current_writer_ = ptr;

            commit_descriptor.release();

            return ptr;
        }
        catch (...) {
            writer_mutex_.unlock();
            throw;
        }
    }



    virtual void finish_commit(CommitDescriptorT* commit_descriptor)
    {
        LockGuard lock(reader_mutex_);

        cleanup_eviction_queue();

        if (former_head_ptr_ && !former_head_ptr_->is_persistent()) {
            eviction_queue_.push_back(*former_head_ptr_);
        }

        former_head_ptr_ = head_ptr_;
        head_ptr_ = commit_descriptor;

        if (commit_descriptor->is_persistent()) {
            auto commit_id = commit_descriptor->superblock()->commit_id();
            this->persistent_commits_[commit_id] = commit_descriptor;
        }

        writer_mutex_.unlock();
    }

    virtual void finish_rollback(CommitDescriptorT* commit_descriptor) {
        writer_mutex_.unlock();
    }

    SharedPtr<ISWMRStoreHistoryView<ApiProfileT>> history_view() override {
        check_if_open();

        LockGuard lock(reader_mutex_);
        auto head = do_open_readonly(head_ptr_);
        return MakeShared<SWMRMappedStoreHistoryView<Profile>>(self_ptr(), std::move(head));
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

    std::vector<SWMRReadOnlyCommitPtr> build_ordered_commits_list(
            const Optional<SequenceID>& from = Optional<SequenceID>{}
    ){
        std::vector<SWMRReadOnlyCommitPtr> commits;

        for (CommitDescriptorT& commit: eviction_queue_) {
            auto ptr = do_open_readonly(&commit);

            if ((!from) || (from && from.get() < ptr->sequence_id())) {
                commits.push_back(ptr);
            }
        }

        for (auto pair: persistent_commits_) {
            auto ptr = do_open_readonly(pair.second);

            if ((!from) || (from && from.get() < ptr->sequence_id())) {
                commits.push_back(ptr);
            }
        }

        if (former_head_ptr_) {
            auto ptr = do_open_readonly(former_head_ptr_);

            if ((!from) || (from && from.get() < ptr->sequence_id())) {
                commits.push_back(ptr);
            }
        }

        if (head_ptr_) {
            auto ptr = do_open_readonly(head_ptr_);

            if ((!from) || (from && from.get() < ptr->sequence_id())) {
                commits.push_back(ptr);
            }
        }

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
                            "Expected counter for the block ID {} is not found in the store.",
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

    void for_all_evicting_commits(std::function<void (CommitDescriptorT*)> fn) {
        for (auto& descr: eviction_queue_) {
            fn(&descr);
        }
    }

    void cleanup_eviction_queue() {
        eviction_queue_.erase_and_dispose(
            eviction_queue_.begin(),
            eviction_queue_.end(),
            [](CommitDescriptorT* commit_descr) noexcept {
                delete commit_descr;
            }
        );
    }
};

}
