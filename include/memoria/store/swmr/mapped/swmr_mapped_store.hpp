
// Copyright 2019-2021 Victor Smirnov
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

#include <memoria/store/swmr/common/mapped_swmr_store_base.hpp>

#include <memoria/store/swmr/mapped/swmr_mapped_store_readonly_commit.hpp>
#include <memoria/store/swmr/mapped/swmr_mapped_store_writable_commit.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/filesystem/path.hpp>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <mutex>
#include <unordered_set>
#include <functional>

namespace memoria {

template <typename Profile>
class MappedSWMRStore: public MappedSWMRStoreBase<Profile>, public EnableSharedFromThis<MappedSWMRStore<Profile>> {

    using Base = MappedSWMRStoreBase<Profile>;

    using MappedReadOnlyCommitPtr = SnpSharedPtr<MappedSWMRStoreReadOnlyCommit<Profile>>;
    using MappedWritableCommitPtr = SnpSharedPtr<MappedSWMRStoreWritableCommit<Profile>>;

protected:

    using typename Base::ReadOnlyCommitPtr;
    using typename Base::WritableCommitPtr;

    using typename Base::SWMRReadOnlyCommitPtr;
    using typename Base::SWMRWritableCommitPtr;


    using typename Base::CommitID;
    using typename Base::SequenceID;
    using typename Base::CommitDescriptorT;
    using typename Base::CounterStorageT;
    using typename Base::BlockID;

    using Base::block_counters_;
    using Base::get_superblock;
    using Base::init_mapped_store;
    using Base::buffer_;

    using ApiProfileT = ApiProfile<Profile>;

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

    template <typename> friend class MappedSWMRStoreReadonlyCommit;
    template <typename> friend class MappedSWMRStoreWritableCommit;
    template <typename> friend class MappedSWMRStoreCommitBase;
    template <typename> friend class SWMRMappedStoreHistoryView;

    friend Result<SharedPtr<ISWMRStore<ApiProfileT>>> open_swmr_store(U8StringView);
    friend Result<SharedPtr<ISWMRStore<ApiProfileT>>> create_swmr_store(U8StringView, uint64_t);

    friend Result<SharedPtr<ISWMRStore<ApiProfileT>>> open_lite_raw_swmr_store(U8StringView);
    friend Result<SharedPtr<ISWMRStore<ApiProfileT>>> create_lite_raw_swmr_store(U8StringView, uint64_t);

    friend Result<SharedPtr<ISWMRStore<ApiProfileT>>> open_lite_swmr_store(U8StringView);
    friend Result<SharedPtr<ISWMRStore<ApiProfileT>>> create_lite_swmr_store(U8StringView, uint64_t);

    U8String file_name_;
    uint64_t file_size_;
    std::unique_ptr<boost::interprocess::file_mapping> file_;
    boost::interprocess::mapped_region region_;

    std::unique_ptr<detail::FileLockHandler> lock_;


public:
    MappedSWMRStore(MaybeError& maybe_error, U8String file_name, uint64_t file_size_mb):
        file_name_(file_name),
        file_size_(compute_file_size(file_size_mb))
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            if (filesystem::exists(file_name.to_std_string())) {                
                return MEMORIA_MAKE_GENERIC_ERROR("Provided file {} already exists", file_name);
            }
            MEMORIA_TRY_VOID(acquire_lock(file_name.data(), true));

            MEMORIA_TRY_VOID(make_file(file_name, file_size_));

            file_   = std::make_unique<boost::interprocess::file_mapping>(file_name_.data(), boost::interprocess::read_write);
            region_ = boost::interprocess::mapped_region(*file_.get(), boost::interprocess::read_write, 0, file_size_);
            buffer_ = Span<uint8_t>(ptr_cast<uint8_t>(region_.get_address()), file_size_);

            return VoidResult::of();
        });
    }

    MappedSWMRStore(MaybeError& maybe_error, U8String file_name):
        file_name_(file_name)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            MEMORIA_TRY_VOID(acquire_lock(file_name.data(), false));

            file_       = std::make_unique<boost::interprocess::file_mapping>(file_name_.data(), boost::interprocess::read_write);
            file_size_  = memoria::filesystem::file_size(file_name_);
            MEMORIA_TRY_VOID(check_file_size());

            region_ = boost::interprocess::mapped_region(*file_.get(), boost::interprocess::read_write);
            buffer_ = Span<uint8_t>(ptr_cast<uint8_t>(region_.get_address()), file_size_);

            return VoidResult::of();
        });
    }

    virtual ~MappedSWMRStore() noexcept {
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


    virtual VoidResult flush() noexcept {
        MEMORIA_TRY_VOID(check_if_open());
        MEMORIA_TRY_VOID(flush_data());
        return flush_header();
    }


    VoidResult init_store() noexcept {
        return this->init_mapped_store();
    }


    virtual VoidResult close() noexcept
    {
        if (file_)
        {
            LockGuard lock(writer_mutex_);

            auto ctr_file_pos = head_ptr_->superblock()->block_counters_file_pos();
            CounterStorageT* ctr_storage = ptr_cast<CounterStorageT>(buffer_.data() + ctr_file_pos);

            size_t idx{};
            auto res = block_counters_.for_each([&](const BlockID& block_id, uint64_t counter) noexcept {
                ctr_storage[idx].block_id = block_id;
                ctr_storage[idx].counter  = counter;
                ++idx;
                return VoidResult::of();
            });

            MEMORIA_RETURN_IF_ERROR(res);

            std::cout << "Written " << idx << " counters" << std::endl;

            MEMORIA_TRY_VOID(flush_data());

            head_ptr_->superblock()->block_counters_size() = block_counters_.size();

            block_counters_.clear();

            MEMORIA_TRY_VOID(flush_header());

            MEMORIA_TRY_VOID(lock_->unlock());
            region_.flush();
            file_.reset();
        }

        return VoidResult::of();
    }

    static void init_profile_metadata() noexcept {
        MappedSWMRStoreWritableCommit<Profile>::init_profile_metadata();
    }

    VoidResult flush_data(bool async = false) noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());
        if (!region_.flush(HEADER_SIZE, buffer_.size() - HEADER_SIZE, async)) {
            return make_generic_error("DataFlush operation failed");
        }

        return VoidResult::of();
    }

    VoidResult flush_header(bool async = false) noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());
        if (!region_.flush(0, HEADER_SIZE, async)) {
            return make_generic_error("HeaderFlush operation failed");
        }
        return VoidResult::of();
    }

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

    VoidResult finish_rollback(CommitDescriptorT* commit_descriptor) noexcept
    {
        return VoidResult::of();
    }

    Result<SharedPtr<ISWMRStoreHistoryView<ApiProfileT>>> history_view() noexcept {

        MEMORIA_TRY(head, do_open_readonly(head_ptr_));

        return Result<SharedPtr<ISWMRStoreHistoryView<ApiProfileT>>>::of(
            MakeShared<SWMRMappedStoreHistoryView<Profile>>(this->shared_from_this(), head)
        );
    }

private:

    VoidResult check_file_size() noexcept
    {
        if (file_size_ % BASIC_BLOCK_SIZE) {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "SWMR Store's file size is not multiple of {}: {}", BASIC_BLOCK_SIZE, file_size_
            );
        }

        if (file_size_ < BASIC_BLOCK_SIZE * 2) {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "SWMR Store's file size is too small {}", file_size_
            );
        }

        return VoidResult::of();
    }

    VoidResult check_if_open() noexcept {
        if (!file_) {
            return make_generic_error("File {} has been already closed", file_name_);
        }

        return VoidResult::of();
    }



    VoidResult do_open_file() noexcept
    {
        return this->do_open_buffer();
    }

    static uint64_t compute_file_size(uint64_t file_size_mb) noexcept
    {
        uint64_t allocation_size_mb = ALLOCATION_MAP_SIZE_STEP / MB;
        return (file_size_mb / allocation_size_mb) * allocation_size_mb * MB;
    }

    static VoidResult make_file(const U8String& name, uint64_t file_size) noexcept {
        return wrap_throwing([&](){
            std::filebuf fbuf;

            fbuf.open(name.to_std_string(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);

            //Set the size
            fbuf.pubseekoff(file_size - 1, std::ios_base::beg);
            fbuf.sputc(0);
            fbuf.close();
        });
    }

    VoidResult acquire_lock(const char* file_name, bool create_file) noexcept {
        auto lock = detail::FileLockHandler::lock_file(file_name, create_file);

        if (!lock.is_error()) {
            if (!lock.get()) {
                return MEMORIA_MAKE_GENERIC_ERROR("Provided file {} has been already locked", file_name);
            }
            else {
                lock_ = std::move(lock.get());
            }
        }
        else {
            return std::move(lock).transfer_error();
        }

        return VoidResult::of();
    }

    virtual Result<SWMRReadOnlyCommitPtr> do_open_readonly(CommitDescriptorT* commit_descr) noexcept
    {
        using ResultT = Result<SWMRReadOnlyCommitPtr>;
        MaybeError maybe_error{};
        MappedReadOnlyCommitPtr ptr{};

        {
            ptr = snp_make_shared<MappedSWMRStoreReadOnlyCommit<Profile>>(
                maybe_error, this->shared_from_this(), buffer_, commit_descr
            );
        }

        if (!maybe_error) {
            return ResultT::of(
                std::move(ptr)
            );
        }
        else {
            return std::move(maybe_error.get());
        }
    }

    virtual Result<SWMRWritableCommitPtr> do_create_writable(CommitDescriptorT* head, CommitDescriptorT* commit_descr) noexcept
    {
        using ResultT = Result<SWMRWritableCommitPtr>;

        MaybeError maybe_error{};
        auto ptr = snp_make_shared<MappedSWMRStoreWritableCommit<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, head, commit_descr
        );

        if (!maybe_error) {
            return ResultT::of(std::move(ptr));
        }
        else {
            return std::move(maybe_error.get());
        }
    }

    virtual Result<SWMRWritableCommitPtr> do_create_writable_for_init(CommitDescriptorT* commit_descr) noexcept
    {
        using ResultT = Result<SWMRWritableCommitPtr>;

        MaybeError maybe_error{};

        auto ptr = snp_make_shared<MappedSWMRStoreWritableCommit<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, commit_descr, InitStoreTag{}
        );

        if (!maybe_error)
        {
            return ResultT::of(std::move(ptr));
        }
        else {
            return std::move(maybe_error.get());
        }
    }
};

}
