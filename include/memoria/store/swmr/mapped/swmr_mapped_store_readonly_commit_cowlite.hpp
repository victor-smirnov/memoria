
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

#include <memoria/profiles/impl/cow_lite_profile.hpp>

#include <functional>

namespace memoria {

template <typename>
class MappedSWMRStoreReadOnlyCommit;

template <typename ChildProfile>
class MappedSWMRStoreReadOnlyCommit<CowLiteProfile<ChildProfile>>:
        public SWMRStoreReadOnlyCommitBase<CowLiteProfile<ChildProfile>>,
        public EnableSharedFromThis<MappedSWMRStoreReadOnlyCommit<CowLiteProfile<ChildProfile>>>
{
protected:
    using Profile = CowLiteProfile<ChildProfile>;

    using Base = SWMRStoreReadOnlyCommitBase<Profile>;

    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;

    using typename Base::Store;
    using typename Base::CDescrPtr;
    using typename Base::Superblock;
    using typename Base::CtrID;
    using typename Base::CtrReferenceableResult;
    using typename Base::StoreT;
    using typename Base::BlockID;
    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockType;

    using typename Base::DirectoryCtrType;
    using typename Base::HistoryCtrType;
    using typename Base::HistoryCtr;

    using typename Base::AllocationMapCtr;
    using typename Base::AllocationMapCtrType;
    using typename Base::AllocationMetadataT;

    using typename Base::CommitID;

    using typename Base::Shared;

    using Base::BASIC_BLOCK_SIZE;

    using Base::store_;
    using Base::directory_ctr_;
    using Base::commit_descriptor_;
    using Base::internal_find_by_root_typed;
    using Base::traverse_cow_containers;
    using Base::traverse_ctr_cow_tree;
    using Base::get_superblock;

    Span<uint8_t> buffer_;

    mutable boost::object_pool<Shared> shared_pool_;
    mutable boost::object_pool<detail::MMapSBPtrPooledSharedImpl> sb_shared_pool_;

    template <typename>
    friend class MappedSWMRStore;

public:
    using Base::find;
    using Base::getBlock;

    MappedSWMRStoreReadOnlyCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CDescrPtr& commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate = nullptr
    ) noexcept:
        Base(store, commit_descriptor, refcounter_delegate),
        buffer_(buffer)
    {
    }

    void post_init() {
        auto sb = this->get_superblock(commit_descriptor_->superblock_ptr());

        auto root_block_id = sb->directory_root_id();
        if (root_block_id.is_set())
        {
            auto directory_ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(root_block_id);

            directory_ctr_ = directory_ctr_ref;
            directory_ctr_->internal_reset_allocator_holder();
        }
    }


    uint64_t sequence_id() const {
        auto sb = get_superblock();
        return sb->sequence_id();
    }

    virtual SnpSharedPtr<StoreT> self_ptr() noexcept {
        return this->shared_from_this();
    }

    using typename Base::ResolvedBlock;
    virtual ResolvedBlock resolve_block(const BlockID& block_id)
    {
        BlockType* block = ptr_cast<BlockType>(buffer_.data() + block_id.value() * BASIC_BLOCK_SIZE);

        Shared* shared = shared_pool_.construct(block_id, block, 0);

        shared->set_allocator(this);

        return {block_id.value(), SharedBlockConstPtr{shared}};
    }


    virtual void updateBlock(Shared* block) {
    }

    virtual void releaseBlock(Shared* block) noexcept {
        shared_pool_.destroy(block);
    }

    virtual SharedSBPtr<Superblock> get_superblock(uint64_t pos) {
        Superblock* sb = ptr_cast<Superblock>(buffer_.data() + pos);
        return SharedSBPtr(sb, sb_shared_pool_.construct(&sb_shared_pool_));
    }

    virtual AllocationMetadataT get_allocation_metadata(const BlockID& block_id) {
        return AllocationMetadataT{(int64_t)block_id.value(), 1, 0};
    }
};

}
