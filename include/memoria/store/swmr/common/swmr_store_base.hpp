
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
    virtual VoidResult unlock() noexcept = 0;

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

    static constexpr size_t  BASIC_BLOCK_SIZE = 4096;
    static constexpr size_t  HEADER_SIZE = BASIC_BLOCK_SIZE * 2;
    static constexpr int32_t ALLOCATION_MAP_LEVELS    = ICtrApi<AllocationMap, ApiProfileT>::LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP = ICtrApi<AllocationMap, ApiProfileT>::ALLOCATION_SIZE * BASIC_BLOCK_SIZE;
    static constexpr size_t  MB = 1024*1024;

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
            delete commit.second;
        }
    }


    virtual Result<std::vector<CommitID>> persistent_commits() noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());

        LockGuard lock(reader_mutex_);

        using ResultT = Result<std::vector<CommitID>>;

        std::vector<CommitID> commits;

        for (const auto& descr: persistent_commits_) {
            commits.push_back(descr.first);
        }

        return ResultT::of(std::move(commits));
    }

    virtual Result<ReadOnlyCommitPtr> open(CommitID commit_id) noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());
        LockGuard lock(reader_mutex_);

        using ResultT = Result<ReadOnlyCommitPtr>;

        if (MMA_UNLIKELY(head_ptr_->superblock()->commit_id() == commit_id)) {
            return open();
        }
        else if (MMA_UNLIKELY(former_head_ptr_->superblock()->commit_id() == commit_id)) {
            return open_readonly(former_head_ptr_);
        }

        auto ii = persistent_commits_.find(commit_id);
        if (ii != persistent_commits_.end())
        {
            return ResultT{
                memoria_static_pointer_cast<ISWMRStoreReadOnlyCommit<ApiProfileT>>(do_open_readonly(ii->second))
            };
        }
        else {
            return make_generic_error_with_source(MA_SRC, "Can't find commit {}", commit_id);
        }
    }

    virtual Result<ReadOnlyCommitPtr> open() noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());

        using ResultT = Result<ReadOnlyCommitPtr>;

        LockGuard lock(reader_mutex_);
        return ResultT{
            memoria_static_pointer_cast<ISWMRStoreReadOnlyCommit<ApiProfileT>>(do_open_readonly(head_ptr_))
        };
    }

    virtual BoolResult drop_persistent_commit(CommitID commit_id) noexcept {
        MEMORIA_TRY_VOID(check_if_open());
        return BoolResult::of();
    }

    virtual VoidResult rollback_last_commit() noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());
        return VoidResult::of();
    }

    virtual VoidResult flush() noexcept = 0;

    virtual Result<WritableCommitPtr> begin() noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());

        using ResultT = Result<SWMRWritableCommitPtr>;
        writer_mutex_.lock();

        ResultT res = wrap_throwing([&]() -> ResultT {
            CommitDescriptorT* commit_descriptor = new CommitDescriptorT();

            Result<SWMRWritableCommitPtr> ptr_res = do_create_writable(head_ptr_, commit_descriptor);

            if (ptr_res.is_ok())
            {
                return ResultT::of(std::move(ptr_res.get()));
            }
            else {
                delete commit_descriptor;
                return std::move(ptr_res).transfer_error();
            }
        });

        if (MMA_UNLIKELY(res.is_error()))
        {
            writer_mutex_.unlock();
            return MEMORIA_PROPAGATE_ERROR(res);
        }
        else {
            current_writer_ = res.get();
            MEMORIA_TRY_VOID(res.get()->finish_commit_opening());
            return Result<WritableCommitPtr>::of(std::move(res).get());
        }
    }

    virtual VoidResult close() noexcept = 0;

    virtual VoidResult flush_data(bool async = false) noexcept = 0;
    virtual VoidResult flush_header(bool async = false) noexcept = 0;

    VoidResult finish_commit(CommitDescriptorT* commit_descriptor) noexcept
    {
        return wrap_throwing([&](){
            {
                LockGuard lock(reader_mutex_);

                if (former_head_ptr_ && !former_head_ptr_->is_persistent()) {
                    eviction_queue_.push_back(*former_head_ptr_);
                }

                former_head_ptr_ = head_ptr_;
                head_ptr_ = commit_descriptor;

                if (commit_descriptor->is_persistent()) {
                    auto commit_id = commit_descriptor->superblock()->commit_id();
                    this->persistent_commits_[commit_id] = commit_descriptor;
                }
            }

            writer_mutex_.unlock();
        });
    }

    VoidResult finish_rollback(CommitDescriptorT* commit_descriptor) noexcept {
        return VoidResult::of();
    }

//    Result<SharedPtr<ISWMRStoreHistoryView<ApiProfileT>>> history_view() noexcept {
//        MEMORIA_TRY(head, open_mapped_readonly(head_ptr_));

//        return Result<SharedPtr<ISWMRStoreHistoryView<ApiProfileT>>>::of(
//            MakeShared<SWMRMappedStoreHistoryView<Profile>>(this->shared_from_this(), head)
//        );
//    }

    void unlock_writer() noexcept {
        writer_mutex_.unlock();
    }

protected:






    virtual VoidResult check_if_open() noexcept  = 0;


    virtual VoidResult check_file_size() noexcept = 0;



    Result<ReadOnlyCommitPtr> open_readonly(CommitDescriptorT* commit_descr) noexcept {
        using ResultT = Result<ReadOnlyCommitPtr>;
        return ResultT {
            memoria_static_pointer_cast<ISWMRStoreReadOnlyCommit<ApiProfileT>>(do_open_readonly(commit_descr))
        };
    }

    virtual Result<SWMRReadOnlyCommitPtr> do_open_readonly(CommitDescriptorT* commit_descr) noexcept = 0;
    virtual Result<SWMRWritableCommitPtr> do_create_writable(CommitDescriptorT* head, CommitDescriptorT* commit_descr) noexcept = 0;
    virtual Result<SWMRWritableCommitPtr> do_create_writable_for_init(CommitDescriptorT* commit_descr) noexcept = 0;

    virtual Result<Optional<SequenceID>> check(const Optional<SequenceID>& from, StoreCheckCallbackFn callback) noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());
        return wrap_throwing([&]() {
            return do_check(from, callback);
        });
    }

    Result<std::vector<SWMRReadOnlyCommitPtr>> build_ordered_commits_list(
            const Optional<SequenceID>& from = Optional<SequenceID>{}
    ) noexcept {
        using ResultT = Result<std::vector<SWMRReadOnlyCommitPtr>>;

        std::vector<SWMRReadOnlyCommitPtr> commits;

        for (CommitDescriptorT& commit: eviction_queue_) {
            MEMORIA_TRY(ptr, do_open_readonly(&commit));

            if ((!from) || (from && from.get() < ptr->sequence_id())) {
                commits.push_back(ptr);
            }
        }

        for (auto pair: persistent_commits_) {
            MEMORIA_TRY(ptr, do_open_readonly(pair.second));

            if ((!from) || (from && from.get() < ptr->sequence_id())) {
                commits.push_back(ptr);
            }
        }

        if (former_head_ptr_) {
            MEMORIA_TRY(ptr, do_open_readonly(former_head_ptr_));

            if ((!from) || (from && from.get() < ptr->sequence_id())) {
                commits.push_back(ptr);
            }
        }

        if (head_ptr_) {
            MEMORIA_TRY(ptr, do_open_readonly(head_ptr_));

            if ((!from) || (from && from.get() < ptr->sequence_id())) {
                commits.push_back(ptr);
            }
        }

        std::sort(commits.begin(), commits.end(), [&](const auto& one, const auto& two) -> bool {
            return one->sequence_id() < two->sequence_id();
        });

        return ResultT::of(commits);
    }

    Result<Optional<SequenceID>> do_check(const Optional<SequenceID>& from, StoreCheckCallbackFn callback)
    {
        using ResultT = Result<Optional<SequenceID>>;
        LockGuard lock(writer_mutex_);

        MEMORIA_TRY(commits, build_ordered_commits_list(from));

        if (commits.size() > 0)
        {
            CommitCheckState<Profile> check_state;

            for (auto& commit: commits) {
                MEMORIA_TRY_VOID(commit->check(callback));
                if (!from) {
                    MEMORIA_TRY_VOID(commit->build_block_refcounters(check_state.counters));
                }
            }

            SWMRWritableCommitPtr head = current_writer_.lock();

            if (head) {
                MEMORIA_TRY_VOID(head->check(callback));
                if (!from) {
                    MEMORIA_TRY_VOID(head->build_block_refcounters(check_state.counters));
                }
            }

            if (!from) {
                MEMORIA_TRY_VOID(check_refcounters(check_state.counters, callback));
            }

            return ResultT::of(commits[commits.size() - 1]->sequence_id());
        }
        else {
            return ResultT::of();
        }
    }

    VoidResult check_refcounters(const SWMRBlockCounters<Profile>& counters, StoreCheckCallbackFn callback) noexcept {
        if (counters.size() != block_counters_.size())
        {
            LDDocument doc;
            doc.set_varchar(fmt::format(
                        "Check failure: mismatched number of counters. Expected: {}, actual: {}",
                        block_counters_.size(),
                        counters.size()));

            MEMORIA_TRY_VOID(callback(doc));
        }

        auto res = counters.for_each([&](const BlockID& block_id, uint64_t counter) noexcept -> VoidResult {
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

                return VoidResult::of();
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

        return res;
    }

    virtual VoidResult ref_block(const BlockID& block_id) noexcept {
        block_counters_.inc(block_id);
        return VoidResult::of();
    }

    virtual VoidResult unref_block(const BlockID& block_id, const std::function<VoidResult()>& on_zero) noexcept {
        MEMORIA_TRY(zero, block_counters_.dec(block_id));
        if (zero) {
            return on_zero();
        }
        return VoidResult::of();
    }

    virtual VoidResult unref_ctr_root(const BlockID&) noexcept {
        return make_generic_error("SWMRStoreBase::unref_ctr_root() should not be called");
    }
};

}
