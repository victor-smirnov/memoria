
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
    using Base::writer_mutex_;
    using Base::head_ptr_;
    using Base::former_head_ptr_;

    using Base::HEADER_SIZE;
    using Base::BASIC_BLOCK_SIZE;
    using Base::ALLOCATION_MAP_SIZE_STEP;
    using Base::MB;

    using ApiProfileT = ApiProfile<Profile>;

    using LockGuard     = std::lock_guard<std::recursive_mutex>;
    using Superblock    = SWMRSuperblock<Profile>;

    template <typename> friend class MappedSWMRStoreReadonlyCommit;
    template <typename> friend class MappedSWMRStoreWritableCommit;
    template <typename> friend class MappedSWMRStoreCommitBase;

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
            acquire_lock(file_name.data(), true);

            make_file(file_name, file_size_);

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
            acquire_lock(file_name.data(), false);

            file_       = std::make_unique<boost::interprocess::file_mapping>(file_name_.data(), boost::interprocess::read_write);
            file_size_  = memoria::filesystem::file_size(file_name_);
            check_file_size();

            region_ = boost::interprocess::mapped_region(*file_.get(), boost::interprocess::read_write);
            buffer_ = Span<uint8_t>(ptr_cast<uint8_t>(region_.get_address()), file_size_);

            return VoidResult::of();
        });
    }

    ~MappedSWMRStore() noexcept {
        close();
    }

    virtual void flush() {
        check_if_open();
        flush_data();
        return flush_header();
    }


    void init_store() {
        return this->init_mapped_store();
    }


    virtual void close()
    {
        if (file_)
        {
            LockGuard lock(writer_mutex_);

            auto ctr_file_pos = head_ptr_->superblock()->block_counters_file_pos();
            CounterStorageT* ctr_storage = ptr_cast<CounterStorageT>(buffer_.data() + ctr_file_pos);

            size_t idx{};
            block_counters_.for_each([&](const BlockID& block_id, uint64_t counter) noexcept {
                ctr_storage[idx].block_id = block_id;
                ctr_storage[idx].counter  = counter;
                ++idx;
            });

            std::cout << "Written " << idx << " counters" << std::endl;

            flush_data();

            head_ptr_->superblock()->block_counters_size() = block_counters_.size();

            block_counters_.clear();

            flush_header();

            lock_->unlock();
            region_.flush();
            file_.reset();
        }
    }

    static void init_profile_metadata() noexcept {
        MappedSWMRStoreWritableCommit<Profile>::init_profile_metadata();
    }

    void flush_data(bool async = false)
    {
        check_if_open();
        if (!region_.flush(HEADER_SIZE, buffer_.size() - HEADER_SIZE, async)) {
            make_generic_error("DataFlush operation failed").do_throw();
        }
    }

    void flush_header(bool async = false)
    {
        check_if_open();
        if (!region_.flush(0, HEADER_SIZE, async)) {
            make_generic_error("HeaderFlush operation failed").do_throw();
        }
    }

    SharedPtr<ISWMRStoreHistoryView<ApiProfileT>> history_view()
    {
        auto head = do_open_readonly(head_ptr_);
        return MakeShared<SWMRMappedStoreHistoryView<Profile>>(this->shared_from_this(), head);
    }

    void do_open_file()
    {
        return this->do_open_buffer();
    }

private:

    void check_file_size()
    {
        if (file_size_ % BASIC_BLOCK_SIZE) {
            MEMORIA_MAKE_GENERIC_ERROR(
                "SWMR Store's file size is not multiple of {}: {}", BASIC_BLOCK_SIZE, file_size_
            ).do_throw();
        }

        if (file_size_ < BASIC_BLOCK_SIZE * 2) {
            MEMORIA_MAKE_GENERIC_ERROR(
                "SWMR Store's file size is too small {}", file_size_
            ).do_throw();
        }
    }

    void check_if_open() {
        if (!file_) {
            make_generic_error("File {} has been already closed", file_name_).do_throw();
        }
    }





    static uint64_t compute_file_size(uint64_t file_size_mb) noexcept
    {
        uint64_t allocation_size_mb = ALLOCATION_MAP_SIZE_STEP / MB;
        return (file_size_mb / allocation_size_mb) * allocation_size_mb * MB;
    }

    static void make_file(const U8String& name, uint64_t file_size)
    {
        std::filebuf fbuf;

        fbuf.open(name.to_std_string(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);

        //Set the size
        fbuf.pubseekoff(file_size - 1, std::ios_base::beg);
        fbuf.sputc(0);
        fbuf.close();
    }

    void acquire_lock(const char* file_name, bool create_file) {
        auto lock = detail::FileLockHandler::lock_file(file_name, create_file);

        if (!lock.is_error()) {
            if (!lock.get()) {
                MEMORIA_MAKE_GENERIC_ERROR("Provided file {} has been already locked", file_name).do_throw();
            }
            else {
                lock_ = std::move(lock.get());
            }
        }
        else {
            return std::move(lock).transfer_error().do_throw();
        }
    }

    virtual SWMRReadOnlyCommitPtr do_open_readonly(CommitDescriptorT* commit_descr)
    {
        MaybeError maybe_error{};
        MappedReadOnlyCommitPtr ptr{};

        {
            ptr = snp_make_shared<MappedSWMRStoreReadOnlyCommit<Profile>>(
                maybe_error, this->shared_from_this(), buffer_, commit_descr
            );
        }

        if (!maybe_error) {
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.get()).do_throw();
        }
    }

    virtual SWMRWritableCommitPtr do_create_writable(CommitDescriptorT* head, CommitDescriptorT* commit_descr)
    {
        MaybeError maybe_error{};
        auto ptr = snp_make_shared<MappedSWMRStoreWritableCommit<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, commit_descr
        );

        if (!maybe_error) {
            ptr->init_commit(head);
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.get()).do_throw();
        }
    }

    virtual SWMRWritableCommitPtr do_create_writable_for_init(CommitDescriptorT* commit_descr)
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
};

}
