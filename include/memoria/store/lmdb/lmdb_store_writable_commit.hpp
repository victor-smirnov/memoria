
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
#include <memoria/store/lmdb/lmdb_store_readonly_commit.hpp>

#include <memoria/core/datatypes/type_registry.hpp>

#include <memoria/core/tools/2q_cache.hpp>

#include <type_traits>

template <typename Profile>
class LMDBStoreReadOnlyCommit;

namespace memoria {

struct InitLMDBStoreTag{};

template <typename Profile>
class LMDBStoreWritableCommit:
        public LMDBStoreCommitBase<Profile>,
        public ISWMRStoreWritableCommit<ApiProfile<Profile>>,
        public EnableSharedFromThis<LMDBStoreWritableCommit<Profile>>
{
    using Base = LMDBStoreCommitBase<Profile>;

    using typename Base::Store;
    using typename Base::CtrID;
    using typename Base::CtrReferenceableResult;
    using typename Base::AllocatorT;
    using typename Base::CommitID;
    using typename Base::BlockID;
    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockType;
    using typename Base::ApiProfileT;
    using typename Base::Shared;
    using typename Base::Superblock;

    using typename Base::DirectoryCtrType;

    enum {ENTRY_STATE_ACTIVE, ENTRY_STATE_DELETED};

    static constexpr int32_t LMDB_HEADER_SIZE = 64;

    using UpdatedEntriesMemberHook = boost::intrusive::list_member_hook<
        boost::intrusive::link_mode<
            boost::intrusive::safe_link
        >
    >;

    struct CacheEntryBase: Shared {
        UpdatedEntriesMemberHook upd_hook_;

        CacheEntryBase(const BlockID& id, BlockType* block, int32_t state) noexcept :
            Shared(id, block, state),
            upd_hook_()
        {}

        bool is_updated() const noexcept {
            return upd_hook_.is_linked();
        }
    };

    using BlockCache = TwoQueueCache<BlockID, CacheEntryBase>;
    using BlockCacheEntry = typename BlockCache::EntryT;

    using UpdatedEntriesList = boost::intrusive::list<
        CacheEntryBase,
        boost::intrusive::member_hook<
            CacheEntryBase,
            UpdatedEntriesMemberHook,
            &CacheEntryBase::upd_hook_
        >
    >;


    static constexpr int32_t BASIC_BLOCK_SIZE            = Store::BASIC_BLOCK_SIZE;
    static constexpr int32_t SUPERBLOCK_SIZE             = BASIC_BLOCK_SIZE;

    using Base::directory_ctr_;

    using Base::createCtrName;
    using Base::superblock_;
    using Base::getRootID;
    using Base::getBlock;
    using Base::instance_map_;
    using Base::mdb_env_;
    using Base::transaction_;
    using Base::system_db_;
    using Base::data_db_;
    using Base::store_;
    using Base::get_data_addr;



    boost::object_pool<BlockCacheEntry> block_cache_entry_pool_;
    BlockCache block_cache_;
    UpdatedEntriesList updated_entries_;

    bool committed_{false};
    bool allocator_initialization_mode_{false};

    template <typename>
    friend class LMDBStore;

public:
    using Base::check;

    LMDBStoreWritableCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            MDB_env* mdb_env,
            Superblock* superblock,
            MDB_dbi system_db,
            MDB_dbi data_db
    ) noexcept :
        Base(maybe_error, store, mdb_env),
        block_cache_(1024*1024, [this](bool keep_entry, BlockCacheEntry* entry){
            return this->evictionFn(keep_entry, entry);
        })
    {
        superblock_ = superblock;
        system_db_ = system_db;
        data_db_ = data_db;

        wrap_construction(maybe_error, [&]() -> VoidResult {
            if (const int rc = mma_mdb_txn_begin(mdb_env_, nullptr, 0, &transaction_)) {
                return make_generic_error("Can't start read-write transaction, error = {}", mma_mdb_strerror(rc));
            }

            return VoidResult::of();
        });

        if (!maybe_error) {
            internal_init_system_ctr<DirectoryCtrType>(
                maybe_error,
                directory_ctr_,
                superblock_->directory_root_id(),
                DirectoryCtrID
            );

            if (maybe_error) {                
                mma_mdb_txn_abort(transaction_);
            }
        }
    }

    LMDBStoreWritableCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            MDB_env* mdb_env,
            Superblock* superblock,
            MDB_dbi system_db,
            MDB_dbi data_db,
            InitLMDBStoreTag
    ) noexcept:
        Base(maybe_error, store, mdb_env),
        block_cache_(1024*1024, [this](bool keep_entry, BlockCacheEntry* entry){
            return this->evictionFn(keep_entry, entry);
        })
    {
        superblock_ = superblock;
        system_db_ = system_db;
        data_db_ = data_db;

        wrap_construction(maybe_error, [&]() -> VoidResult {
            if (const int rc = mma_mdb_txn_begin(mdb_env_, nullptr, 0, &transaction_)) {
                return make_generic_error("Can't start read-write transaction, error = {}", mma_mdb_strerror(rc));
            }

            return VoidResult::of();
        });

        if (!maybe_error) {
            internal_init_system_ctr<DirectoryCtrType>(
                maybe_error,
                directory_ctr_,
                superblock_->directory_root_id(),
                DirectoryCtrID
            );

            if (maybe_error) {
                mma_mdb_txn_abort(transaction_);
            }
        }
    }

    virtual ~LMDBStoreWritableCommit() noexcept {
        if (transaction_ && !committed_) {
            mma_mdb_txn_abort(transaction_);
        }

        block_cache_.for_each_entry([&](BlockCacheEntry* entry){
           ::free(entry->get());
        });
    }

    VoidResult finish_store_initialization() noexcept {
        return commit(true);
    }

    VoidResult finish_commit_opening() noexcept {
        return VoidResult::of();
    }

    virtual CommitID commit_id() noexcept {
        return Base::commit_id();
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id) noexcept
    {
        MEMORIA_TRY_VOID(checkIfConainersCreationAllowed());
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(self_ptr(), ctr_id, decl);
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> create(const LDTypeDeclarationView& decl) noexcept
    {
        MEMORIA_TRY_VOID(checkIfConainersCreationAllowed());
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());

        MEMORIA_TRY(ctr_name, createCtrName());
        return factory->create_instance(self_ptr(), ctr_name, decl);
    }

    virtual VoidResult commit(bool) noexcept {
        if (!committed_)
        {
            MEMORIA_TRY_VOID(flush_open_containers());
            MEMORIA_TRY_VOID(flush_updated_entries());

            if (superblock_->is_updated()) {
                superblock_->clear_updates();
                MEMORIA_TRY_VOID(write_data(DirectoryCtrID, superblock_, superblock_->superblock_size(), system_db_));
            }

            if (const int rc = mma_mdb_txn_commit(transaction_)) {
                return make_generic_error("Can't commit read-write transaction, error = {}", mma_mdb_strerror(rc));
            }

            committed_ = true;
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Transaction {} has been already committed", commit_id());
        }

        return VoidResult::of();
    }

    virtual VoidResult flush_open_containers() noexcept {
        for (const auto& pair: instance_map_)
        {
            MEMORIA_TRY_VOID(pair.second->flush());
        }

        return VoidResult::of();
    }

    virtual BoolResult drop_ctr(const CtrID& name) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());

        MEMORIA_TRY(root_id, getRootID(name));

        if (root_id.is_set())
        {
            MEMORIA_TRY(block, getBlock(root_id));

            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(block->ctr_type_hash());

            MEMORIA_TRY_VOID(ctr_intf->drop(name, self_ptr()));
            return BoolResult::of(true);
        }
        else {
            return BoolResult::of(false);
        }
    }

    Result<CtrID> clone_ctr(const CtrID& ctr_name) noexcept {
        return clone_ctr(ctr_name, CtrID{});
    }

    Result<CtrID> clone_ctr(const CtrID& ctr_name, const CtrID& new_ctr_name) noexcept
    {
        MEMORIA_TRY(root_id, getRootID(ctr_name));
        MEMORIA_TRY(block, getBlock(root_id));

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->clone_ctr(ctr_name, new_ctr_name, self_ptr());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot", ctr_name);
        }
    }


    virtual VoidResult set_persistent(bool persistent) noexcept
    {
        return make_generic_error("Method set_persistent(bool) is not implemented for LMDBStore commit");
    }

    virtual bool is_persistent() noexcept {
        return false;
    }


    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> find(const CtrID& ctr_id) noexcept {
        return Base::find(ctr_id);
    }

    virtual bool is_committed() const noexcept {
        return committed_;
    }

    virtual bool is_active() const noexcept {
        return !committed_;
    }

    virtual bool is_marked_to_clear() const noexcept {
        return false;
    }


    virtual VoidResult dump_open_containers() noexcept {
        return Base::dump_open_containers();
    }

    virtual BoolResult has_open_containers() noexcept {
        return Base::has_open_containers();
    }

    virtual Result<std::vector<CtrID>> container_names() const noexcept {
        return Base::container_names();
    }

    virtual BoolResult check() noexcept {
        return Base::check();
    }

    virtual Result<Optional<U8String>> ctr_type_name_for(const CtrID& name) noexcept {
        return Base::ctr_type_name_for(name);
    }

    virtual VoidResult walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) noexcept {
        return Base::walk_containers(walker, allocator_descr);
    }

    virtual VoidResult setRoot(const CtrID& ctr_id, const BlockID& root) noexcept
    {
        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            if (!root.is_null())
            {
                superblock_->directory_root_id() = root;
                superblock_->touch();
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Containers directory removal attempted");
            }
        }
        else {
            if (root.is_set())
            {
                MEMORIA_TRY_VOID(directory_ctr_->replace_and_return(ctr_id, root));
            }
            else {
                MEMORIA_TRY_VOID(directory_ctr_->remove_and_return(ctr_id));
            }
        }

        return VoidResult::of();
    }

    virtual VoidResult removeBlock(const BlockID& id) noexcept
    {
        auto res = block_cache_.has_entry(id);

        if (!res)
        {
            return remove_data(id, data_db_);
        }
        else {
            BlockCacheEntry* entry = res.get();

            if (entry->is_linked()) {
                block_cache_.remove(id);
                forget_entry(entry);
            }
            else {
                entry->state() = ENTRY_STATE_DELETED;
            }

            return VoidResult::of();
        }
    }

    virtual Result<SharedBlockPtr> createBlock(int32_t initial_size) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());
        using ResultT = Result<SharedBlockPtr>;

        if (initial_size == -1) {
            initial_size = BASIC_BLOCK_SIZE;
        }

        initial_size = nearest_log2(initial_size) - LMDB_HEADER_SIZE;

        uint8_t* block_addr = ptr_cast<uint8_t>(::malloc(initial_size));

        std::memset(block_addr, 0, initial_size);

        MEMORIA_TRY(id, newId());
        BlockType* block = new (block_addr) BlockType(id, id);
        block->init();

        block->memory_block_size() = initial_size;

        BlockCacheEntry* entry = block_cache_entry_pool_.construct(id, block, ENTRY_STATE_ACTIVE);
        entry->set_allocator(this);

        block_cache_.insert(entry);
        updated_entries_.push_back(*entry);

        return ResultT::of(entry);
    }

    virtual Result<SharedBlockPtr> cloneBlock(const SharedBlockPtr& block) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());
        using ResultT = Result<SharedBlockPtr>;

        int32_t block_size = block->memory_block_size();

        uint8_t* block_addr = ptr_cast<uint8_t>(::malloc(block_size));

        std::memcpy(block_addr, block.block(), block_size);

        MEMORIA_TRY(id, newId());
        BlockType* new_block = ptr_cast<BlockType>(block_addr);
        new_block->id()   = id;
        new_block->uuid() = id;

        new_block->set_references(0);

        BlockCacheEntry* entry = block_cache_entry_pool_.construct(id, new_block, ENTRY_STATE_ACTIVE);
        entry->set_allocator(this);

        block_cache_.insert(entry);
        updated_entries_.push_back(*entry);

        return ResultT::of(SharedBlockPtr{entry});
    }

protected:

    virtual Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>> snapshot_ref_creation_allowed() noexcept {
        using ResultT = Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>>;
        return ResultT::of();
    }


    virtual Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>> snapshot_ref_opening_allowed() noexcept {
        return Base::snapshot_ref_opening_allowed();
    }

    virtual SnpSharedPtr<AllocatorT> self_ptr() noexcept {
        return this->shared_from_this();
    }

private:
    template <typename CtrName, typename CtrInstanceVar>
    void internal_init_system_ctr(
            MaybeError& maybe_error,
            CtrInstanceVar& assign_to,
            const BlockID& root_block_id,
            const CtrID& ctr_id
    ) noexcept
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            if (root_block_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<CtrName>(root_block_id);
                MEMORIA_RETURN_IF_ERROR(ctr_ref);
                assign_to = ctr_ref.get();                
            }
            else {
                auto ctr_ref = this->template internal_create_by_name_typed<CtrName>(ctr_id);
                MEMORIA_RETURN_IF_ERROR(ctr_ref);
                assign_to = ctr_ref.get();
            }

            assign_to->internal_reset_allocator_holder();

            return VoidResult::of();
        });
    }

    VoidResult checkIfConainersCreationAllowed() noexcept {
        if (MMA_UNLIKELY(committed_)) {
            return make_generic_error("Snapshot has been already committed");
        }
        return VoidResult::of();
    }

    VoidResult check_updates_allowed() noexcept {
        if (MMA_UNLIKELY(committed_)) {
            return make_generic_error("Snapshot has been already committed");
        }

        return VoidResult::of();
    }

private:
    virtual Result<SharedBlockPtr> updateBlock(Shared* block) noexcept {
        MEMORIA_TRY_VOID(check_updates_allowed());

        BlockCacheEntry* entry = ptr_cast<BlockCacheEntry>(block);

        if (!entry->is_updated()) {
            updated_entries_.push_back(*entry);
        }

        return Result<SharedBlockPtr>::of(block);
    }

    virtual VoidResult resizeBlock(Shared* block, int32_t new_size) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());

        int32_t block_size = block->get()->memory_block_size();

        new_size = nearest_log2(new_size) - LMDB_HEADER_SIZE;

        uint8_t* block_addr = ptr_cast<uint8_t>(::malloc(new_size));

        int32_t transfer_size = std::min(new_size, block_size);

        std::memcpy(block_addr, block->get(), transfer_size);

        BlockType* new_block = ptr_cast<BlockType>(block_addr);

        auto res = ProfileMetadata<Profile>::local()
                ->get_block_operations(new_block->ctr_type_hash(), new_block->block_type_hash())
                ->resize(block->get(), new_block, new_size);

        if (MMA_UNLIKELY(res.is_error())) {
            ::free(block_addr);
            return std::move(res).transfer_error();
        }

        block->set_block(new_block);

        BlockCacheEntry* entry = ptr_cast<BlockCacheEntry>(block);

        if (!entry->is_updated()) {
            updated_entries_.push_back(*entry);
        }

        return VoidResult::of();
    }

    virtual VoidResult releaseBlock(Shared* block) noexcept
    {
        BlockCacheEntry* entry = ptr_cast<BlockCacheEntry>(block);

        if (entry->state() == ENTRY_STATE_ACTIVE) {
            return block_cache_.attach(entry);
        }
        else {
            forget_entry(entry);
            return VoidResult::of();
        }
    }

    virtual Result<SharedBlockConstPtr> getBlock(const BlockID& id) noexcept
    {
        if (MMA_UNLIKELY(committed_)) {
            return make_generic_error("Snapshot has been already committed");
        }

        using ResultT = Result<SharedBlockConstPtr>;

        if (MMA_UNLIKELY(id.is_null())) {
            return ResultT::of();
        }

        auto block = block_cache_.get(id);
        if (block) {
            BlockCacheEntry* entry = block.get();
            if (entry->get()) {
                return ResultT::of(block.get());
            }
            else {
                MEMORIA_TRY(block_ptr, get_data_addr(id, data_db_));
                if (block_ptr.mv_data)
                {
                    BlockType* block_data = ptr_cast<BlockType>(::malloc(block_ptr.mv_size));
                    std::memcpy(block_data, block_ptr.mv_data, block_ptr.mv_size);
                    entry->set_block(block_data);
                    return ResultT::of(SharedBlockConstPtr(entry));
                }
                else {
                    return make_generic_error("Block {} is not found in the data_db", id);
                }
            }
        }
        else {
            MEMORIA_TRY(block_ptr, get_data_addr(id, data_db_));
            if (block_ptr.mv_data) {
                BlockType* block = ptr_cast<BlockType>(::malloc(block_ptr.mv_size));
                std::memcpy(block, block_ptr.mv_data, block_ptr.mv_size);

                BlockCacheEntry* entry = block_cache_entry_pool_.construct(id, block, ENTRY_STATE_ACTIVE);
                entry->set_allocator(this);

                block_cache_.insert(entry);

                return ResultT::of(SharedBlockConstPtr(entry));
            }
            else {
                return make_generic_error("Block {} is not found in the data_db", id);
            }
        }
    }


    virtual bool isActive() const noexcept {
        return is_active();
    }

    virtual Result<BlockID> newId() noexcept {
        UUID uuid{};
        uuid.lo() = superblock_->new_block_id();
        return uuid;
    }

    virtual VoidResult drop() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("drop() is not supported for LMDB store");
    }

    VoidResult evictionFn(bool keep_entry, BlockCacheEntry* entry) noexcept
    {
        if (entry->is_updated()) {
            MEMORIA_TRY_VOID(write_data(entry->id(), entry->get(), entry->get()->memory_block_size(), data_db_));
        }

        if (keep_entry) {
            ::free(entry->get());
            entry->set_block(static_cast<BlockType*>(nullptr));

            if (entry->is_updated()) {
                updated_entries_.erase(updated_entries_.iterator_to(*entry));
            }
        }
        else {
            forget_entry(entry);
        }

        return VoidResult::of();
    }

    void forget_entry(BlockCacheEntry* entry) noexcept
    {
        ::free(entry->get());

        if (entry->is_updated()) {
            updated_entries_.erase(updated_entries_.iterator_to(*entry));
        }

        block_cache_entry_pool_.destroy(entry);
    }

    VoidResult flush_updated_entries() noexcept
    {
        while (updated_entries_.size())
        {
            auto ii = updated_entries_.begin();
            BlockCacheEntry* entry = ptr_cast<BlockCacheEntry>(&*ii);
            MEMORIA_TRY_VOID(write_data(entry->id(), entry->get(), entry->get()->memory_block_size(), data_db_));

            updated_entries_.erase(ii);
        }

        return VoidResult::of();
    }

    VoidResult write_data(const BlockID& block_id, void* bytes, size_t size, MDB_dbi dbi) noexcept
    {
        MDB_val key = {sizeof(block_id), ptr_cast<void>(&block_id)};
        MDB_val data = {size, bytes};

        if (int rc = mma_mdb_put(transaction_, dbi, &key, &data, 0)) {
            return make_generic_error("Can't write block {} of {} bytes to the database, error = {}", block_id, size, mma_mdb_strerror(rc));
        }

        return VoidResult::of();
    }

    VoidResult remove_data(const BlockID& block_id, MDB_dbi dbi) noexcept
    {
        MDB_val key = {sizeof(block_id), ptr_cast<void>(&block_id)};
        MDB_val data{};

        if (int rc = mma_mdb_del(transaction_, dbi, &key, &data)) {
            return make_generic_error("Can't delete block {} from the database, error = {}", block_id, mma_mdb_strerror(rc));
        }

        return VoidResult::of();
    }

    static constexpr int32_t nearest_log2(int32_t value) noexcept {
        int32_t vv = (1 << (Log2(value)));
        return (vv / 2 == value) ? value : vv;
    }
};

}
