
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

#include <memoria/store/lmdb/lmdb_store_common.hpp>
#include <memoria/core/tools/simple_2q_cache.hpp>

#include <functional>

#include <mdb/mma_lmdb.h>

namespace memoria {

template <typename Profile>
class LMDBStoreReadOnlyCommit:
        public LMDBStoreCommitBase<Profile>,
        public EnableSharedFromThis<LMDBStoreReadOnlyCommit<Profile>>
{
protected:
    using Base = LMDBStoreCommitBase<Profile>;

    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;

    using typename Base::Store;
    using typename Base::CtrID;
    using typename Base::CtrReferenceableResult;
    using typename Base::AllocatorT;
    using typename Base::BlockID;
    using typename Base::Shared;
    using typename Base::BlockG;
    using typename Base::Superblock;
    using typename Base::BlockType;

    using typename Base::DirectoryCtrType;
    using typename Base::CommitID;

    using Base::directory_ctr_;
    using Base::superblock_;
    using Base::internal_find_by_root_typed;
    using Base::mdb_env_;
    using Base::transaction_;
    using Base::system_db_;
    using Base::data_db_;
    using Base::get_data_addr;

    using BlockGuardCache = SimpleTwoQueueCache<BlockID, Shared>;
    using BlockCacheEntry = typename BlockGuardCache::EntryT;

    template <typename>
    friend class LMDBStore;

    mutable boost::object_pool<BlockCacheEntry> block_shared_cache_pool_;
    mutable BlockGuardCache block_shared_cache_;

public:
    using Base::find;
    using Base::getBlock;

    LMDBStoreReadOnlyCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            MDB_env* mdb_env,
            MDB_dbi system_db,
            MDB_dbi data_db
    ) noexcept:
        Base(maybe_error, store, mdb_env),
        block_shared_cache_(16*1024)
    {
        system_db_ = system_db;
        data_db_ = data_db;

        wrap_construction(maybe_error, [&]() -> VoidResult {
            if (const int rc = mma_mdb_txn_begin(mdb_env_, nullptr, MDB_RDONLY, &transaction_)) {
                return make_generic_error("Can't start read-only transaction, error = {}", mma_mdb_strerror(rc));
            }

            MEMORIA_TRY(superblock_ptr, get_data_addr(DirectoryCtrID, system_db_));

            // FIXME: handle errors here
            if (superblock_ptr.mv_data) {
                superblock_ = ptr_cast<Superblock>(superblock_ptr.mv_data);
            }
            else {
                return make_generic_error("Superblock is empty!");
            }

            auto root_block_id = superblock_->directory_root_id();
            if (root_block_id.is_set())
            {
                auto directory_ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(root_block_id);
                if (!directory_ctr_ref.is_error()) {
                    directory_ctr_ = directory_ctr_ref.get();
                    directory_ctr_->internal_reset_allocator_holder();
                }
                else {
                    mma_mdb_txn_abort(transaction_);
                    return std::move(directory_ctr_ref).transfer_error();
                }
            }

            return VoidResult::of();
        });
    }

    virtual ~LMDBStoreReadOnlyCommit() noexcept {
        if (transaction_) {
            mma_mdb_txn_abort(transaction_);
        }
    }

protected:
    uint64_t sequence_id() const {
        return mma_mdb_txn_id(transaction_);
    }

    virtual SnpSharedPtr<AllocatorT> self_ptr() noexcept {
        return this->shared_from_this();
    }

    virtual Result<BlockG> updateBlock(Shared* block) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("updateBlock() is not implemented for ReadOnly commits");
    }

    virtual VoidResult resizeBlock(Shared* block, int32_t new_size) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("resizeBlock() is not implemented for ReadOnly commits");
    }

    virtual VoidResult releaseBlock(Shared* block) noexcept
    {
        block_shared_cache_.attach(static_cast<BlockCacheEntry*>(block), [&](BlockCacheEntry* entry){
            block_shared_cache_pool_.destroy(entry);
        });

        return VoidResult::of();
    }

    virtual Result<BlockG> getBlock(const BlockID& id) noexcept
    {
        using ResultT = Result<BlockG>;

        auto block = block_shared_cache_.get(id);
        if (block) {
            return ResultT::of(block.get());
        }
        else {
            MEMORIA_TRY(block_ptr, get_data_addr(id, data_db_));
            if (block_ptr.mv_data) {
                BlockType* block = ptr_cast<BlockType>(block_ptr.mv_data);

                BlockCacheEntry* entry = block_shared_cache_pool_.construct(id, block, 0);
                entry->set_allocator(this);

                block_shared_cache_.insert(entry);

                return ResultT::of(BlockG(entry));
            }
            else {
                return make_generic_error("Block {} is not found in the data_db", id);
            }
        }
    }

    virtual VoidResult removeBlock(const BlockID& id) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("removeBlock() is not implemented for ReadOnly commits");
    }

    virtual Result<BlockG> createBlock(int32_t initial_size) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("createBlock() is not implemented for ReadOnly commits");
    }

    virtual Result<BlockG> cloneBlock(const BlockG& block) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("cloneBlock() is not implemented for ReadOnly commits");
    }

    virtual VoidResult setRoot(const CtrID& ctr_id, const BlockID& root) noexcept
    {
        return MEMORIA_MAKE_GENERIC_ERROR("setRoot() is not implemented for ReadOnly commits");
    }


    virtual Result<BlockID> newId() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("newId() is not implemented for ReadOnly commits");
    }

    virtual bool isActive() const noexcept {
        return false;
    }

    virtual VoidResult flush_open_containers() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("flush_open_containers() is not implemented for ReadOnly commits");
    }

    virtual bool is_committed() const noexcept {
        return true;
    }

    virtual bool is_active() const noexcept {
        return false;
    }

    virtual bool is_marked_to_clear() const noexcept {
        return false;
    }

    virtual VoidResult drop() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("drop() is not implemented for LMDB store");
    }
};

}
