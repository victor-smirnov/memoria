
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

#include <lmdb.h>

namespace memoria {

template <typename Profile>
class LMDBStoreReadOnlySnapshot:
        public LMDBStoreSnapshotBase<Profile>,
        public EnableSharedFromThis<LMDBStoreReadOnlySnapshot<Profile>>
{
protected:
    using Base = LMDBStoreSnapshotBase<Profile>;

    using ReadOnlySnapshotPtr = SharedPtr<ISWMRStoreReadOnlySnapshot<Profile>>;

    using typename Base::Store;
    using typename Base::CtrID;
    using typename Base::CtrReferenceableResult;
    using typename Base::StoreT;
    using typename Base::BlockID;
    using typename Base::Shared;
    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::Superblock;
    using typename Base::BlockType;
    using typename Base::ApiProfileT;

    using typename Base::DirectoryCtrType;
    using typename Base::SnapshotID;

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

    LMDBStoreReadOnlySnapshot(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            MDB_env* mdb_env,
            MDB_dbi system_db,
            MDB_dbi data_db
    ):
        Base(maybe_error, store, mdb_env),
        block_shared_cache_(16*1024)
    {
        system_db_ = system_db;
        data_db_ = data_db;

        wrap_construction(maybe_error, [&]() -> VoidResult {
            if (const int rc = mdb_txn_begin(mdb_env_, nullptr, MDB_RDONLY, &transaction_)) {
                return make_generic_error("Can't start read-only transaction, error = {}", mdb_strerror(rc));
            }

            auto superblock_ptr = get_data_addr(DirectoryCtrID, system_db_);

            // FIXME: handle errors here
            if (superblock_ptr.mv_data) {
                superblock_ = ptr_cast<Superblock>(superblock_ptr.mv_data);
            }
            else {
                return make_generic_error("Superblock is empty!");
            }

            return VoidResult::of();
        });
    }


    VoidResult post_init()
    {
        auto root_block_id = superblock_->directory_root_id();
        if (root_block_id.is_set())
        {
            auto directory_ctr_ref = wrap_throwing([&](){
                return this->template internal_find_by_root_typed<DirectoryCtrType>(root_block_id);
            });

            if (!directory_ctr_ref.is_error()) {
                directory_ctr_ = directory_ctr_ref.get();
                directory_ctr_->internal_detouch_from_store();
            }
            else {
                mdb_txn_abort(transaction_);
                return std::move(directory_ctr_ref).transfer_error();
            }
        }

        return VoidResult::of();
    }

    virtual ~LMDBStoreReadOnlySnapshot() noexcept {
        if (transaction_) {
            mdb_txn_abort(transaction_);
        }
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const hermes::Datatype& decl, const CtrID& ctr_id
    )
    {
        auto ptr = this->create_ctr_instance(decl, ctr_id);
        ptr->internal_detouch_from_store();
        return ptr;
    }

protected:
    uint64_t sequence_id() const {
        return mdb_txn_id(transaction_);
    }

    virtual SnpSharedPtr<StoreT> self_ptr() {
        return this->shared_from_this();
    }

    virtual void updateBlock(Shared* block) {
        MEMORIA_MAKE_GENERIC_ERROR("updateBlock() is not implemented for ReadOnly snapshots").do_throw();
    }

    virtual void resizeBlock(Shared* block, int32_t new_size) {
        MEMORIA_MAKE_GENERIC_ERROR("resizeBlock() is not implemented for ReadOnly snapshots").do_throw();
    }

    virtual void releaseBlock(Shared* block) noexcept
    {
        block_shared_cache_.attach(static_cast<BlockCacheEntry*>(block), [&](BlockCacheEntry* entry){
            block_shared_cache_pool_.destroy(entry);
        });
    }

    virtual SharedBlockConstPtr getBlock(const BlockID& id)
    {
        if (MMA_UNLIKELY(id.is_null())) {
            return SharedBlockConstPtr{};
        }

        auto block = block_shared_cache_.get(id);
        if (block) {
            return block.value();
        }
        else {
            auto block_ptr = get_data_addr(id, data_db_);
            if (block_ptr.mv_data) {
                BlockType* block = ptr_cast<BlockType>(block_ptr.mv_data);

                BlockCacheEntry* entry = block_shared_cache_pool_.construct(id, block);
                entry->set_store(this);

                block_shared_cache_.insert(entry);

                return entry;
            }
            else {
                make_generic_error("Block {} is not found in the data_db", id).do_throw();
            }
        }
    }

    virtual void removeBlock(const BlockID& id) {
        MEMORIA_MAKE_GENERIC_ERROR("removeBlock() is not implemented for ReadOnly snapshots").do_throw();
    }

    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID&) {
        MEMORIA_MAKE_GENERIC_ERROR("createBlock() is not implemented for ReadOnly snapshots").do_throw();
    }

    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID&) {
        MEMORIA_MAKE_GENERIC_ERROR("cloneBlock() is not implemented for ReadOnly snapshots").do_throw();
    }

    virtual void setRoot(const CtrID& ctr_id, const BlockID& root)
    {
        MEMORIA_MAKE_GENERIC_ERROR("setRoot() is not implemented for ReadOnly snapshots").do_throw();
    }


    virtual BlockID newId() {
        MEMORIA_MAKE_GENERIC_ERROR("newId() is not implemented for ReadOnly snapshots").do_throw();
    }

    virtual bool isActive() const {
        return false;
    }

    virtual void flush_open_containers() {
        MEMORIA_MAKE_GENERIC_ERROR("flush_open_containers() is not implemented for ReadOnly commits").do_throw();
    }

    virtual bool is_committed() const {
        return true;
    }

    virtual bool is_active() const {
        return false;
    }

    virtual bool is_marked_to_clear() const  {
        return false;
    }

    virtual void drop() {
        MEMORIA_MAKE_GENERIC_ERROR("drop() is not implemented for LMDB store").do_throw();
    }


    bool is_transient()  {
        return true;
    }

    bool is_system_snapshot()  {
        return false;
    }
};

}
