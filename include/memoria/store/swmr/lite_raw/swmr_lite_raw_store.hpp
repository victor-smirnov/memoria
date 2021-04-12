
// Copyright 2021 Victor Smirnov
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
class SWMRLiteRawStore: public MappedSWMRStoreBase<Profile>, public EnableSharedFromThis<SWMRLiteRawStore<Profile>> {

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
    using typename Base::LockGuard;
    using typename Base::ApiProfileT;

    using Base::block_counters_;
    using Base::get_superblock;
    using Base::init_mapped_store;
    using Base::buffer_;
    using Base::writer_mutex_;
    using Base::head_ptr_;
    using Base::former_head_ptr_;

    using Base::HEADER_SIZE;
    using Base::BASIC_BLOCK_SIZE;
    using Base::ALLOCATION_MAP_SIZE_STEP;
    using Base::MB;

    bool closed_{false};

    template <typename> friend class MappedSWMRStoreReadonlyCommit;
    template <typename> friend class MappedSWMRStoreWritableCommit;
    template <typename> friend class MappedSWMRStoreCommitBase;

public:
    SWMRLiteRawStore(Span<uint8_t> buffer) noexcept {
        buffer_ = buffer;
    }

    ~SWMRLiteRawStore() noexcept {
        close().terminate_if_error();
    }

    virtual VoidResult flush() noexcept {
        return VoidResult::of();
    }


    VoidResult init_store() noexcept {
        return this->init_mapped_store();
    }

    virtual VoidResult close() noexcept {
        if (!closed_)
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

            closed_ = true;
        }

        return VoidResult::of();
    }

    static void init_profile_metadata() noexcept {
        MappedSWMRStoreWritableCommit<Profile>::init_profile_metadata();
    }

    VoidResult flush_data(bool async = false) noexcept {
        return VoidResult::of();
    }

    VoidResult flush_header(bool async = false) noexcept {
        return VoidResult::of();
    }

    Result<SharedPtr<ISWMRStoreHistoryView<ApiProfileT>>> history_view() noexcept
    {
        MEMORIA_TRY(head, do_open_readonly(head_ptr_));

        return Result<SharedPtr<ISWMRStoreHistoryView<ApiProfileT>>>::of(
            MakeShared<SWMRMappedStoreHistoryView<Profile>>(this->shared_from_this(), head)
        );
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
            maybe_error, this->shared_from_this(), buffer_, commit_descr
        );

        if (!maybe_error) {
            MEMORIA_TRY_VOID(ptr->init_commit(head));
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
            maybe_error, this->shared_from_this(), buffer_, commit_descr
        );

        if (!maybe_error)
        {
            MEMORIA_TRY_VOID(ptr->init_store_commit());
            return ResultT::of(std::move(ptr));
        }
        else {
            return std::move(maybe_error.get());
        }
    }

    virtual VoidResult check_if_open() noexcept {
        return VoidResult::of();
    }
};

}