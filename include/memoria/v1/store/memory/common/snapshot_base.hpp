
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

#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/store/memory/common/store_stat.hpp>


#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/ctr_impl.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/type_name.hpp>

#include <memoria/v1/api/datatypes/type_registry.hpp>

#include "persistent_tree.hpp"

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif



#include <vector>
#include <memory>
#include <mutex>



namespace memoria {
namespace v1 {
namespace store {
namespace memory {

template <typename Profile, typename PersistentAllocator, typename SnapshotType>
class SnapshotBase:
        public ProfileAllocatorType<Profile>,
        public IMemorySnapshot<Profile>,
        public SnpSharedFromThis<SnapshotType>
{    
protected:
	using MyType			= SnapshotType;
    using Base              = ProfileAllocatorType<Profile>;

public:
    using ProfileT = Profile;

    using typename Base::CtrID;
    using typename Base::BlockID;
    using typename Base::BlockType;
    using typename Base::SnapshotID;
protected:

	using HistoryNode		= typename PersistentAllocator::HistoryNode;
    using PersistentTreeT   = typename PersistentAllocator::PersistentTreeT;
    


    using PersistentAllocatorPtr = AllocSharedPtr<PersistentAllocator>;
    using SnapshotPtr            = SnpSharedPtr<MyType>;
    using SnapshotApiPtr         = SnpSharedPtr<IMemorySnapshot<Profile>>;
    using AllocatorPtr           = AllocSharedPtr<Base>;
    
    using NodeBaseT         = typename PersistentTreeT::NodeBaseT;
    using LeafNodeT         = typename PersistentTreeT::LeafNodeT;
    using PTreeValue        = typename LeafNodeT::Value;
    
    using RCBlockPtr			= typename std::remove_pointer<typename PersistentTreeT::Value::Value>::type;

    using Status            = typename HistoryNode::Status;



    class CtrDescr {
    	int64_t references_;
    public:
    	CtrDescr(): references_() {}
    	CtrDescr(int64_t val): references_(val) {}

    	int64_t references() const {return references_;}
    	void ref() 	 {++references_;}
    	int64_t unref() {return --references_;}
    };

    using CtrInstanceMap 	= std::unordered_map<std::type_index, CtrDescr>;

public:

    template <typename CtrName>
    using CtrT = SharedCtr<CtrName, ProfileAllocatorType<Profile>, Profile>;

    template <typename CtrName>
    using CtrPtr = CtrSharedPtr<CtrT<CtrName>>;

    using typename Base::BlockG;
    using typename Base::Shared;

    using RootMapType = CtrT<Map<CtrID, BlockID>>;

protected:

    HistoryNode*            history_node_;
    PersistentAllocatorPtr  history_tree_;
    PersistentAllocator*    history_tree_raw_ = nullptr;

    PersistentTreeT persistent_tree_;

    StaticPool<BlockID, Shared, 256> pool_;

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
public:

    SnapshotBase(HistoryNode* history_node, const PersistentAllocatorPtr& history_tree):
        history_node_(history_node),
        history_tree_(history_tree),
        history_tree_raw_(history_tree.get()),
        persistent_tree_(history_node_),
        logger_("PersistentInMemStoreSnp", Logger::DERIVED, &history_tree->logger_)
    {
        history_node_->ref();

        if (history_node->is_active())
        {
            history_tree_raw_->ref_active();
        }
    }

    SnapshotBase(HistoryNode* history_node, PersistentAllocator* history_tree):
        history_node_(history_node),
        history_tree_raw_(history_tree),
        persistent_tree_(history_node_),
        logger_("PersistentInMemStoreSnp")
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

        if (root_id.isSet())
        {
            BlockG root_block = findBlock(root_id);
            root_map_ = ctr_make_shared<RootMapType>(ptr, root_block);
        }
        else {
            root_map_ = ctr_make_shared<RootMapType>(ptr, CtrID{}, Map<CtrID, BlockID>());
        }

        root_map_->reset_allocator_holder();
    }
    
    static void init_profile_metadata() {
        RootMapType::init_profile_metadata();
    }


    virtual ~SnapshotBase() noexcept
    {
    }
    
    virtual SnpSharedPtr<ProfileAllocatorType<Profile>> self_ptr() {
        return this->shared_from_this();
    }
    
    PairPtr& pair() {
        return pair_;
    }

    const PairPtr& pair() const {
        return pair_;
    }

    const CtrID& uuid() const {
        return history_node_->snapshot_id();
    }

    bool is_active() const {
        return history_node_->is_active();
    }

    bool is_data_locked() const {
    	return history_node_->is_data_locked();
    }

    virtual bool isActive() const {
        return is_active();
    }

    bool is_marked_to_clear() const {
        return history_node_->is_dropped();
    }

    bool is_committed() const {
        return history_node_->is_committed();
    }

    std::vector<CtrID> container_names() const
    {
        std::vector<CtrID> names;

        auto ii = root_map_->begin();
        while (!ii->isEnd()) {
            names.push_back(ii->key());
            ii->next();
        }

        return names;
    }

    std::vector<U16String> container_names_str() const
    {
        std::vector<U16String> names;

        auto ii = root_map_->begin();
        while (!ii->isEnd()) {
            names.push_back(ii->key().to_u16());
            ii->next();
        }

        return names;
    }


    bool drop_ctr(const CtrID& name)
    {
        checkUpdateAllowed();

        CtrID root_id = getRootID(name);

        if (root_id.is_set())
        {
            BlockG block = this->getBlock(root_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(block->ctr_type_hash());
            ctr_intf->drop(name, this->shared_from_this());

            return true;
        }
        else {
            return false;
        }
    }


    Optional<U16String> ctr_type_name_for(const CtrID& name)
    {
        auto root_id = this->getRootID(name);
        auto block 	 = this->getBlock(root_id);

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            return ctr_intf->ctr_type_name();
        }
        else {
            return Optional<U16String>();
        }
    }

    void set_as_master()
    {
    	history_tree_raw_->set_master(uuid());
    }

    void set_as_branch(U16StringRef name)
    {
    	history_tree_raw_->set_branch(name, uuid());
    }

    U16StringRef metadata() const
    {
        return history_node_->metadata();
    }

    void set_metadata(U16StringRef metadata)
    {
        if (history_node_->is_active())
        {
            history_node_->set_metadata(metadata);
        }
        else
        {
            MMA1_THROW(Exception()) << WhatCInfo("Snapshot is already committed.");
        }
    }

    void for_each_ctr_node(const CtrID& name, typename ContainerOperations<Profile>::BlockCallbackFn fn)
    {
    	auto root_id = this->getRootID(name);
        auto block 	 = this->getBlock(root_id);

        if (block)
    	{
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            ctr_intf->for_each_ctr_node(name, this->shared_from_this(), fn);
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Container with name {} does not exist in snapshot {}", name, history_node_->snapshot_id()));
    	}
    }



    void import_new_ctr_from(SnapshotApiPtr ptr, const CtrID& name)
    {
        checkUpdateAllowed();

        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

    	txn->checkIfExportAllowed();

        BlockID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&](const BlockID& uuid, const BlockID& id, const void* block_data){
                auto rc_handle = txn->export_block_rchandle(id);
    			using Value = typename PersistentTreeT::Value;

    			rc_handle->ref();

    			auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

                if (old_value.block_ptr())
    			{
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Block with ID {} is not new in snapshot {}", id, txn_id));
    			}
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    		}
    		else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId()));
    		}
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Container with name {} already exists in snapshot {}", name, txn_id));
    	}
    }


    void copy_new_ctr_from(SnapshotApiPtr ptr, const CtrID& name)
    {
        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

    	txn->checkReadAllowed();
    	checkUpdateAllowed();

        BlockID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&](const BlockID& uuid, const BlockID& id, const void* block_data){
                clone_foreign_block(T2T<const BlockType*>(block_data));
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    		}
    		else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId()));
    		}
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Container with name {} already exists in snapshot {}", name, txn_id));
    	}
    }


    void import_ctr_from(SnapshotApiPtr ptr, const CtrID& name)
    {
        checkUpdateAllowed();

        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

        txn->checkIfExportAllowed();

        BlockID root_id = this->getRootID(name);

    	auto txn_id = uuid();

    	if (!root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&, this](const BlockID& uuid, const BlockID& id, const void* block_data) {
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
                        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected refcount == 0 for block {}", old_value.block_ptr()->raw_data()->uuid()));
    				}
    			}
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    		}
    		else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId()));
    		}
    	}
    	else {
    		import_new_ctr_from(txn, name);
    	}
    }


    void copy_ctr_from(SnapshotApiPtr ptr, const CtrID& name)
    {
        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

    	txn->checkReadAllowed();
    	checkUpdateAllowed();

        BlockID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (!root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&, this](const BlockID& uuid, const BlockID& id, const void* block_data) {
                auto block = this->getBlock(id);
                if (block && block->uuid() == uuid)
    			{
    				return;
    			}

                clone_foreign_block(T2T<const BlockType*>(block_data));
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    		}
    		else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected empty root ID for container {} in snapshot {}", name, txn_id));
    		}
    	}
    	else {
    		copy_new_ctr_from(txn, name);
    	}
    }


    CtrID clone_ctr(const CtrID& ctr_name) {
        return clone_ctr(ctr_name, CtrID{});
    }

    CtrID clone_ctr(const CtrID& ctr_name, const CtrID& new_ctr_name)
    {
        auto root_id = this->getRootID(ctr_name);
        auto block 	 = this->getBlock(root_id);

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->clone_ctr(ctr_name, new_ctr_name, this->shared_from_this());
        }
        else {
            MMA1_THROW(Exception()) << fmt::format_ex(u"Container with name {} does not exist in snapshot {} ", ctr_name, history_node_->snapshot_id());
        }
    }

    BlockG findBlock(const BlockID& id)
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
                return BlockG();
            }
        }

        return BlockG(shared);
    }

    virtual BlockG getBlock(const BlockID& id)
    {
        if (id.isSet())
        {
            BlockG block = findBlock(id);

            if (block) {
                return block;
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Block is not found for the specified id: {}", id));
            }
        }
        else {
            return BlockG();
        }
    }

    void dumpAccess(const char* msg, const BlockID& id, const Shared* shared)
    {
        std::cout << msg << ": " << id << " " << shared->get() << " " << shared->get()->uuid() << " " << shared->state() << std::endl;
    }


    virtual void registerCtr(const std::type_info& ti)
    {
    	auto ii = instance_map_.find(ti);
    	if (ii == instance_map_.end())
    	{
    		instance_map_.insert({ti, CtrDescr(1)});
    	}
    	else {
    		ii->second.ref();
    	}
    }

    virtual void unregisterCtr(const std::type_info& ti)
    {
    	auto ii = instance_map_.find(ti);
    	if (ii == instance_map_.end())
    	{
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Container {} is not registered in snapshot {}", ti.name(), uuid()));
    	}
    	else if (ii->second.unref() == 0) {
    		instance_map_.erase(ii);
    	}
    }

public:
    bool has_open_containers() {
    	return instance_map_.size() > 1;
    }

    void dump_open_containers()
    {
    	for (const auto& pair: instance_map_)
    	{
            std::cout << demangle(pair.first.name()) << " -- " << pair.second.references() << std::endl;
    	}
    }

    void dump_dictionary_blocks()
    {
        auto ii = root_map_->begin();
        if (!ii->isEnd())
        {
            do {
                ii->dump();
            }
            while (ii->nextLeaf());
        }
    }


    virtual BlockG getBlockForUpdate(const BlockID& id)
    {
        // FIXME: Though this check prohibits new block acquiring for update,
        // already acquired updatable blocks can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed(CtrID{});

        if (id.isSet())
        {
            Shared* shared = get_shared(id, Shared::UPDATE);

            if (!shared->get())
            {
                auto block_opt = persistent_tree_.find(id);

                if (block_opt)
                {
                    const auto& txn_id = history_node_->snapshot_id();

                    if (block_opt.value().snapshot_id() != txn_id)
                    {
                        BlockType* new_block = clone_block(block_opt.value().block_ptr()->raw_data());

                        ptree_set_new_block(new_block);

                        shared->set_block(new_block);

                        shared->refresh();
                    }
                    else {
                        MEMORIA_V1_ASSERT(shared->state(), ==, Shared::UPDATE);

                        shared->set_block(block_opt.value().block_ptr()->raw_data());

                        shared->refresh();
                    }
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Block is not found for the specified id: {}", id));
                }
            }
            else if (shared->state() == Shared::READ)
            {
                auto block_opt = persistent_tree_.find(id);

                if (block_opt)
                {
                    BlockType* new_block = clone_block(block_opt.value().block_ptr()->raw_data());

                    ptree_set_new_block(new_block);
                    shared->set_block(new_block);

                    shared->refresh();
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Block is not found for the specified id: {}", id));
                }
            }
            else if (shared->state() == Shared::UPDATE)
            {
                //MEMORIA_ASEERT();
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid BlockShared state: {}", shared->state()));
            }

            shared->state() = Shared::UPDATE;

            return BlockG(shared);
        }
        else {
            return BlockG();
        }
    }



    virtual BlockG updateBlock(Shared* shared)
    {
        // FIXME: Though this check prohibits new block acquiring for update,
        // already acquired updatable blocks can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed(CtrID{});

        if (shared->state() == Shared::READ)
        {
            BlockType* new_block = clone_block(shared->get());

            ptree_set_new_block(new_block);

            shared->set_block(new_block);

            shared->state() = Shared::UPDATE;

            shared->refresh();
        }

        return BlockG(shared);
    }

    virtual void removeBlock(const BlockID& id)
    {
        checkUpdateAllowed(CtrID{});

        auto iter = persistent_tree_.locate(id);

        if (!iter.is_end())
        {
            auto shared = pool_.get(id);

            if (!shared)
            {
                persistent_tree_.remove(iter);
            }
            else {
                shared->state() = Shared::_DELETE;
            }
        }
    }






    virtual BlockG createBlock(int32_t initial_size)
    {
        checkUpdateAllowed(CtrID{});

        if (initial_size == -1)
        {
            initial_size = DEFAULT_BLOCK_SIZE;
        }

        void* buf = allocate_system<uint8_t>(initial_size).release();

        memset(buf, 0, initial_size);

        BlockID id = newId();

        BlockType* p = new (buf) BlockType(id);

        p->memory_block_size() = initial_size;

        Shared* shared  = pool_.allocate(id);

        shared->id()    = id;
        shared->state() = Shared::UPDATE;

        shared->set_block(p);
        shared->set_allocator(this);

        ptree_set_new_block(p);

        return BlockG(shared);
    }


    virtual BlockG cloneBlock(const Shared* shared, const BlockID& new_id)
    {
        checkUpdateAllowed(CtrID{});

        BlockID new_id_v = new_id.is_set() ? new_id : newId();
        BlockType* new_block = this->clone_block(shared->get());

        new_block->id() = new_id_v;

        Shared* new_shared  = pool_.allocate(new_id_v);

        new_shared->id()    = new_id_v;
        new_shared->state() = Shared::UPDATE;

        new_shared->set_block(new_block);
        new_shared->set_allocator(this);

        ptree_set_new_block(new_block);

        return BlockG(new_shared);
    }


    virtual void resizeBlock(Shared* shared, int32_t new_size)
    {
        checkUpdateAllowed();

        if (shared->state() == Shared::READ)
        {
            BlockType* block = shared->get();
            BlockType* new_block = allocate_system<BlockType>(new_size).release();

            ProfileMetadata<Profile>::local()
                    ->get_block_operations(block->ctr_type_hash(), block->block_type_hash())
                    ->resize(block, new_block, new_size);

            shared->set_block(new_block);

            ptree_set_new_block(new_block);
        }
        else if (shared->state() == Shared::UPDATE)
        {
            BlockType* block = shared->get();
            BlockType* new_block = reallocate_system<BlockType>(block, new_size).release();

            ProfileMetadata<Profile>::local()
                    ->get_block_operations(block->ctr_type_hash(), block->block_type_hash())
                    ->resize(block, new_block, new_size);

            shared->set_block(new_block);

            ptree_set_new_block(new_block);
        }
    }

    virtual void releaseBlock(Shared* shared) noexcept
    {
        if (shared->state() == Shared::_DELETE)
        {
            persistent_tree_.remove(shared->get()->id());
        }

        pool_.release(shared->id());
    }

    virtual BlockG getBlockG(BlockType* block)
    {
        MMA1_THROW(Exception()) << WhatCInfo("Method getBlockG is not implemented for this allocator");
    }


    virtual BlockID newId() {
        return history_tree_raw_->newId();
    }

    virtual CtrID currentTxnId() const {
        return history_node_->snapshot_id();
    }

    // memory pool allocator
    virtual void* allocateMemory(size_t size) {
        return allocate_system<uint8_t>(size).release();
    }
    virtual void  freeMemory(void* ptr) {
        free_system(ptr);
    }

    virtual Logger& logger() {return logger_;}

    virtual BlockID getRootID(const CtrID& name)
    {
        if (!name.is_null())
        {
            auto iter = root_map_->find(name);

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
                MMA1_THROW(Exception()) << WhatCInfo("Allocator directory removal attempted");
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
        if (MMA1_UNLIKELY(!root_map_)) {
            return false;
        }

        if (!name.is_null())
        {
            auto iter = root_map_->find(name);
            return iter->is_found(name);
        }
        else {
            return !history_node_->root_id().is_null();
        }
    }

    virtual CtrID createCtrName()
    {
        return IDTools<CtrID>::make_random();
    }


    virtual bool check()
    {
        bool result = false;

        for (auto iter = root_map_->begin(); !iter->is_end(); )
        {
            auto ctr_name = iter->key();

            BlockG block = this->getBlock(iter->value());

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            result = ctr_intf->check(ctr_name, this->shared_from_this()) || result;

            iter->next();
        }

        return result;
    }

    U16String get_branch_suffix() const
    {
        return u"";
    }

    virtual void walk_containers(ContainerWalker<ProfileT>* walker, const char16_t* allocator_descr = nullptr) {
        walkContainers(walker, allocator_descr);
    }

    virtual void walkContainers(ContainerWalker<ProfileT>* walker, const char16_t* allocator_descr = nullptr)
    {
		if (allocator_descr != nullptr)
		{
            walker->beginSnapshot(fmt::format(u"Snapshot-{} -- {}", history_node_->snapshot_id(), allocator_descr).data());
		}
		else {
            walker->beginSnapshot(fmt::format(u"Snapshot-{}", history_node_->snapshot_id()).data());
		}

        auto iter = root_map_->Begin();

        while (!iter->isEnd())
        {
            auto ctr_name   = iter->key();
            auto root_id    = iter->value();

            auto block      = this->getBlock(root_id);

            auto ctr_hash   = block->ctr_type_hash();
            auto ctr_intf   = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            ctr_intf->walk(ctr_name, this->shared_from_this(), walker);

            iter->next();
        }

        walker->endSnapshot();
    }



    void dump_persistent_tree() {
        persistent_tree_.dump_tree();
    }


    virtual CtrSharedPtr<CtrReferenceable<Profile>> create(const DataTypeDeclaration& decl, const CtrID& ctr_id)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_typedecl_string());
        return factory->create_instance(this->shared_from_this(), ctr_id, decl);
    }

    virtual CtrSharedPtr<CtrReferenceable<Profile>> create(const DataTypeDeclaration& decl)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_typedecl_string());
        return factory->create_instance(this->shared_from_this(), this->createCtrName(), decl);
    }

    virtual CtrSharedPtr<CtrReferenceable<Profile>> find(const CtrID& ctr_id)
    {
        checkIfConainersOpeneingAllowed();

        CtrID root_id = getRootID(ctr_id);

        if (root_id.is_set())
        {
            BlockG block = this->getBlock(root_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ctr_intf->new_ctr_instance(block, this->shared_from_this());
        }
        else {
            return CtrSharedPtr<CtrReferenceable<Profile>>();
        }
    }


    void pack_store()
    {
        this->history_tree_raw_->pack();
    }

    virtual U16String ctr_type_name(const CtrID& name)
    {
        CtrID root_id = getRootID(name);

        if (root_id.is_set())
        {
            BlockG block = this->getBlock(root_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ctr_intf->ctr_type_name();
        }
        else {
            MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't find container with name {}", name);
        }
    }

    virtual CtrSharedPtr<CtrReferenceable<Profile>> from_root_id(const CtrID& root_block_id, const CtrID& name)
    {
        if (root_block_id.is_set())
        {
            BlockG block = this->getBlock(root_block_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ctr_intf->new_ctr_instance(block, this->shared_from_this());
        }
        else {
            return CtrSharedPtr<CtrReferenceable<Profile>>();
        }
    }

    CtrBlockDescription<Profile> describe_block(const CtrID& block_id)
    {
        BlockG block = this->getBlock(block_id);
        return describe_block(block);
    }

    CtrBlockDescription<Profile> describe_block(const BlockG& block)
    {
        auto ctr_intf = ProfileMetadata<Profile>::local()
                ->get_container_operations(block->ctr_type_hash());

        return ctr_intf->describe_block1(block->id(), this->shared_from_this());
    }

protected:

    SharedPtr<SnapshotMemoryStat> do_compute_memory_stat(bool include_containers)
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

        SnapshotStatsCountingConsumer<SnapshotBase> consumer(visited_blocks, this, include_containers);

        persistent_tree_.conditional_tree_traverse(consumer);

        return consumer.finish();
    }

    SharedPtr<SnapshotMemoryStat> do_compute_memory_stat(_::BlockSet& visited_blocks, bool include_containers)
    {
        SnapshotStatsCountingConsumer<SnapshotBase> consumer(visited_blocks, this, include_containers);

        persistent_tree_.conditional_tree_traverse(consumer);

        return consumer.finish();
    }

    auto export_block_rchandle(const CtrID& id)
    {
    	auto opt = persistent_tree_.find(id);

    	if (opt)
    	{
            return opt.value().block_ptr();
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Block with id {} does not exist in snapshot {}", id, currentTxnId()));
    	}
    }




    void clone_foreign_block(const BlockType* foreign_block)
    {
        BlockType* new_block = clone_block(foreign_block);
        ptree_set_new_block(new_block);
    }


    BlockType* clone_block(const BlockType* block)
    {
        char* buffer = (char*) this->malloc(block->memory_block_size());

        CopyByteBuffer(block, buffer, block->memory_block_size());
        BlockType* new_block = T2T<BlockType*>(buffer);

        new_block->uuid() = newId();

        return new_block;
    }

    Shared* get_shared(BlockType* block)
    {
        MEMORIA_V1_ASSERT_TRUE(block != nullptr);

        Shared* shared = pool_.get(block->id());

        if (shared == NULL)
        {
            shared = pool_.allocate(block->id());

            shared->id()        = block->id();
            shared->state()     = Shared::UNDEFINED;
            shared->set_block(block);
            shared->set_allocator(this);
        }

        return shared;
    }

    Shared* get_shared(const BlockID& id, int32_t state)
    {
        Shared* shared = pool_.get(id);

        if (shared == NULL)
        {
            shared = pool_.allocate(id);

            shared->id()        = id;
            shared->state()     = state;
            shared->set_block((BlockType*)nullptr);
            shared->set_allocator(this);
        }

        return shared;
    }

    void* malloc(size_t size)
    {
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot's {} data is locked", uuid()));
    	}
    }

    void checkIfConainersCreationAllowed()
    {
    	if (!is_active())
    	{
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot's {} data is not active, snapshot status = {}", uuid(), (int32_t)history_node_->status()));
    	}
    }


    void checkIfExportAllowed()
    {
    	if (history_node_->is_dropped())
    	{
    		// Double checking. This shouldn't happen
    		if (!history_node_->root())
    		{
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has been cleared", uuid()));
    		}
    	}
    	else if (history_node_->is_active()) {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is still active", uuid()));
    	}
    }




    void checkReadAllowed()
    {
    	// read is always allowed
    }

    void checkUpdateAllowed()
    {
        checkReadAllowed();

        if (!history_node_->is_active())
        {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has been already committed or data is locked", uuid()));
        }
    }

    void checkUpdateAllowed(const CtrID& ctrName)
    {
    	checkReadAllowed();

    	if ((!history_node_->is_active()) && ctrName.is_set())
    	{
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has been already committed or data is locked", uuid()));
    	}
    }

    void checkIfDataLocked()
    {
    	checkReadAllowed();

    	if (!history_node_->is_data_locked())
    	{
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} hasn't been locked", uuid()));
    	}
    }


    void do_drop() throw ()
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
                    auto shared = pool_.get(block_descr.block_ptr()->raw_data()->id());

                    if (shared)
                    {
                        block_descr.block_ptr()->clear();
                        shared->state() = Shared::_DELETE;
                    }

                    delete block_descr.block_ptr();
                }
            }
        });
    }

    static void delete_snapshot(HistoryNode* node) throw ()
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

}}}}
