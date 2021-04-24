
// Copyright 2016 Victor Smirnov
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

#include <memoria/core/memory/malloc.hpp>

#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/store/memory_nocow/common/store_stat.hpp>

#include <memoria/core/container/store.hpp>
#include <memoria/core/container/ctr_impl.hpp>

#include <memoria/core/exceptions/exceptions.hpp>


#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/containers/map/map_factory.hpp>
#include <memoria/core/tools/pair.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/tools/simple_2q_cache.hpp>

#include <memoria/core/datatypes/type_registry.hpp>

#include <memoria/core/linked/document/linked_document.hpp>

#include <memoria/store/memory_nocow/common/persistent_tree.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <boost/pool/object_pool.hpp>

#include <vector>
#include <memory>
#include <mutex>

namespace memoria {
namespace store {
namespace memory_nocow {

template <typename Profile, typename PersistentAllocator, typename SnapshotType>
class SnapshotBase:
        public ProfileStoreType<Profile>,
        public IMemorySnapshot<ApiProfile<Profile>>,
        public SnpSharedFromThis<SnapshotType>
{
protected:
	using MyType			= SnapshotType;
    using Base              = ProfileStoreType<Profile>;

public:
    using ProfileT = Profile;

    using typename Base::CtrID;
    using typename Base::BlockID;
    using typename Base::BlockGUID;
    using typename Base::BlockType;
    using typename Base::SnapshotID;

    using ApiProfileT = ApiProfile<Profile>;
protected:

    using HistoryNode       = typename PersistentAllocator::HistoryNode;
    using PersistentTreeT   = typename PersistentAllocator::PersistentTreeT;
    
    using PersistentAllocatorPtr = AllocSharedPtr<PersistentAllocator>;
    using SnapshotPtr            = SnpSharedPtr<MyType>;
    using SnapshotApiPtr         = SnpSharedPtr<IMemorySnapshot<ApiProfileT>>;
    using AllocatorPtr           = AllocSharedPtr<Base>;
    
    using NodeBaseT         = typename PersistentTreeT::NodeBaseT;
    using LeafNodeT         = typename PersistentTreeT::LeafNodeT;
    using PTreeValue        = typename LeafNodeT::Value;
    
    using RCBlockPtr		= typename std::remove_pointer<typename PersistentTreeT::Value::Value>::type;

    using Status            = typename HistoryNode::Status;

    using CtrInstanceMap  = std::unordered_map<CtrID, CtrReferenceable<ApiProfileT>*>;

    using BlockGuardCache = SimpleTwoQueueCache<BlockID, typename Base::Shared>;
    using BlockCacheEntry = typename BlockGuardCache::EntryT;

public:

    template <typename CtrName>
    using CtrT = SharedCtr<CtrName, ProfileStoreType<Profile>, Profile>;

    template <typename CtrName>
    using CtrPtr = CtrSharedPtr<CtrT<CtrName>>;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::Shared;

    using RootMapType = CtrT<Map<CtrID, BlockID>>;

protected:

    HistoryNode*            history_node_;
    PersistentAllocatorPtr  history_tree_;
    PersistentAllocator*    history_tree_raw_ = nullptr;

    PersistentTreeT persistent_tree_;

    Logger logger_;

    CtrInstanceMap instance_map_;

    template <typename>
    friend class ThreadsMemoryStoreImpl;
    
    template <typename>
    friend class FibersMemoryStoreImpl;
    
    
    template <typename, typename>
    friend class MemoryStoreBase;
    
    PairPtr pair_;
    
    CtrSharedPtr<RootMapType> root_map_;

    mutable ObjectPools object_pools_;
    mutable boost::object_pool<BlockCacheEntry> block_shared_cache_pool_;
    mutable BlockGuardCache block_shared_cache_;

public:

    SnapshotBase(MaybeError& maybe_error, HistoryNode* history_node, const PersistentAllocatorPtr& history_tree):
        history_node_(history_node),
        history_tree_(history_tree),
        history_tree_raw_(history_tree.get()),
        persistent_tree_(history_node_),
        logger_("PersistentInMemStoreSnp", Logger::DERIVED, &history_tree->logger_),
        block_shared_cache_(1024)
    {
        history_node_->ref();

        if (history_node->is_active())
        {
            history_tree_raw_->ref_active();
        }
    }

    SnapshotBase(MaybeError& maybe_error, HistoryNode* history_node, PersistentAllocator* history_tree):
        history_node_(history_node),
        history_tree_raw_(history_tree),
        persistent_tree_(history_node_),
        logger_("PersistentInMemStoreSnp"),
        block_shared_cache_(1024)
    {
        history_node_->ref();

        if (history_node->is_active())
        {
            history_tree_raw_->ref_active();
        }
    }
    
    void post_init()
    {
        auto ptr = this->shared_from_this();

        BlockID root_id = history_node_->root_id();

        MaybeError maybe_error;
        if (root_id.isSet())
        {
            auto root_block = findBlock(root_id);
            root_map_ = ctr_make_shared<RootMapType>(maybe_error, ptr, root_block);
        }
        else {
            root_map_ = ctr_make_shared<RootMapType>(maybe_error, ptr, CtrID{}, Map<CtrID, BlockID>());
        }

        root_map_->reset_allocator_holder();

        if (maybe_error) {
            std::move(maybe_error.get()).do_throw();
        }
    }
    
    static void init_profile_metadata() {
        RootMapType::init_profile_metadata();
    }


    virtual ~SnapshotBase() noexcept
    {
    }

    virtual ObjectPools& object_pools() const noexcept {
        return object_pools_;
    }
    
    virtual SnpSharedPtr<ProfileStoreType<Profile>> self_ptr() noexcept {
        return this->shared_from_this();
    }
    
    PairPtr& pair() noexcept {
        return pair_;
    }

    const PairPtr& pair() const noexcept {
        return pair_;
    }

    const CtrID& uuid() const noexcept {
        return history_node_->snapshot_id();
    }

    bool is_active() const noexcept {
        return history_node_->is_active();
    }

    bool is_data_locked() const noexcept {
    	return history_node_->is_data_locked();
    }

    virtual bool isActive() const noexcept {
        return is_active();
    }

    bool is_marked_to_clear() const noexcept {
        return history_node_->is_dropped();
    }

    bool is_committed() const noexcept {
        return history_node_->is_committed();
    }

    std::vector<CtrID> container_names() const
    {
        std::vector<CtrID> names;

        auto ii = root_map_->ctr_begin();

        while (!ii->iter_is_end())
        {
            names.push_back(ii->key());
            ii->next();
        }

        return std::move(names);
    }

    std::vector<U8String> container_names_str() const
    {
        std::vector<U8String> names;

        auto ii = root_map_->ctr_begin();
        while (!ii->iter_is_end())
        {
            std::stringstream ss;
            ss << ii->key().view();

            names.push_back(U8String(ss.str()));
            ii->next();
        }

        return std::move(names);
    }


    bool drop_ctr(const CtrID& name)
    {
        checkUpdateAllowed();

        auto root_id = getRootID(name);

        if (root_id.is_set())
        {
            auto block = this->getBlock(root_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(block->ctr_type_hash());

            ctr_intf->drop(name, this->shared_from_this());
            return true;
        }
        else {
            return false;
        }
    }


    Optional<U8String> ctr_type_name_for(const CtrID& name)
    {
        auto root_id = this->getRootID(name);

        auto block = this->getBlock(root_id);

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            return ctr_intf->ctr_type_name();
        }
        else {
            return Optional<U8String>{};
        }
    }

    void set_as_master()
    {
        return history_tree_raw_->set_master(uuid());
    }

    void set_as_branch(U8StringRef name)
    {
        return history_tree_raw_->set_branch(name, uuid());
    }

    U8StringRef metadata() const
    {
        return history_node_->metadata();
    }

    void set_metadata(U8StringRef metadata)
    {
        if (history_node_->is_active())
        {
            history_node_->set_metadata(metadata);
        }
        else
        {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot is already committed.").do_throw();
        }
    }

    void for_each_ctr_node(const CtrID& name, typename ContainerOperations<Profile>::BlockCallbackFn fn)
    {
        auto root_id = this->getRootID(name);
        auto block = this->getBlock(root_id);

        if (block)
    	{
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->for_each_ctr_node(name, this->shared_from_this(), fn);
    	}
    	else {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot {}", name, history_node_->snapshot_id()).do_throw();
    	}
    }



    void import_new_ctr_from(SnapshotApiPtr ptr, const CtrID& name)
    {
        checkUpdateAllowed();

        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

        txn->checkIfExportAllowed();
        auto root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

        if (root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&](const BlockGUID&, const BlockID& id, const void*) {
                auto rc_handle = txn->export_block_rchandle(id);
                using Value = typename PersistentTreeT::Value;

                rc_handle->ref();

                auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

                if (old_value.block_ptr())
                {
                    MEMORIA_MAKE_GENERIC_ERROR("Block with ID {} is not new in snapshot {}", id, txn_id).do_throw();
                }

            });



            auto root_id = txn->getRootID(name);

            if (root_id.is_set())
            {
                root_map_->assign(name, root_id);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId()).do_throw();
            }
    	}
    	else {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} already exists in snapshot {}", name, txn_id).do_throw();
    	}
    }


    void copy_new_ctr_from(SnapshotApiPtr ptr, const CtrID& name)
    {
        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

        txn->checkReadAllowed();
        checkUpdateAllowed();

        auto root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&](const BlockGUID&, const BlockID&, const void* block_data) {
                return clone_foreign_block(ptr_cast<const BlockType>(block_data));
            });

            auto root_id1 = txn->getRootID(name);

            if (root_id1.is_set()) {
                root_map_->assign(name, root_id1);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId()).do_throw();
            }
    	}
    	else {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} already exists in snapshot {}", name, txn_id).do_throw();
    	}
    }


    void import_ctr_from(SnapshotApiPtr ptr, const CtrID& name)
    {
        checkUpdateAllowed();

        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

        txn->checkIfExportAllowed();

        auto root_id = this->getRootID(name);

    	auto txn_id = uuid();

        if (!root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&, this](const BlockGUID& uuid, const BlockID& id, const void*) {
                auto block = this->getBlock(id);

                if (block && block->uuid() == uuid)
                {
                    return;
                }

                auto rc_handle = txn->export_block_rchandle(id);
                using Value = typename PersistentTreeT::Value;

                rc_handle->ref();

                auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

                if (old_value.block_ptr())
                {
                    if (old_value.block_ptr()->unref() == 0)
                    {
                        // FIXME: just delete the block?
                        MEMORIA_MAKE_GENERIC_ERROR("Unexpected refcount == 0 for block {}", old_value.block_ptr()->raw_data()->uuid()).do_throw();
                    }
                }
            });


            auto root_id = txn->getRootID(name);
            if (root_id.is_set())
            {
                root_map_->assign(name, root_id);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId()).do_throw();
            }
    	}
    	else {
            return import_new_ctr_from(txn, name);
    	}
    }


    void copy_ctr_from(SnapshotApiPtr ptr, const CtrID& name)
    {
        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

        txn->checkReadAllowed();
        checkUpdateAllowed();

        auto root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

        if (!root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&, this](const BlockGUID& uuid, const BlockID& id, const void* block_data) {
                auto block = this->getBlock(id);
                if (block && block->uuid() == uuid)
                {
                    return ;
                }

                return clone_foreign_block(ptr_cast<const BlockType>(block_data));
            });

            auto root_id1 = txn->getRootID(name);
            if (root_id1.is_set())
            {
                root_map_->assign(name, root_id1);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn_id).do_throw();
            }
    	}
    	else {
            return copy_new_ctr_from(txn, name);
    	}
    }


    CtrID clone_ctr(const CtrID& ctr_name) {
        return clone_ctr(ctr_name, CtrID{});
    }

    CtrID clone_ctr(const CtrID& ctr_name, const CtrID& new_ctr_name)
    {
        auto root_id = this->getRootID(ctr_name);
        auto block = this->getBlock(root_id);

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->clone_ctr(ctr_name, new_ctr_name, this->shared_from_this());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot {} ", ctr_name, history_node_->snapshot_id()).do_throw();
        }
    }

    SharedBlockConstPtr findBlock(const BlockID& id)
    {
        Shared* shared = get_shared(id, Shared::READ);

        if (!shared->get())
        {
            checkReadAllowed();

            auto block_opt = persistent_tree_.find(id);

            if (block_opt)
            {
                const auto& txn_id = history_node_->snapshot_id();

                if (block_opt.value().snapshot_id() != txn_id)
                {
                    shared->state() = Shared::READ;
                }
                else {
                    shared->state() = Shared::UPDATE;
                }

                shared->set_block(block_opt.value().block_ptr()->raw_data());
            }
            else {
                return SharedBlockConstPtr{};
            }
        }

        return SharedBlockConstPtr(shared);
    }

    virtual SharedBlockConstPtr getBlock(const BlockID& id)
    {
        if (id.isSet())
        {
            auto block = findBlock(id);

            if (block) {
                return block;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Block is not found for the specified id: {}", id).do_throw();
            }
        }
        else {
            return SharedBlockConstPtr{};
        }
    }

    void dumpAccess(const char* msg, const BlockID& id, const Shared* shared)
    {
        std::cout << msg << ": " << id << " " << shared->get() << " " << shared->get()->uuid() << " " << shared->state() << std::endl;
    }


    virtual void registerCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>* instance)
    {
        auto ii = instance_map_.find(ctr_id);
    	if (ii == instance_map_.end())
    	{
            instance_map_.insert({ctr_id, instance});
        }
    	else {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} has been already registered", ctr_id).do_throw();
    	}
    }

    virtual void unregisterCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>*)
    {
        instance_map_.erase(ctr_id);
    }

public:
    bool has_open_containers() {
        return instance_map_.size() > 1;
    }

    void dump_open_containers()
    {
    	for (const auto& pair: instance_map_)
    	{
            std::cout << pair.first << " -- " << pair.second->describe_type() << std::endl;
    	}
    }

    void flush_open_containers()
    {
        for (const auto& pair: instance_map_)
        {
            pair.second->flush();
        }
    }

    void dump_dictionary_blocks()
    {
        auto ii = root_map_->ctr_begin();
        if (!ii->iter_is_end())
        {
            do {
                ii->dump();

                auto has_next = ii->iter_next_leaf();
                if (!has_next) break;
            }
            while (true);
        }
    }


    virtual void updateBlock(Shared* shared)
    {
        // FIXME: Though this check prohibits new block acquiring for update,
        // already acquired updatable blocks can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed(CtrID{});

        if (shared->state() == Shared::READ)
        {
            auto new_block = clone_block(shared->get());

            ptree_set_new_block(new_block);

            shared->set_block(new_block);

            shared->state() = Shared::UPDATE;
        }
    }

    virtual void removeBlock(const BlockID& id)
    {
        checkUpdateAllowed(CtrID{});

        auto iter = persistent_tree_.locate(id);

        if (!iter.is_end())
        {
            auto shared = block_shared_cache_.get(id);

            if (!shared)
            {
                persistent_tree_.remove(iter);
            }
            else {
                shared.get()->state() = Shared::_DELETE;
            }
        }
    }






    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID&)
    {
        checkUpdateAllowed(CtrID{});

        if (initial_size == -1)
        {
            initial_size = DEFAULT_BLOCK_SIZE;
        }

        void* buf = allocate_system<uint8_t>(static_cast<size_t>(initial_size)).release();

        memset(buf, 0, static_cast<size_t>(initial_size));

        auto id = newId();

        BlockType* p = new (buf) BlockType(id, id);

        p->memory_block_size() = initial_size;

        BlockCacheEntry* shared = block_shared_cache_pool_.construct(id, p, Shared::UPDATE);
        shared->set_allocator(this);

        ptree_set_new_block(p);

        block_shared_cache_.insert(shared);

        return SharedBlockPtr(shared);
    }


    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID&)
    {
        checkUpdateAllowed(CtrID{});

        auto new_id = newId();


        auto new_block = this->clone_block(block.block());

        new_block->id() = new_id;

        BlockCacheEntry* new_shared  = block_shared_cache_pool_.construct(new_id, new_block, Shared::UPDATE);
        new_shared->set_allocator(this);

        ptree_set_new_block(new_block);

        block_shared_cache_.insert(new_shared);

        return SharedBlockPtr(new_shared);
    }


    virtual void resizeBlock(Shared* shared, int32_t new_size)
    {
        checkUpdateAllowed();

        BlockType* block = shared->get();

        if (shared->state() == Shared::READ)
        {
            auto buf = allocate_system<uint8_t>(new_size);

            BlockType* new_block = ptr_cast<BlockType>(buf.get());

            int32_t transfer_size = std::min(new_size, block->memory_block_size());

            std::memcpy(new_block, block, transfer_size);

            ProfileMetadata<Profile>::local()
                    ->get_block_operations(new_block->ctr_type_hash(), new_block->block_type_hash())
                    ->resize(block, new_block, new_size);

            shared->set_block(new_block);
            shared->state() = Shared::UPDATE;

            ptree_set_new_block(new_block);

            buf.release();
        }
        else if (shared->state() == Shared::UPDATE)
        {
            auto buf = allocate_system<uint8_t>(new_size);

            BlockType* new_block = ptr_cast<BlockType>(buf.get());

            int32_t transfer_size = std::min(new_size, block->memory_block_size());

            std::memcpy(new_block, block, transfer_size);

            ProfileMetadata<Profile>::local()
                    ->get_block_operations(new_block->ctr_type_hash(), new_block->block_type_hash())
                    ->resize(block, new_block, new_size);

            shared->set_block(new_block);

            ptree_set_new_block(new_block);

            buf.release();
        }
    }

    virtual void releaseBlock(Shared* shared) noexcept
    {
        if (shared->state() == Shared::_DELETE)
        {
            persistent_tree_.remove(shared->id());
            block_shared_cache_.remove_from_map(shared->id());
            block_shared_cache_pool_.destroy(static_cast<BlockCacheEntry*>(shared));
        }
        else {
            block_shared_cache_.attach(static_cast<BlockCacheEntry*>(shared), [&](BlockCacheEntry* entry){
                block_shared_cache_pool_.destroy(entry);
            });
        }
    }


    virtual BlockID newId() {
        return history_tree_raw_->newBlockId();
    }

    virtual CtrID currentTxnId() const noexcept {
        return history_node_->snapshot_id();
    }

    // memory pool allocator
    virtual void* allocateMemory(size_t size) {
        return allocate_system<uint8_t>(size).release();
    }

    virtual void freeMemory(void* ptr) noexcept {
        free_system(ptr);
    }

    virtual Logger& logger() noexcept {return logger_;}

    virtual BlockID getRootID(const CtrID& name)
    {
        if (!name.is_null())
        {
            auto iter = root_map_->ctr_map_find(name);

            if (iter->is_found(name))
            {
                return iter->value();
            }
            else {
                return BlockID{};
            }
        }
        else {
            return history_node_->root_id();
        }
    }

    virtual void setRoot(const CtrID& name, const BlockID& root)
    {
        if (root.is_null())
        {
            if (!name.is_null())
            {
                root_map_->remove(name);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Allocator directory removal attempted").do_throw();
            }
        }
        else {
            if (!name.is_null())
            {
                root_map_->assign(name, root);
            }
            else {
                history_node_->root_id() = root;
            }
        }
    }

    virtual bool hasRoot(const CtrID& name)
    {
        if (MMA_UNLIKELY(!root_map_)) {
            return false;
        }

        if (!name.is_null())
        {
            auto iter = root_map_->ctr_map_find(name);
            return iter->is_found(name);
        }
        else {
            return !history_node_->root_id().is_null();
        }
    }

    virtual CtrID createCtrName()
    {
        return ProfileTraits<Profile>::make_random_ctr_id();
    }


    virtual bool check()
    {
        bool result = false;

        auto iter = root_map_->ctr_begin();

        while(!iter->is_end())
        {
            auto ctr_name = iter->key();

            auto block = this->getBlock(iter->value());

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            auto res = ctr_intf->check(ctr_name, this->shared_from_this());

            result = res || result;

            iter->next();
        }

        return result;
    }

    U8String get_branch_suffix() const
    {
        return "";
    }

    virtual void walk_containers(ContainerWalker<ProfileT>* walker, const char* allocator_descr = nullptr) {
        return walkContainers(walker, allocator_descr);
    }

    virtual void walkContainers(ContainerWalker<ProfileT>* walker, const char* allocator_descr = nullptr)
    {
        if (allocator_descr != nullptr)
        {
            walker->beginSnapshot(fmt::format("Snapshot-{} -- {}", history_node_->snapshot_id(), allocator_descr).data());
        }
        else {
            walker->beginSnapshot(fmt::format("Snapshot-{}", history_node_->snapshot_id()).data());
        }

        auto iter = root_map_->ctr_begin();
        while (!iter->iter_is_end())
        {
            auto ctr_name   = iter->key();
            auto root_id    = iter->value();

            auto block = this->getBlock(root_id);

            auto ctr_hash   = block->ctr_type_hash();
            auto ctr_intf   = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            ctr_intf->walk(ctr_name, this->shared_from_this(), walker);

            iter->next();
        }

        walker->endSnapshot();
    }



    virtual void dump_persistent_tree() {
        persistent_tree_.dump_tree();
    }


    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(this->shared_from_this(), ctr_id, decl);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_cxx_typedecl());

        auto ctr_name = this->createCtrName();

        return factory->create_instance(this->shared_from_this(), ctr_name, decl);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id)
    {
        checkIfConainersOpeneingAllowed();

        auto root_id = getRootID(ctr_id);
        if (root_id.is_set())
        {
            auto block = this->getBlock(root_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            auto ii = instance_map_.find(ctr_id);
            if (ii != instance_map_.end())
            {
                auto ctr_hash = block->ctr_type_hash();
                auto instance_hash = ii->second->type_hash();

                if (instance_hash == ctr_hash) {
                    return ii->second->shared_self();
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR(
                                "Exisitng ctr instance type hash mismatch: expected {}, actual {}",
                                ctr_hash,
                                instance_hash
                    ).do_throw();
                }
            }
            else {
                return ctr_intf->new_ctr_instance(block, this->shared_from_this());
            }
        }
        else {
            return CtrSharedPtr<CtrReferenceable<ApiProfileT>>{};
        }
    }

    void pack_store()
    {
        this->history_tree_raw_->pack();
    }

    virtual U8String ctr_type_name(const CtrID& name)
    {

        auto root_id = getRootID(name);
        if (root_id.is_set())
        {
            auto block = this->getBlock(root_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ctr_intf->ctr_type_name();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Can't find container with name {}", name).do_throw();
        }
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> from_root_id(const BlockID& root_block_id, const CtrID& name)
    {
        if (root_block_id.is_set())
        {
            auto block = this->getBlock(root_block_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ctr_intf->new_ctr_instance(block, this->shared_from_this());
        }
        else {
            return CtrSharedPtr<CtrReferenceable<ApiProfileT>>{};
        }
    }

    CtrBlockDescription<ApiProfileT> describe_block(const CtrID& block_id)
    {
        auto block = this->getBlock(block_id);
        return describe_block(block);
    }

    CtrBlockDescription<ApiProfileT> describe_block(const SharedBlockPtr& block)
    {
        auto ctr_intf = ProfileMetadata<Profile>::local()
                ->get_container_operations(block->ctr_type_hash());

        return ctr_intf->describe_block1(block->id(), this->shared_from_this());
    }

protected:

    SharedPtr<SnapshotMemoryStat<ApiProfileT>> do_compute_memory_stat()
    {
        _::BlockSet visited_blocks;

        PersistentTreeStatVisitAccumulatingConsumer vp_accum(visited_blocks);

        HistoryNode* node = this->history_node_->parent();

        while (node)
        {
            PersistentTreeT persistent_tree(node);
            persistent_tree.conditional_tree_traverse(vp_accum);

            node = node->parent();
        }

        SnapshotStatsCountingConsumer<SnapshotBase> consumer(visited_blocks, this);

        persistent_tree_.conditional_tree_traverse(consumer);

        return consumer.finish();
    }

    SharedPtr<SnapshotMemoryStat<ApiProfileT>> do_compute_memory_stat(_::BlockSet& visited_blocks)
    {
        SnapshotStatsCountingConsumer<SnapshotBase> consumer(visited_blocks, this);

        persistent_tree_.conditional_tree_traverse(consumer);

        return consumer.finish();
    }

    // FIXME: use noexcept
    auto export_block_rchandle(const CtrID& id)
    {
    	auto opt = persistent_tree_.find(id);

    	if (opt)
    	{
            return opt.value().block_ptr();
    	}
    	else {
            MMA_THROW(Exception()) << WhatInfo(format_u8("Block with id {} does not exist in snapshot {}", id, currentTxnId()));
    	}
    }




    void clone_foreign_block(const BlockType* foreign_block)
    {
        auto new_block = clone_block(foreign_block);
        ptree_set_new_block(new_block);
    }


    BlockType* clone_block(const BlockType* block)
    {
        char* buffer = (char*) this->malloc(block->memory_block_size());

        CopyByteBuffer(block, buffer, block->memory_block_size());
        BlockType* new_block = ptr_cast<BlockType>(buffer);

        auto new_block_id = newId();
        new_block->uuid() = new_block_id;

        return new_block;
    }

    void put_into_cache(Shared* shared)
    {
        auto evicted_entry = block_shared_cache_.insert(shared);

        if (evicted_entry) {
            if (evicted_entry->state() == Shared::DELETE_)
            {

            }
        }
    }

    Shared* get_shared(BlockType* block)
    {
        MEMORIA_V1_ASSERT_TRUE(block != nullptr);

        auto shared_opt = block_shared_cache_.get(block->id());

        if (!shared_opt)
        {
            BlockCacheEntry* shared  = block_shared_cache_pool_.construct(block->id());
            shared->state() = Shared::UNDEFINED;
            shared->set_block(block);
            shared->set_allocator(this);

            block_shared_cache_.insert(shared);

            return shared;
        }

        return shared_opt.get();
    }

    Shared* get_shared(const BlockID& id, int32_t state)
    {
        auto shared_opt = block_shared_cache_.get(id);

        if (!shared_opt)
        {
            BlockCacheEntry* shared  = block_shared_cache_pool_.construct(id, (BlockType*)nullptr, state);
            shared->set_allocator(this);
            block_shared_cache_.insert(shared);

            return shared;
        }

        return shared_opt.get();
    }

    void* malloc(size_t size) noexcept {
        return ::malloc(size);
    }

    void ptree_set_new_block(BlockType* block)
    {
        const auto& txn_id = history_node_->snapshot_id();
        using Value = typename PersistentTreeT::Value;
        auto ptr = new RCBlockPtr(block, 1);
        auto old_value = persistent_tree_.assign(block->id(), Value(ptr, txn_id));

        if (old_value.block_ptr())
        {
            if (old_value.block_ptr()->unref() == 0) {
                delete old_value.block_ptr();
            }
        }
    }


    void checkIfConainersOpeneingAllowed()
    {
        checkReadAllowed();

    	if (is_data_locked())
    	{
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot's {} data is locked", uuid()).do_throw();
    	}
    }

    void checkIfConainersCreationAllowed()
    {
    	if (!is_active())
    	{
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot's {} data is not active, snapshot status = {}", uuid(), (int32_t)history_node_->status()).do_throw();
    	}
    }


    void checkIfExportAllowed()
    {
    	if (history_node_->is_dropped())
    	{
            // Double checking. This shouldn't happen
            if (!history_node_->root())
            {
                MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been cleared", uuid()).do_throw();
            }
    	}
    	else if (history_node_->is_active()) {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is still active", uuid()).do_throw();
    	}
    }




    void checkReadAllowed()
    {
    }

    void checkUpdateAllowed()
    {
        checkReadAllowed();

        if (!history_node_->is_active())
        {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been already committed or data is locked", uuid()).do_throw();
        }
    }

    void checkUpdateAllowed(const CtrID& ctrName)
    {
        checkReadAllowed();

        if ((!history_node_->is_active())) // && ctrName.is_set()
    	{
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been already committed or data is locked", uuid()).do_throw();
    	}
    }

    void checkIfDataLocked()
    {
        checkReadAllowed();

    	if (!history_node_->is_data_locked())
    	{
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} hasn't been locked", uuid()).do_throw();
    	}
    }


    void do_drop() noexcept
    {
        if (history_tree_raw_->is_dump_snapshot_lifecycle()) {
            std::cout << "MEMORIA: DROP snapshot's DATA: " << history_node_->snapshot_id() << std::endl;
        }

    	persistent_tree_.delete_tree([&](LeafNodeT* leaf){
            for (int32_t c = 0; c < leaf->size(); c++)
            {
                auto& block_descr = leaf->data(c);
                if (block_descr.block_ptr()->unref() == 0)
                {
                    auto shared = block_shared_cache_.get(block_descr.block_ptr()->raw_data()->id());

                    if (shared)
                    {
                        block_descr.block_ptr()->clear();
                        shared.get()->state() = Shared::_DELETE;
                    }

                    delete block_descr.block_ptr();
                }
            }
        });
    }

    static void delete_snapshot(HistoryNode* node) noexcept
    {
        PersistentTreeT persistent_tree(node);

        persistent_tree.delete_tree([&](LeafNodeT* leaf){
            for (int32_t c = 0; c < leaf->size(); c++)
            {
                auto& block_descr = leaf->data(c);
                if (block_descr.block_ptr()->unref() == 0)
                {
                    delete block_descr.block_ptr();
                }
            }
        });

        node->assign_root_no_ref(nullptr);
        node->root_id() = BlockID{};
    }



    void check_tree_structure(const NodeBaseT* node)
    {
//      if (node->snapshot_id() == history_node_->snapshot_id())
//      {
//          if (node->refs() != 1)
//          {
//              cerr << "NodeRefProblem1 for: " << endl;
//              node->dump(cerr);
//          }
//      }
//      else {
//          if (node->refs() < 1)
//          {
//              cerr << "NodeRefProblem2 for: " << endl;
//              node->dump(cerr);
//          }
//      }
//
//      if (node->is_leaf())
//      {
//          //auto leaf_node = PersitentTree::to_leaf_node(node);
//      }
//      else {
//          auto branch_node = PersitentTree::to_branch_node(node);
//
//          for (int32_t c = 0; c < branch_node->size(); c++)
//          {
//              check_tree_structure(branch_node->data(c));
//          }
//      }
    }
};

}}}
