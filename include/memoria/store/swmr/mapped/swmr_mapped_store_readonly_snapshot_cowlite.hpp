
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

#include <memoria/store/swmr/common/swmr_store_readonly_snapshot_base.hpp>

#include <memoria/profiles/impl/cow_lite_profile.hpp>

#include <functional>

namespace memoria {

template <typename>
class MappedSWMRStoreReadOnlySnapshot;

template <typename ChildProfile>
class MappedSWMRStoreReadOnlySnapshot<CowLiteProfileT<ChildProfile>>:
        public SWMRStoreReadOnlySnapshotBase<CowLiteProfileT<ChildProfile>>,
        public EnableSharedFromThis<MappedSWMRStoreReadOnlySnapshot<CowLiteProfileT<ChildProfile>>>
{
protected:
    using Profile = CowLiteProfileT<ChildProfile>;

    using Base = SWMRStoreReadOnlySnapshotBase<Profile>;

    using ReadOnlySnapshotPtr = SharedPtr<ISWMRStoreReadOnlySnapshot<Profile>>;

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

    using typename Base::SnapshotID;

    using typename Base::Shared;

    using Base::BASIC_BLOCK_SIZE;

    using Base::store_;
    using Base::directory_ctr_;
    using Base::snapshot_descriptor_;
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

    MappedSWMRStoreReadOnlySnapshot(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CDescrPtr& snapshot_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate = nullptr
    ):
        Base(store, snapshot_descriptor, refcounter_delegate),
        buffer_(buffer)
    {
    }

    void post_init() {
        auto sb = this->get_superblock(snapshot_descriptor_->superblock_ptr());

        auto root_block_id = sb->directory_root_id();
        if (root_block_id.is_set())
        {
            auto directory_ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(root_block_id);

            directory_ctr_ = directory_ctr_ref;
            directory_ctr_->internal_detouch_from_store();
        }
    }


    uint64_t sequence_id() const {
        auto sb = get_superblock();
        return sb->sequence_id();
    }

    virtual SnpSharedPtr<StoreT> self_ptr()  {
        return this->shared_from_this();
    }

    using typename Base::ResolvedBlock;
    virtual ResolvedBlock resolve_block(const BlockID& block_id)
    {
        BlockType* block = ptr_cast<BlockType>(buffer_.data() + block_id.value().value() * BASIC_BLOCK_SIZE);

        Shared* shared = shared_pool_.construct(block_id, block, 0);

        shared->set_allocator(this);

        return {block_id.value().value(), SharedBlockConstPtr{shared}};
    }

    AllocationMetadataT resolve_block_allocation(const BlockID& block_id)
    {
        int32_t level = block_id.value().metadata();
        int64_t id_value = static_cast<int64_t>(block_id.value().value());

        return AllocationMetadataT(id_value, 1, level);
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
        return AllocationMetadataT((int64_t)block_id.value().value(), 1, (int32_t)block_id.value().metadata());
    }

    void check_storage_specific(
            SharedBlockConstPtr block,
            const CheckResultConsumerFn& consumer
    ) {
        int32_t block_size = block->memory_block_size();
        int32_t expected_block_size = (1 << block->id().value().metadata()) * BASIC_BLOCK_SIZE;

        if (block_size != expected_block_size) {
            consumer(CheckSeverity::ERROR, make_string_document("Block size mismatch for block {}. Expected {}, actual {}", expected_block_size, block_size));
        }
    }
};

}
