
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

#include <memoria/store/swmr/common/swmr_store_readonly_commit_base.hpp>

#include <functional>

namespace memoria {

template <typename Profile>
class MappedSWMRStoreReadOnlyCommit:
        public SWMRStoreReadOnlyCommitBase<Profile>,
        public EnableSharedFromThis<MappedSWMRStoreReadOnlyCommit<Profile>>
{
protected:
    using Base = SWMRStoreReadOnlyCommitBase<Profile>;

    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;

    using typename Base::Store;
    using typename Base::CommitDescriptorT;
    using typename Base::CtrID;
    using typename Base::CtrReferenceableResult;
    using typename Base::AllocatorT;
    using typename Base::BlockID;
    using typename Base::BlockG;
    using typename Base::BlockType;

    using typename Base::DirectoryCtrType;
    using typename Base::HistoryCtrType;
    using typename Base::HistoryCtr;

    using typename Base::AllocationMapCtr;
    using typename Base::AllocationMapCtrType;

    using typename Base::CommitID;

    using typename Base::Shared;

    using Base::BASIC_BLOCK_SIZE;

    using Base::directory_ctr_;
    using Base::superblock_;
    using Base::commit_descriptor_;
    using Base::internal_find_by_root_typed;
    using Base::traverse_cow_containers;
    using Base::traverse_ctr_cow_tree;

    Span<uint8_t> buffer_;

    mutable boost::object_pool<Shared> shared_pool_;

    template <typename>
    friend class MappedSWMRStore;

public:
    using Base::find;
    using Base::getBlock;

    MappedSWMRStoreReadOnlyCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CommitDescriptorT* commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate = nullptr
    ) noexcept:
        Base(store, commit_descriptor, refcounter_delegate),
        buffer_(buffer)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            auto root_block_id = commit_descriptor->superblock()->directory_root_id();
            if (root_block_id.is_set())
            {
                auto directory_ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(root_block_id);
                MEMORIA_RETURN_IF_ERROR(directory_ctr_ref);
                directory_ctr_ = directory_ctr_ref.get();
                directory_ctr_->internal_reset_allocator_holder();
            }

            return VoidResult::of();
        });
    }


protected:
    uint64_t sequence_id() const {
        return commit_descriptor_->superblock()->sequence_id();
    }

    virtual SnpSharedPtr<AllocatorT> self_ptr() noexcept {
        return this->shared_from_this();
    }

    virtual Result<BlockG> getBlock(const BlockID& id) noexcept
    {
        using ResultT = Result<BlockG>;
        BlockType* block = ptr_cast<BlockType>(buffer_.data() + id.value() * BASIC_BLOCK_SIZE);

        Shared* shared = shared_pool_.construct(id, block, 0);
        shared->set_allocator(this);

        return ResultT::of(BlockG{shared});
    }

    virtual Result<BlockG> updateBlock(Shared* block) noexcept {
        return Result<BlockG>::of(BlockG{block});
    }

    virtual VoidResult releaseBlock(Shared* block) noexcept {
        shared_pool_.destroy(block);
        return VoidResult::of();
    }
};

}
