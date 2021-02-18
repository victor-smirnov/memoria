
// Copyright 2020 Victor Smirnov
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

namespace memoria {

namespace detail {

struct FileLockHandler {
    virtual ~FileLockHandler() noexcept;
    virtual VoidResult unlock() noexcept = 0;

    static Result<std::unique_ptr<FileLockHandler>> lock_file(const char* name, bool create) noexcept;
};

}

template <typename Profile>
class MappedSWMRStore: public ISWMRStore<Profile>, public EnableSharedFromThis<MappedSWMRStore<Profile>> {

    using Base = ISWMRStore<Profile>;
    using typename Base::ReadOnlyCommitPtr;
    using typename Base::WritableCommitPtr;
    using typename Base::CommitID;

    using CommitDescriptorT = CommitDescriptor<Profile>;

    static constexpr size_t  BASIC_BLOCK_SIZE = 4096;
    static constexpr size_t  HEADER_SIZE = BASIC_BLOCK_SIZE * 2;
    static constexpr int32_t ALLOCATION_MAP_LEVELS    = ICtrApi<AllocationMap, Profile>::LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP = ICtrApi<AllocationMap, Profile>::ALLOCATION_SIZE * BASIC_BLOCK_SIZE;
    static constexpr size_t  MB = 1024*1024;

    mutable std::recursive_mutex reader_mutex_;
    mutable std::recursive_mutex writer_mutex_;

    using LockGuard     = std::lock_guard<std::recursive_mutex>;
    using Superblock    = SWMRSuperblock<Profile>;

    CommitDescriptorT* head_ptr_{};
    CommitDescriptorT* former_head_ptr_{};

    CommitDescriptorsList<Profile> eviction_queue_;
    std::unordered_set<CommitID> removing_persistent_commits_;

    template <typename> friend class MappedSWMRStoreReadonlyCommit;
    template <typename> friend class MappedSWMRStoreWritableCommit;
    template <typename> friend class MappedSWMRStoreCommitBase;

    friend Result<SharedPtr<ISWMRStore<MemoryCoWProfile<>>>> open_mapped_swmr_store(U8StringView);
    friend Result<SharedPtr<ISWMRStore<MemoryCoWProfile<>>>> create_mapped_swmr_store(U8StringView, uint64_t);


    U8String file_name_;
    uint64_t file_size_;
    std::unique_ptr<boost::interprocess::file_mapping> file_;
    boost::interprocess::mapped_region region_;

    std::unique_ptr<detail::FileLockHandler> lock_;

    Span<uint8_t> buffer_;

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
    }


    virtual Result<std::vector<CommitID>> persistent_commits() noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());

        using ResultT = Result<std::vector<CommitID>>;
        return ResultT::of();
    }

    virtual Result<ReadOnlyCommitPtr> open(CommitID commit_id) noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());
        using ResultT = Result<ReadOnlyCommitPtr>;
        return ResultT::of();
    }

    virtual Result<ReadOnlyCommitPtr> open() noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());

        using ResultT = Result<ReadOnlyCommitPtr>;
        MaybeError maybe_error{};
        ReadOnlyCommitPtr ptr{};

        {
            LockGuard lock(reader_mutex_);
            ptr = snp_make_shared<MappedSWMRStoreReadOnlyCommit<Profile>>(
                maybe_error, this->shared_from_this(), buffer_, head_ptr_
            );
        }

        if (!maybe_error) {
            return ResultT::of(std::move(ptr));
        }
        else {
            return std::move(maybe_error.get());
        }
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

    virtual VoidResult flush() noexcept {
        MEMORIA_TRY_VOID(check_if_open());
        MEMORIA_TRY_VOID(flush_data());
        return flush_header();
    }

    virtual Result<WritableCommitPtr> begin() noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());

        using ResultT = Result<SnpSharedPtr<MappedSWMRStoreWritableCommit<Profile>>>;
        writer_mutex_.lock();

        ResultT res = wrap_throwing([&]() -> ResultT {
            CommitDescriptorT* commit_descriptor = new CommitDescriptorT();
            MaybeError maybe_error{};
            auto ptr = snp_make_shared<MappedSWMRStoreWritableCommit<Profile>>(
                maybe_error, this->shared_from_this(), buffer_, head_ptr_, commit_descriptor
            );

            if (!maybe_error) {
                return ResultT::of(std::move(ptr));
            }
            else {
                return std::move(maybe_error.get());
            }
        });

        if (res.is_error())
        {
            writer_mutex_.unlock();
            return MEMORIA_PROPAGATE_ERROR(res);
        }
        else {
            MEMORIA_TRY_VOID(res.get()->finish_commit_opening());
            return Result<WritableCommitPtr>::of(std::move(res).get());
        }
    }

    virtual VoidResult close() noexcept {
        MEMORIA_TRY_VOID(check_if_open());
        MEMORIA_TRY_VOID(lock_->unlock());
        region_.flush();
        file_.reset();
        return VoidResult::of();
    }

    static void init_profile_metadata() noexcept {
        MappedSWMRStoreWritableCommit<Profile>::init_profile_metadata();
    }

    VoidResult flush_data() noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());
        if (!region_.flush(HEADER_SIZE, buffer_.size() - HEADER_SIZE, false)) {
            return make_generic_error("DataFlush operation failed");
        }

        return VoidResult::of();
    }

    VoidResult flush_header() noexcept
    {
        MEMORIA_TRY_VOID(check_if_open());

        if (!region_.flush(0, HEADER_SIZE, false)) {
            return make_generic_error("HeaderFlush operation failed");
        }
        return VoidResult::of();
    }

    VoidResult finish_commit(CommitDescriptorT* commit_descriptor) noexcept
    {
        return wrap_throwing([&](){
            {
                LockGuard lock(reader_mutex_);

                if (former_head_ptr_) {
                    eviction_queue_.push_back(*former_head_ptr_);
                }

                former_head_ptr_ = head_ptr_;
                head_ptr_ = commit_descriptor;
            }

            writer_mutex_.unlock();
        });
    }

    VoidResult finish_rollback(CommitDescriptorT* commit_descriptor) noexcept
    {
        return VoidResult::of();
    }

private:
    VoidResult check_if_open() noexcept {
        if (!file_) {
            return make_generic_error("File {} has been already closed", file_name_);
        }

        return VoidResult::of();
    }

    VoidResult init_store()
    {
        Superblock* sb0 = get_superblock(0);
        Superblock* sb1 = get_superblock(BASIC_BLOCK_SIZE);

        MEMORIA_TRY_VOID(sb0->init(0, buffer_.size(), 0, BASIC_BLOCK_SIZE, 0));
        MEMORIA_TRY_VOID(sb0->build_superblock_description());

        MEMORIA_TRY_VOID(sb1->init(BASIC_BLOCK_SIZE, buffer_.size(), 0, BASIC_BLOCK_SIZE, 0));
        MEMORIA_TRY_VOID(sb1->build_superblock_description());

        MaybeError maybe_error{};
        CommitDescriptorT* commit_descriptor = new CommitDescriptorT();

        writer_mutex_.lock();

        auto ptr = snp_make_shared<MappedSWMRStoreWritableCommit<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, commit_descriptor, InitStoreTag{}
        );

        if (!maybe_error)
        {
            MEMORIA_TRY_VOID(ptr->finish_store_initialization());
            head_ptr_ = commit_descriptor;
            return VoidResult::of();
        }
        else {
            return std::move(maybe_error.get());
        }
    }

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

    Superblock* get_superblock(uint64_t file_pos) noexcept {
        return ptr_cast<Superblock>(buffer_.data() + file_pos);
    }

    VoidResult do_open_file()
    {
        Superblock* sb0 = ptr_cast<Superblock>(buffer_.data());
        Superblock* sb1 = ptr_cast<Superblock>(buffer_.data() + BASIC_BLOCK_SIZE);

        if (!sb0->match_magick()) {
            return MEMORIA_MAKE_GENERIC_ERROR("First SWMR store header magick number mismatch");
        }

        if (!sb1->match_magick()) {
            return MEMORIA_MAKE_GENERIC_ERROR("Second SWMR store header magick number mismatch");
        }

        if (sb0->sequence_id() > sb1->sequence_id())
        {
            head_ptr_ = new CommitDescriptorT(get_superblock(sb0->superblock_file_pos()));

            if (sb1->sequence_id() > 0)
            {
                former_head_ptr_ = new CommitDescriptorT(get_superblock(sb1->superblock_file_pos()));
            }
        }
        else {
            head_ptr_ = new CommitDescriptorT(get_superblock(sb1->superblock_file_pos()));

            if (sb0->sequence_id() > 0)
            {
                former_head_ptr_ = new CommitDescriptorT(get_superblock(sb0->superblock_file_pos()));
            }
        }

        if (head_ptr_->superblock()->file_size() != buffer_.size()) {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "SWMR Store file size mismatch with header: {} {}",
                head_ptr_->superblock()->file_size(), buffer_.size()
            );
        }

        // Read snapshot history and
        // preload all transient snapshots into the
        // eviction queue

        MaybeError maybe_error{};
        auto ptr = snp_make_shared<MappedSWMRStoreReadOnlyCommit<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, head_ptr_
        );

        if (MMA_UNLIKELY((bool)maybe_error)) {
            return std::move(maybe_error.get());
        }

        uint64_t head_pos = head_ptr_->superblock()->superblock_file_pos();

        uint64_t former_head_pos{};
        if (former_head_ptr_) {
            former_head_pos = former_head_ptr_->superblock()->superblock_file_pos();
        }

        auto rr = ptr->for_each_root_block([&](int64_t root_block_addr) noexcept -> VoidResult {
            if (root_block_addr < 0)
            {
                uint64_t superblock_pos = -root_block_addr;
                if (superblock_pos != former_head_pos && superblock_pos != head_pos)
                {
                    auto res = wrap_throwing([&](){
                        Superblock* superblock = ptr_cast<Superblock>(buffer_.data() + superblock_pos);
                        CommitDescriptorT* commit_descr = new CommitDescriptorT(superblock);
                        eviction_queue_.push_back(*commit_descr);
                    });
                    MEMORIA_RETURN_IF_ERROR(res);
                }
            }
            return VoidResult::of();
        });
        MEMORIA_RETURN_IF_ERROR(rr);

        return VoidResult::of();
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

    void unlock_writer() noexcept {
        writer_mutex_.unlock();
    }
};

}
