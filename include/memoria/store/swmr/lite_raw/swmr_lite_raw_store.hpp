
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

#include <memoria/store/swmr/mapped/swmr_mapped_store_readonly_commit_cowlite.hpp>
#include <memoria/store/swmr/mapped/swmr_mapped_store_writable_commit_cowlite.hpp>

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
    using typename Base::SuperblockT;
    using typename Base::CounterStorageT;
    using typename Base::BlockID;
    using typename Base::LockGuard;
    using typename Base::ApiProfileT;
    using typename Base::RemovingBlockConsumerFn;

    using Base::history_tree_;
    using Base::block_counters_;
    using Base::get_superblock;
    using Base::buffer_;
    using Base::writer_mutex_;
    using Base::prepare_to_close;

    using Base::HEADER_SIZE;
    using Base::BASIC_BLOCK_SIZE;
    using Base::ALLOCATION_MAP_SIZE_STEP;
    using Base::MB;

    bool closed_{false};

public:
    using Base::init_store;
    using Base::do_open_store;

    SWMRLiteRawStore(Span<uint8_t> buffer) noexcept {
        buffer_ = buffer;
    }

    ~SWMRLiteRawStore() noexcept {
        close();
    }

    virtual ReadOnlyCommitPtr flush() override {
        return ReadOnlyCommitPtr{};
    }

    virtual void close() override {
        LockGuard lock(writer_mutex_);

        if (!closed_)
        {
            prepare_to_close();
            closed_ = true;
        }
    }

    static void init_profile_metadata() noexcept {
        MappedSWMRStoreWritableCommit<Profile>::init_profile_metadata();
    }

    void flush_data(bool async = false) override {
    }

    void flush_header(bool async = false) override {
    }

    virtual SWMRReadOnlyCommitPtr do_open_readonly(CommitDescriptorT* commit_descr) override
    {
        MaybeError maybe_error{};
        MappedReadOnlyCommitPtr ptr = snp_make_shared<MappedSWMRStoreReadOnlyCommit<Profile>>(
                maybe_error, this->shared_from_this(), buffer_, commit_descr
        );

        if (!maybe_error) {
            ptr->post_init();
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.get()).do_throw();
        }
    }

    virtual SWMRWritableCommitPtr do_create_writable(
            CommitDescriptorT* consistency_point,
            CommitDescriptorT* head,
            CommitDescriptorT* parent,
            CommitDescriptorT* commit_descr
    ) override
    {
        MaybeError maybe_error{};
        auto ptr = snp_make_shared<MappedSWMRStoreWritableCommit<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, commit_descr
        );

        if (!maybe_error) {
            ptr->init_commit(consistency_point, head, parent);
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.get()).do_throw();
        }
    }

    virtual SWMRWritableCommitPtr do_open_writable(CommitDescriptorT* commit_descr, RemovingBlockConsumerFn fn) override {
        MaybeError maybe_error{};
        MappedWritableCommitPtr ptr{};


        ptr = snp_make_shared<MappedSWMRStoreWritableCommit<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, commit_descr, fn
        );


        if (!maybe_error) {
            ptr->open_commit();
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.get()).do_throw();
        }
    }

    virtual SWMRWritableCommitPtr do_create_writable_for_init(CommitDescriptorT* commit_descr) override
    {
        MaybeError maybe_error{};

        auto ptr = snp_make_shared<MappedSWMRStoreWritableCommit<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, commit_descr
        );

        if (!maybe_error)
        {
            ptr->init_store_commit();
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.get()).do_throw();
        }
    }

    virtual void check_if_open() override {
    }

    virtual SharedPtr<SWMRStoreBase<Profile>> self_ptr() noexcept override {
        return this->shared_from_this();
    }

    static bool is_my_block(Span<uint8_t> block) noexcept {
        if (block.size() > BASIC_BLOCK_SIZE * 2) {
            return Base::is_my_block(block.data());
        }
        return false;
    }

    virtual U8String describe() const override {
        return format_u8("SWMRLiteRawStore<{}>", TypeNameFactory<Profile>::name());
    }
};

}
