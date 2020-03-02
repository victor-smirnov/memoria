
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

#include <mutex>
#include <unordered_set>

namespace memoria {

template <typename Profile>
class MappedSWMRStore: public ISWMRStore<Profile>, public EnableSharedFromThis<MappedSWMRStore<Profile>> {

    using Base = ISWMRStore<Profile>;
    using typename Base::ReadOnlyCommitPtr;
    using typename Base::WritableCommitPtr;
    using typename Base::CommitID;

    using CommitDescriptorT = CommitDescriptor<Profile>;

    static constexpr size_t BASIC_BLOCK_SIZE = 4096;
    static constexpr size_t HEADER_SIZE = BASIC_BLOCK_SIZE * 2;
    static constexpr int32_t ALLOCATION_MAP_LEVELS    = ICtrApi<AllocationMap, Profile>::LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP = ICtrApi<AllocationMap, Profile>::ALLOCATION_SIZE;
    static constexpr size_t MB = 1024*1024;

    mutable std::recursive_mutex reader_mutex_;
    mutable std::recursive_mutex writer_mutex_;

    CommitDescriptorT* head_ptr_;
    CommitDescriptorsList<Profile> commit_list_;
    std::unordered_set<CommitID> removing_persistent_commits_;

    template <typename> friend class MappedSWMRStoreReadonlyCommit;
    template <typename> friend class MappedSWMRStoreWritableCommit;

    U8String file_name_;
    uint64_t file_size_;
    boost::interprocess::file_mapping file_;
    boost::interprocess::mapped_region region_;

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

            MEMORIA_TRY_VOID(make_file(file_name, file_size_));

            file_ =  boost::interprocess::file_mapping(file_name_.data(), boost::interprocess::read_write);
            region_ = boost::interprocess::mapped_region(file_, boost::interprocess::read_write, 0, file_size_);
            buffer_ = Span<uint8_t>(ptr_cast<uint8_t>(region_.get_address()), file_size_);

            return VoidResult::of();
        });

        //MappedSWMRStoreReadOnlyCommit<Profile> cm1(maybe_error, this->shared_from_this(), buffer_, nullptr);
        //MappedSWMRStoreWritableCommit<Profile> cm2(maybe_error, this->shared_from_this(), buffer_, nullptr, nullptr);

        //MappedSWMRStoreWritableCommit<Profile> cm3(maybe_error, this->shared_from_this(), buffer_, nullptr, InitStoreTag{});
    }

    MappedSWMRStore(MaybeError& maybe_error, U8String file_name):
        file_name_(file_name)
    {
//        wrap_construction(maybe_error, [&](){
//            file_ =  boost::interprocess::file_mapping(file_name_.data(), boost::interprocess::read_write);
//            region_ = boost::interprocess::mapped_region(file_, boost::interprocess::read_write);
//            buffer_ = Span<uint8_t>(ptr_cast<uint8_t>(region_.get_address()), file_size);
//        });

//        MappedSWMRStoreReadOnlyCommit<Profile> cm1(maybe_error, this->shared_from_this(), buffer_, nullptr);
//        MappedSWMRStoreWritableCommit<Profile> cm2(maybe_error, this->shared_from_this(), buffer_, nullptr, nullptr);

//        MappedSWMRStoreWritableCommit<Profile> cm3(maybe_error, this->shared_from_this(), buffer_, nullptr, InitStoreTag{});
    }


    virtual Result<std::vector<CommitID>> persistent_commits() noexcept
    {
        using ResultT = Result<std::vector<CommitID>>;
        return ResultT::of();
    }

    virtual Result<ReadOnlyCommitPtr> open(CommitID commit_id) noexcept
    {
        using ResultT = Result<ReadOnlyCommitPtr>;
        return ResultT::of();
    }

    virtual Result<ReadOnlyCommitPtr> open() noexcept
    {
        using ResultT = Result<ReadOnlyCommitPtr>;
        return ResultT::of();
    }

    virtual BoolResult drop_persistent_commit(CommitID commit_id) noexcept {
        return BoolResult::of();
    }

    virtual VoidResult rollback_last_commit() noexcept {
        return VoidResult::of();
    }

    virtual Result<WritableCommitPtr> begin() noexcept
    {
        using ResultT = Result<WritableCommitPtr>;
        return ResultT::of();
    }

    virtual VoidResult close() noexcept {
        return VoidResult::of();
    }

    static void init_profile_metadata() noexcept {

    }

    VoidResult flush_data() noexcept
    {
        if (!region_.flush(HEADER_SIZE, buffer_.size() - HEADER_SIZE, false)) {
            return make_generic_error("DataFlush operation failed");
        }

        return VoidResult::of();
    }

    VoidResult flush_header() noexcept
    {
        if (!region_.flush(0, HEADER_SIZE, false)) {
            return make_generic_error("HeaderFlush operation failed");
        }
        return VoidResult::of();
    }


private:

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
        });
    }

};

}
