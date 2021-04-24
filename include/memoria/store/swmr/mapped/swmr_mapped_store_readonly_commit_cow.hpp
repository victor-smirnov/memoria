
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
#include <memoria/core/tools/simple_2q_cache.hpp>

#include <memoria/profiles/impl/cow_profile.hpp>

#include <functional>

namespace memoria {

template <typename>
class MappedSWMRStoreReadOnlyCommit;

template <typename ChildProfile>
class MappedSWMRStoreReadOnlyCommit<CowProfile<ChildProfile>>:
        public SWMRStoreReadOnlyCommitBase<CowProfile<ChildProfile>>,
        public EnableSharedFromThis<MappedSWMRStoreReadOnlyCommit<CowProfile<ChildProfile>>>
{
protected:
    using Profile = CowProfile<ChildProfile>;

    using Base = SWMRStoreReadOnlyCommitBase<Profile>;

    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;

    using typename Base::ApiProfileT;
    using typename Base::Store;
    using typename Base::CommitDescriptorT;
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

    using typename Base::CommitID;

    using typename Base::Shared;

    using BlockIDValueHolder = typename BlockID::ValueHolder;

    using BlockMapCtrType    = Map<BlockIDValueHolder, UBigInt>;
    using BlockMapCtr        = ICtrApi<BlockMapCtrType, ApiProfileT>;

    using Base::BASIC_BLOCK_SIZE;

    using Base::directory_ctr_;
    using Base::superblock_;
    using Base::commit_descriptor_;
    using Base::internal_find_by_root_typed;

    class CacheEntryBase: public Shared {
        uint64_t file_pos_;
    public:
        CacheEntryBase(const BlockID& id, BlockType* block, uint64_t file_pos) noexcept :
            Shared(id, block, 0), file_pos_(file_pos)
        {}

        uint64_t file_pos() const noexcept {return file_pos_;}
    };

    using SharedBlockCache = SimpleTwoQueueCache<BlockID, CacheEntryBase>;
    using BlockCacheEntry = typename SharedBlockCache::EntryT;

    CtrSharedPtr<BlockMapCtr> blockmap_ctr_;

    Span<uint8_t> buffer_;

    mutable boost::object_pool<BlockCacheEntry> cache_entry_pool_;
    mutable SharedBlockCache block_cache_;

    template <typename>
    friend class MappedSWMRStore;

public:
    using Base::find;
    using Base::getBlock;
    using Base::traverse_cow_containers;
    using Base::traverse_ctr_cow_tree;


    MappedSWMRStoreReadOnlyCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CommitDescriptorT* commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate = nullptr
    ) noexcept:
        Base(store, commit_descriptor, refcounter_delegate),
        buffer_(buffer),
        block_cache_(1024*128)
    {
        wrap_construction(maybe_error, [&] {
            auto blockmap_root_id = commit_descriptor->superblock()->blockmap_root_id();
            if (blockmap_root_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<BlockMapCtrType>(blockmap_root_id);

                blockmap_ctr_ = ctr_ref;
                blockmap_ctr_->internal_reset_allocator_holder();
            }

            auto directory_root_id = commit_descriptor->superblock()->directory_root_id();
            if (directory_root_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(directory_root_id);

                directory_ctr_ = ctr_ref;
                directory_ctr_->internal_reset_allocator_holder();
            }
        });
    }

    static void init_profile_metadata() noexcept {
        Base::init_profile_metadata();
        BlockMapCtr::template init_profile_metadata<Profile>();
    }


    uint64_t sequence_id() const {
        return commit_descriptor_->superblock()->sequence_id();
    }

    virtual SnpSharedPtr<StoreT> self_ptr() noexcept {
        return this->shared_from_this();
    }


    using typename Base::ResolvedBlock;
    virtual ResolvedBlock resolve_block(const BlockID& block_id)
    {
        auto existing_entry = block_cache_.get(block_id);
        if (existing_entry)
        {
            BlockCacheEntry* shared = existing_entry.get();
            return {shared->file_pos() * BASIC_BLOCK_SIZE, SharedBlockConstPtr{shared}};
        }
        else {
            uint64_t at;

            if (MMA_UNLIKELY(block_id.value().version() == 15))
            {
                at = unpack_uint64_t(block_id.value());
            }
            else {
                auto ii = blockmap_ctr_->find(block_id.value());
                if (ii->is_found(block_id.value())) {
                    at = ii->value();
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Can't find block ID {} in the BlockMap", block_id).do_throw();
                }
            }

            BlockType* block = ptr_cast<BlockType>(buffer_.data() + at * BASIC_BLOCK_SIZE);
            BlockCacheEntry* shared = cache_entry_pool_.construct(block_id, block, at);
            shared->set_allocator(this);

            block_cache_.insert(shared);

            return {at * BASIC_BLOCK_SIZE, SharedBlockConstPtr{shared}};
        }
    }

    virtual void updateBlock(Shared* block) {
    }

    virtual void releaseBlock(Shared* block) noexcept {
        block_cache_.attach(static_cast<BlockCacheEntry*>(block), [&](BlockCacheEntry* entry){
            cache_entry_pool_.destroy(entry);
        });
    }
};

}
