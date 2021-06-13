
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
        public ProfileRWStoreType<Profile>,
        public ISWMRStoreWritableCommit<ApiProfile<Profile>>,
        public EnableSharedFromThis<LMDBStoreWritableCommit<Profile>>
{
    using Base = LMDBStoreCommitBase<Profile>;

    using typename Base::Store;
    using typename Base::CtrID;
    using typename Base::CtrReferenceableResult;
    using typename Base::StoreT;
    using typename Base::CommitID;
    using typename Base::BlockID;
    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockType;
    using typename Base::ApiProfileT;
    using typename Base::Shared;
    using typename Base::Superblock;

    using typename Base::DirectoryCtrType;

    using RWStoreT = ProfileRWStoreType<Profile>;

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

        }
    }

    void post_init(MaybeError& maybe_error) noexcept
    {
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

    void finish_store_initialization() {
        return commit(true);
    }

    void finish_commit_opening() {
    }

    virtual CommitID commit_id() {
        return Base::commit_id();
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_mutable_instance(self_ptr(), rw_self_ptr(), ctr_id, decl);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());

        auto ctr_name = createCtrName();
        return factory->create_mutable_instance(self_ptr(), rw_self_ptr(), ctr_name, decl);
    }

    virtual void commit(bool) {
        if (!committed_)
        {
            flush_open_containers();
            flush_updated_entries();

            if (superblock_->is_updated()) {
                superblock_->clear_updates();
                write_data(DirectoryCtrID, superblock_, superblock_->superblock_size(), system_db_);
            }

            if (const int rc = mma_mdb_txn_commit(transaction_)) {
                make_generic_error("Can't commit read-write transaction, error = {}", mma_mdb_strerror(rc)).do_throw();
            }

            committed_ = true;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Transaction {} has been already committed", commit_id()).do_throw();
        }
    }

    virtual void flush_open_containers() {
        for (const auto& pair: instance_map_)
        {
            pair.second->flush();
        }
    }

    virtual bool drop_ctr(const CtrID& name)
    {
        check_updates_allowed();

        auto root_id = getRootID(name);

        if (root_id.is_set())
        {
            auto block = getBlock(root_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(block->ctr_type_hash());

            ctr_intf->drop(name, self_ptr(), rw_self_ptr());
            return true;
        }
        else {
            return false;
        }
    }

    CtrID clone_ctr(const CtrID& ctr_name) {
        return clone_ctr(ctr_name, CtrID{});
    }

    CtrID clone_ctr(const CtrID& ctr_name, const CtrID& new_ctr_name)
    {
        auto root_id = getRootID(ctr_name);
        auto block   = getBlock(root_id);

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->clone_ctr(ctr_name, new_ctr_name, self_ptr(), rw_self_ptr());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot", ctr_name).do_throw();
        }
    }


    virtual void set_persistent(bool persistent)
    {
        make_generic_error("Method set_persistent(bool) is not implemented for LMDBStore commit").do_throw();
    }

    virtual bool is_persistent() noexcept {
        return false;
    }


    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id) {
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


    virtual void dump_open_containers() {
        return Base::dump_open_containers();
    }

    virtual bool has_open_containers() {
        return Base::has_open_containers();
    }

    virtual std::vector<CtrID> container_names() const {
        return Base::container_names();
    }

    virtual bool check() {
        return Base::check();
    }

    virtual Optional<U8String> ctr_type_name_for(const CtrID& name) {
        return Base::ctr_type_name_for(name);
    }

    virtual void walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) {
        return Base::walk_containers(walker, allocator_descr);
    }

    virtual void setRoot(const CtrID& ctr_id, const BlockID& root)
    {
        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            if (!root.is_null())
            {
                superblock_->directory_root_id() = root;
                superblock_->touch();
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Containers directory removal attempted").do_throw();
            }
        }
        else {
            if (root.is_set())
            {
                directory_ctr_->replace_and_return(ctr_id, root);
            }
            else {
                directory_ctr_->remove_and_return(ctr_id);
            }
        }
    }

    virtual void removeBlock(const BlockID& id)
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
        }
    }

    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID&)
    {
        check_updates_allowed();

        if (initial_size == -1) {
            initial_size = BASIC_BLOCK_SIZE;
        }

        initial_size = nearest_log2(initial_size) - LMDB_HEADER_SIZE;

        uint8_t* block_addr = ptr_cast<uint8_t>(::malloc(initial_size));

        std::memset(block_addr, 0, initial_size);

        auto id = newId();
        BlockType* block = new (block_addr) BlockType(id, id);

        block->memory_block_size() = initial_size;

        BlockCacheEntry* entry = block_cache_entry_pool_.construct(id, block, ENTRY_STATE_ACTIVE);
        entry->set_allocator(this);

        block_cache_.insert(entry);
        updated_entries_.push_back(*entry);

        return entry;
    }

    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID&)
    {
        check_updates_allowed();
        int32_t block_size = block->memory_block_size();

        uint8_t* block_addr = ptr_cast<uint8_t>(::malloc(block_size));

        std::memcpy(block_addr, block.block(), block_size);

        auto id = newId();
        BlockType* new_block = ptr_cast<BlockType>(block_addr);
        new_block->id()   = id;
        new_block->uuid() = id;

        new_block->set_references(0);

        BlockCacheEntry* entry = block_cache_entry_pool_.construct(id, new_block, ENTRY_STATE_ACTIVE);
        entry->set_allocator(this);

        block_cache_.insert(entry);
        updated_entries_.push_back(*entry);

        return SharedBlockPtr{entry};
    }

protected:

    virtual SnpSharedPtr<ROStoreApiBase<ApiProfileT>> snapshot_ref_creation_allowed() {
        return SnpSharedPtr<ROStoreApiBase<ApiProfileT>>{};
    }


    virtual SnpSharedPtr<ROStoreApiBase<ApiProfileT>> snapshot_ref_opening_allowed() {
        return Base::snapshot_ref_opening_allowed();
    }

    virtual SnpSharedPtr<StoreT> self_ptr() noexcept {
        return this->shared_from_this();
    }

    virtual SnpSharedPtr<RWStoreT> rw_self_ptr() noexcept {
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
                assign_to = ctr_ref;
            }
            else {
                auto ctr_ref = this->template internal_create_by_name_typed<CtrName>(ctr_id);

                assign_to = ctr_ref;
            }

            assign_to->internal_reset_allocator_holder();

            return VoidResult::of();
        });
    }

    void checkIfConainersCreationAllowed() {
        if (MMA_UNLIKELY(committed_)) {
            make_generic_error("Snapshot has been already committed").do_throw();
        }
    }

    void check_updates_allowed() {
        if (MMA_UNLIKELY(committed_)) {
            make_generic_error("Snapshot has been already committed").do_throw();
        }
    }

private:
    virtual void updateBlock(Shared* block) {
        check_updates_allowed();

        BlockCacheEntry* entry = ptr_cast<BlockCacheEntry>(block);

        if (!entry->is_updated()) {
            updated_entries_.push_back(*entry);
        }
    }

    virtual void resizeBlock(Shared* block, int32_t new_size)
    {
        check_updates_allowed();

        int32_t block_size = block->get()->memory_block_size();

        new_size = nearest_log2(new_size) - LMDB_HEADER_SIZE;

        auto block_ptr = allocate_system<uint8_t>(new_size);

        uint8_t* block_addr = block_ptr.get();

        int32_t transfer_size = std::min(new_size, block_size);

        std::memcpy(block_addr, block->get(), transfer_size);

        BlockType* new_block = ptr_cast<BlockType>(block_addr);

        ProfileMetadata<Profile>::local()
                ->get_block_operations(new_block->ctr_type_hash(), new_block->block_type_hash())
                ->resize(block->get(), new_block, new_size);

        block->set_block(new_block);

        BlockCacheEntry* entry = ptr_cast<BlockCacheEntry>(block);

        if (!entry->is_updated()) {
            updated_entries_.push_back(*entry);
        }

        block_ptr.release();
    }

    virtual void releaseBlock(Shared* block) noexcept
    {
        BlockCacheEntry* entry = ptr_cast<BlockCacheEntry>(block);

        if (entry->state() == ENTRY_STATE_ACTIVE) {
            return block_cache_.attach(entry);
        }
        else {
            forget_entry(entry);
        }
    }

    virtual SharedBlockConstPtr getBlock(const BlockID& id)
    {
        if (MMA_UNLIKELY(committed_)) {
            make_generic_error("Snapshot has been already committed").do_throw();
        }

        if (MMA_UNLIKELY(id.is_null())) {
            return SharedBlockConstPtr{};
        }

        auto block = block_cache_.get(id);
        if (block) {
            BlockCacheEntry* entry = block.get();
            if (entry->get()) {
                return block.get();
            }
            else {
                auto block_ptr = get_data_addr(id, data_db_);
                if (block_ptr.mv_data)
                {
                    BlockType* block_data = ptr_cast<BlockType>(::malloc(block_ptr.mv_size));
                    std::memcpy(block_data, block_ptr.mv_data, block_ptr.mv_size);
                    entry->set_block(block_data);
                    return SharedBlockConstPtr(entry);
                }
                else {
                    make_generic_error("Block {} is not found in the data_db", id).do_throw();
                }
            }
        }
        else {
            auto block_ptr = get_data_addr(id, data_db_);
            if (block_ptr.mv_data) {
                BlockType* block = ptr_cast<BlockType>(::malloc(block_ptr.mv_size));
                std::memcpy(block, block_ptr.mv_data, block_ptr.mv_size);

                BlockCacheEntry* entry = block_cache_entry_pool_.construct(id, block, ENTRY_STATE_ACTIVE);
                entry->set_allocator(this);

                block_cache_.insert(entry);

                return SharedBlockConstPtr(entry);
            }
            else {
                make_generic_error("Block {} is not found in the data_db", id).do_throw();
            }
        }
    }


    virtual bool isActive() const noexcept {
        return is_active();
    }

    virtual BlockID newId() noexcept {
        UUID uuid = uuid_pack_uint64_t(superblock_->new_block_id());
        return uuid;
    }

    virtual void drop() noexcept {
        MEMORIA_MAKE_GENERIC_ERROR("drop() is not supported for LMDBStore").do_throw();
    }

    void evictionFn(bool keep_entry, BlockCacheEntry* entry)
    {
        if (entry->is_updated()) {
            write_data(entry->id(), entry->get(), entry->get()->memory_block_size(), data_db_);
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
    }

    void forget_entry(BlockCacheEntry* entry) noexcept
    {
        ::free(entry->get());

        if (entry->is_updated()) {
            updated_entries_.erase(updated_entries_.iterator_to(*entry));
        }

        block_cache_entry_pool_.destroy(entry);
    }

    void flush_updated_entries()
    {
        while (updated_entries_.size())
        {
            auto ii = updated_entries_.begin();
            BlockCacheEntry* entry = ptr_cast<BlockCacheEntry>(&*ii);
            write_data(entry->id(), entry->get(), entry->get()->memory_block_size(), data_db_);

            updated_entries_.erase(ii);
        }
    }

    void write_data(const BlockID& block_id, void* bytes, size_t size, MDB_dbi dbi)
    {
        MDB_val key = {sizeof(block_id), ptr_cast<void>(&block_id)};
        MDB_val data = {size, bytes};

        if (int rc = mma_mdb_put(transaction_, dbi, &key, &data, 0)) {
            make_generic_error("Can't write block {} of {} bytes to the database, error = {}", block_id, size, mma_mdb_strerror(rc)).do_throw();
        }
    }

    void remove_data(const BlockID& block_id, MDB_dbi dbi)
    {
        MDB_val key = {sizeof(block_id), ptr_cast<void>(&block_id)};
        MDB_val data{};

        if (int rc = mma_mdb_del(transaction_, dbi, &key, &data)) {
            make_generic_error("Can't delete block {} from the database, error = {}", block_id, mma_mdb_strerror(rc)).do_throw();
        }
    }

    static constexpr int32_t nearest_log2(int32_t value) noexcept {
        int32_t vv = (1 << (Log2(value)));
        return (vv / 2 == value) ? value : vv;
    }

    CtrSharedPtr<CtrReferenceable<ApiProfileT>> new_ctr_instance(
            ContainerOperationsPtr<Profile> ctr_intf,
            SharedBlockConstPtr block
    )
    {
        return ctr_intf->new_mutable_ctr_instance(block, self_ptr(), rw_self_ptr());
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const LDTypeDeclarationView& decl, const CtrID& ctr_id
    )
    {
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_mutable_instance(this, this, ctr_id, decl);
    }
};

}
