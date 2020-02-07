
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

#include <memoria/store/memory/common/store_stat.hpp>
#include <memoria/store/memory/common/static_pool.hpp>


#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/ctr_impl.hpp>

#include <memoria/core/exceptions/exceptions.hpp>


#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/containers/map/map_factory.hpp>
#include <memoria/core/tools/pair.hpp>
#include <memoria/core/tools/type_name.hpp>

#include <memoria/core/datatypes/type_registry.hpp>

#include <memoria/core/linked/document/linked_document.hpp>

#include "persistent_tree.hpp"

#include <memoria/core/memory/ptr_cast.hpp>

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif



#include <vector>
#include <memory>
#include <mutex>



namespace memoria {
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

    using CtrInstanceMap = std::unordered_map<CtrID, CtrReferenceable<Profile>*>;

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
    
    virtual SnpSharedPtr<ProfileAllocatorType<Profile>> self_ptr() noexcept {
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

    Result<std::vector<CtrID>> container_names() const noexcept
    {
        std::vector<CtrID> names;

        auto ii = root_map_->ctr_begin();
        MEMORIA_RETURN_IF_ERROR(ii);

        while (!ii.get()->iter_is_end())
        {
            names.push_back(ii.get()->key());
            auto res = ii.get()->next();
            MEMORIA_RETURN_IF_ERROR(res);
        }

        return Result<std::vector<CtrID>>::of(std::move(names));
    }

    Result<std::vector<U8String>> container_names_str() const noexcept
    {
        std::vector<U8String> names;

        auto ii = root_map_->ctr_begin();
        MEMORIA_RETURN_IF_ERROR(ii);

        while (!ii.get()->iter_is_end())
        {
            std::stringstream ss;
            ss << ii.get()->key().view();

            names.push_back(U8String(ss.str()));
            auto res = ii.get()->next();
            MEMORIA_RETURN_IF_ERROR(res);
        }

        return Result<std::vector<U8String>>::of(std::move(names));
    }


    BoolResult drop_ctr(const CtrID& name) noexcept
    {
        checkUpdateAllowed();

        auto root_id = getRootID(name);
        MEMORIA_RETURN_IF_ERROR(root_id);


        if (root_id.get().is_set())
        {
            auto block = this->getBlock(root_id.get());
            MEMORIA_RETURN_IF_ERROR(block);

            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(block.get()->ctr_type_hash());

            auto res = ctr_intf->drop(name, this->shared_from_this());
            MEMORIA_RETURN_IF_ERROR(res);

            return BoolResult::of(true);
        }
        else {
            return BoolResult::of(false);
        }
    }


    Result<Optional<U8String>> ctr_type_name_for(const CtrID& name) noexcept
    {
        using ResultT = Result<Optional<U8String>>;

        auto root_id = this->getRootID(name);
        MEMORIA_RETURN_IF_ERROR(root_id);

        auto block 	 = this->getBlock(root_id.get());
        MEMORIA_RETURN_IF_ERROR(block);

        if (block.get())
        {
            auto ctr_hash = block.get()->ctr_type_hash();

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            return ResultT::of(ctr_intf->ctr_type_name());
        }
        else {
            return ResultT::of();
        }
    }

    VoidResult set_as_master() noexcept
    {
        return history_tree_raw_->set_master(uuid());
    }

    VoidResult set_as_branch(U8StringRef name) noexcept
    {
        return history_tree_raw_->set_branch(name, uuid());
    }

    U8StringRef metadata() const noexcept
    {
        return history_node_->metadata();
    }

    VoidResult set_metadata(U8StringRef metadata) noexcept
    {
        if (history_node_->is_active())
        {
            history_node_->set_metadata(metadata);
        }
        else
        {
            return VoidResult::make_error("Snapshot is already committed.");
        }
    }

    VoidResult for_each_ctr_node(const CtrID& name, typename ContainerOperations<Profile>::BlockCallbackFn fn) noexcept
    {
        auto root_id = this->getRootID(name);
        MEMORIA_RETURN_IF_ERROR(root_id);

        auto block = this->getBlock(root_id.get());
        MEMORIA_RETURN_IF_ERROR(block);

        if (block.get())
    	{
            auto ctr_hash = block.get()->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->for_each_ctr_node(name, this->shared_from_this(), fn);
    	}
    	else {
            return VoidResult::make_error("Container with name {} does not exist in snapshot {}", name, history_node_->snapshot_id());
    	}
    }



    VoidResult import_new_ctr_from(SnapshotApiPtr ptr, const CtrID& name) noexcept
    {
        checkUpdateAllowed();

        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

    	txn->checkIfExportAllowed();

        auto root_id = this->getRootID(name);
        MEMORIA_RETURN_IF_ERROR(root_id);

    	auto txn_id = currentTxnId();

        if (root_id.get().is_null())
    	{
            auto res = txn->for_each_ctr_node(name, [&](const BlockID&, const BlockID& id, const void*) noexcept {
                auto rc_handle = txn->export_block_rchandle(id);
    			using Value = typename PersistentTreeT::Value;

    			rc_handle->ref();

    			auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

                if (old_value.block_ptr())
    			{
                    return VoidResult::make_error("Block with ID {} is not new in snapshot {}", id, txn_id);
    			}

                return VoidResult::of();
    		});

            MEMORIA_RETURN_IF_ERROR(res);

            auto root_id = txn->getRootID(name);
            MEMORIA_RETURN_IF_ERROR(root_id);

            if (root_id.get().is_set())
    		{
                auto res = root_map_->assign(name, root_id.get());
                MEMORIA_RETURN_IF_ERROR(res);
    		}
    		else {
                return VoidResult::make_error("Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId());
    		}
    	}
    	else {
            return VoidResult::make_error("Container with name {} already exists in snapshot {}", name, txn_id);
    	}

        return VoidResult::of();
    }


    VoidResult copy_new_ctr_from(SnapshotApiPtr ptr, const CtrID& name) noexcept
    {
        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

    	txn->checkReadAllowed();
    	checkUpdateAllowed();

        auto root_id_res = this->getRootID(name);
        MEMORIA_RETURN_IF_ERROR(root_id_res);

        BlockID root_id = root_id_res.get();

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
            auto res = txn->for_each_ctr_node(name, [&](const BlockID&, const BlockID&, const void* block_data) noexcept -> VoidResult {
                return clone_foreign_block(ptr_cast<const BlockType>(block_data));
    		});
            MEMORIA_RETURN_IF_ERROR(res);

            auto root_id1 = txn->getRootID(name);
            MEMORIA_RETURN_IF_ERROR(root_id1);

            if (root_id1.get().is_set()) {
                auto assign_res = root_map_->assign(name, root_id1.get());
                MEMORIA_RETURN_IF_ERROR(assign_res);
    		}
    		else {
                return VoidResult::make_error("Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId());
    		}
    	}
    	else {
            return VoidResult::make_error("Container with name {} already exists in snapshot {}", name, txn_id);
    	}

        return VoidResult::of();
    }


    Result<void> import_ctr_from(SnapshotApiPtr ptr, const CtrID& name) noexcept
    {
        checkUpdateAllowed();

        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

        txn->checkIfExportAllowed();

        auto root_id = this->getRootID(name);

    	auto txn_id = uuid();

        if (!root_id.get().is_null())
    	{
            auto res = txn->for_each_ctr_node(name, [&, this](const BlockID& uuid, const BlockID& id, const void*) noexcept -> VoidResult {
                auto block = this->getBlock(id);
                MEMORIA_RETURN_IF_ERROR(block);

                if (block.get() && block.get()->uuid() == uuid)
    			{
                    return VoidResult::of();
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
                        return VoidResult::make_error("Unexpected refcount == 0 for block {}", old_value.block_ptr()->raw_data()->uuid());
    				}
    			}

                return VoidResult::of();
    		});

            MEMORIA_RETURN_IF_ERROR(res);

            auto root_id = txn->getRootID(name);
            MEMORIA_RETURN_IF_ERROR(root_id);

            if (root_id.get().is_set())
    		{
                auto assign_res = root_map_->assign(name, root_id.get());
                MEMORIA_RETURN_IF_ERROR(assign_res);
    		}
    		else {
                return VoidResult::make_error("Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId());
    		}

            return VoidResult::of();
    	}
    	else {
            return import_new_ctr_from(txn, name);
    	}
    }


    VoidResult copy_ctr_from(SnapshotApiPtr ptr, const CtrID& name) noexcept
    {
        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

    	txn->checkReadAllowed();
    	checkUpdateAllowed();

        auto root_id = this->getRootID(name);
        MEMORIA_RETURN_IF_ERROR(root_id);

    	auto txn_id = currentTxnId();

        if (!root_id.get().is_null())
    	{
            auto res = txn->for_each_ctr_node(name, [&, this](const BlockID& uuid, const BlockID& id, const void* block_data) noexcept -> VoidResult {
                auto block = this->getBlock(id);
                MEMORIA_RETURN_IF_ERROR(block);

                if (block.get() && block.get()->uuid() == uuid)
    			{
                    return VoidResult::of();
    			}

                return clone_foreign_block(ptr_cast<const BlockType>(block_data));
    		});

            MEMORIA_RETURN_IF_ERROR(res);

            auto root_id1 = txn->getRootID(name);
            MEMORIA_RETURN_IF_ERROR(root_id1);

            if (root_id1.get().is_set())
    		{
                auto assign_res = root_map_->assign(name, root_id1.get());
                MEMORIA_RETURN_IF_ERROR(assign_res);
    		}
    		else {
                return VoidResult::make_error("Unexpected empty root ID for container {} in snapshot {}", name, txn_id);
    		}

            return VoidResult::of();
    	}
    	else {
            return copy_new_ctr_from(txn, name);
    	}
    }


    Result<CtrID> clone_ctr(const CtrID& ctr_name) noexcept {
        return clone_ctr(ctr_name, CtrID{});
    }

    Result<CtrID> clone_ctr(const CtrID& ctr_name, const CtrID& new_ctr_name) noexcept
    {
        auto root_id = this->getRootID(ctr_name);
        MEMORIA_RETURN_IF_ERROR(root_id);

        auto block = this->getBlock(root_id.get());
        MEMORIA_RETURN_IF_ERROR(block);

        if (block.get())
        {
            auto ctr_hash = block.get()->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->clone_ctr(ctr_name, new_ctr_name, this->shared_from_this());
        }
        else {
            return Result<CtrID>::make_error("Container with name {} does not exist in snapshot {} ", ctr_name, history_node_->snapshot_id());
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

    virtual Result<BlockG> getBlock(const BlockID& id) noexcept
    {
        if (id.isSet())
        {
            BlockG block = findBlock(id);

            if (block) {
                return Result<BlockG>::of(std::move(block));
            }
            else {
                return Result<BlockG>::make_error("Block is not found for the specified id: {}", id);
            }
        }
        else {
            return Result<BlockG>::of(BlockG());
        }
    }

    void dumpAccess(const char* msg, const BlockID& id, const Shared* shared)
    {
        std::cout << msg << ": " << id << " " << shared->get() << " " << shared->get()->uuid() << " " << shared->state() << std::endl;
    }


    virtual Result<void> registerCtr(const CtrID& ctr_id, CtrReferenceable<Profile>* instance) noexcept
    {
        auto ii = instance_map_.find(ctr_id);
    	if (ii == instance_map_.end())
    	{
            instance_map_.insert({ctr_id, instance});
    	}
    	else {
            return Result<void>::make_error("Container with name {} has been already registered", ctr_id);
    	}

        return Result<void>::of();
    }

    virtual Result<void> unregisterCtr(const CtrID& ctr_id, CtrReferenceable<Profile>*) noexcept
    {
        instance_map_.erase(ctr_id);
        return Result<void>::of();
    }

public:
    Result<bool> has_open_containers() noexcept {
        return Result<bool>::of(instance_map_.size() > 1);
    }

    Result<void> dump_open_containers() noexcept
    {
    	for (const auto& pair: instance_map_)
    	{
            std::cout << pair.first << " -- " << pair.second->describe_type() << std::endl;
    	}

        return Result<void>::of();
    }

    VoidResult flush_open_containers() noexcept
    {
        for (const auto& pair: instance_map_)
        {
            MEMORIA_RETURN_IF_ERROR_FN(pair.second->flush());
        }

        return VoidResult::of();
    }

    VoidResult dump_dictionary_blocks() noexcept
    {
        auto ii = root_map_->ctr_begin();
        MEMORIA_RETURN_IF_ERROR(ii);

        if (!ii.get()->iter_is_end())
        {
            do {
                ii.get()->dump();

                auto res = ii.get()->iter_next_leaf();
                MEMORIA_RETURN_IF_ERROR(res);

                if (!res.get()) break;
            }
            while (true);
        }

        return VoidResult::of();
    }


    virtual Result<BlockG> getBlockForUpdate(const BlockID& id) noexcept
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
                        auto new_block = clone_block(block_opt.value().block_ptr()->raw_data());
                        MEMORIA_RETURN_IF_ERROR(new_block);

                        ptree_set_new_block(new_block.get());

                        shared->set_block(new_block.get());

                        shared->refresh();
                    }
                    else {
                        //MEMORIA_V1_ASSERT(shared->state(), ==, Shared::UPDATE);

                        shared->set_block(block_opt.value().block_ptr()->raw_data());

                        shared->refresh();
                    }
                }
                else {
                    return Result<BlockG>::make_error("Block is not found for the specified id: {}", id);
                }
            }
            else if (shared->state() == Shared::READ)
            {
                auto block_opt = persistent_tree_.find(id);

                if (block_opt)
                {
                    auto new_block = clone_block(block_opt.value().block_ptr()->raw_data());
                    MEMORIA_RETURN_IF_ERROR(new_block);

                    ptree_set_new_block(new_block.get());
                    shared->set_block(new_block.get());

                    shared->refresh();
                }
                else {
                    return Result<BlockG>::make_error("Block is not found for the specified id: {}", id);
                }
            }
            else if (shared->state() == Shared::UPDATE)
            {
                //MEMORIA_ASEERT();
            }
            else {
                return Result<BlockG>::make_error("Invalid BlockShared state: {}", shared->state());
            }

            shared->state() = Shared::UPDATE;

            return Result<BlockG>::of(shared);
        }
        else {
            return Result<BlockG>::of();
        }
    }



    virtual Result<BlockG> updateBlock(Shared* shared) noexcept
    {
        // FIXME: Though this check prohibits new block acquiring for update,
        // already acquired updatable blocks can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed(CtrID{});

        if (shared->state() == Shared::READ)
        {
            auto new_block = clone_block(shared->get());
            MEMORIA_RETURN_IF_ERROR(new_block);

            ptree_set_new_block(new_block.get());

            shared->set_block(new_block.get());

            shared->state() = Shared::UPDATE;

            shared->refresh();
        }

        return Result<BlockG>::of(shared);
    }

    virtual VoidResult removeBlock(const BlockID& id) noexcept
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

        return VoidResult::of();
    }






    virtual Result<BlockG> createBlock(int32_t initial_size) noexcept
    {
        checkUpdateAllowed(CtrID{});

        if (initial_size == -1)
        {
            initial_size = DEFAULT_BLOCK_SIZE;
        }

        void* buf = allocate_system<uint8_t>(static_cast<size_t>(initial_size)).release();

        memset(buf, 0, static_cast<size_t>(initial_size));

        auto id = newId();
        MEMORIA_RETURN_IF_ERROR(id);

        BlockType* p = new (buf) BlockType(id.get());

        p->memory_block_size() = initial_size;

        Shared* shared  = pool_.allocate(id.get());

        shared->id()    = id.get();
        shared->state() = Shared::UPDATE;

        shared->set_block(p);
        shared->set_allocator(this);

        ptree_set_new_block(p);

        return Result<BlockG>::of(shared);
    }


    virtual Result<BlockG> cloneBlock(const Shared* shared, const BlockID& new_id) noexcept
    {
        checkUpdateAllowed(CtrID{});

        BlockID new_id_v;

        if (new_id.is_set()) {
            new_id_v = new_id;
        }
        else {
            auto id_res = newId();
            MEMORIA_RETURN_IF_ERROR(id_res);
            new_id_v = id_res.get();
        }

        auto new_block = this->clone_block(shared->get());
        MEMORIA_RETURN_IF_ERROR(new_block);

        new_block.get()->id() = new_id_v;

        Shared* new_shared  = pool_.allocate(new_id_v);

        new_shared->id()    = new_id_v;
        new_shared->state() = Shared::UPDATE;

        new_shared->set_block(new_block.get());
        new_shared->set_allocator(this);

        ptree_set_new_block(new_block.get());

        return Result<BlockG>::of(new_shared);
    }


    virtual VoidResult resizeBlock(Shared* shared, int32_t new_size) noexcept
    {
        checkUpdateAllowed();

        BlockType* block = shared->get();

        if (shared->state() == Shared::READ)
        {
            auto buf = allocate_system<uint8_t>(new_size);

            BlockType* new_block = ptr_cast<BlockType>(buf.get());

            int32_t transfer_size = std::min(new_size, block->memory_block_size());

            std::memcpy(new_block, block, transfer_size);

            auto res = ProfileMetadata<Profile>::local()
                    ->get_block_operations(new_block->ctr_type_hash(), new_block->block_type_hash())
                    ->resize(block, new_block, new_size);

            MEMORIA_RETURN_IF_ERROR(res);

            shared->set_block(new_block);
            shared->state() = Shared::UPDATE;
            shared->refresh();

            ptree_set_new_block(new_block);

            buf.release();
        }
        else if (shared->state() == Shared::UPDATE)
        {
            auto buf = allocate_system<uint8_t>(new_size);

            BlockType* new_block = ptr_cast<BlockType>(buf.get());

            int32_t transfer_size = std::min(new_size, block->memory_block_size());

            std::memcpy(new_block, block, transfer_size);

            auto res = ProfileMetadata<Profile>::local()
                    ->get_block_operations(new_block->ctr_type_hash(), new_block->block_type_hash())
                    ->resize(block, new_block, new_size);
            MEMORIA_RETURN_IF_ERROR(res);

            shared->set_block(new_block);

            ptree_set_new_block(new_block);

            buf.release();
        }

        return VoidResult::of();
    }

    virtual VoidResult releaseBlock(Shared* shared) noexcept
    {
        if (shared->state() == Shared::_DELETE)
        {
            persistent_tree_.remove(shared->get()->id());
        }

        pool_.release(shared->id());

        return VoidResult::of();
    }

    virtual Result<BlockG> getBlockG(BlockType*) noexcept
    {
        return Result<BlockG>::make_error("Method getBlockG is not implemented for this allocator");
    }


    virtual Result<BlockID> newId() noexcept {
        return Result<BlockID>::of(history_tree_raw_->newId());
    }

    virtual CtrID currentTxnId() const noexcept {
        return history_node_->snapshot_id();
    }

    // memory pool allocator
    virtual Result<void*> allocateMemory(size_t size) noexcept {
        return wrap_throwing([&]() -> Result<void*> {
            return allocate_system<uint8_t>(size).release();
        });
    }

    virtual void freeMemory(void* ptr) noexcept {
        free_system(ptr);
    }

    virtual Logger& logger() noexcept {return logger_;}

    virtual Result<BlockID> getRootID(const CtrID& name) noexcept
    {
        if (!name.is_null())
        {
            auto iter = root_map_->ctr_map_find(name);
            MEMORIA_RETURN_IF_ERROR(iter);

            if (iter.get()->is_found(name))
            {
                return Result<BlockID>::of(iter.get()->value());
            }
            else {
                return Result<BlockID>::of();
            }
        }
        else {
            return Result<BlockID>::of(history_node_->root_id());
        }
    }

    virtual VoidResult setRoot(const CtrID& name, const BlockID& root) noexcept
    {
        if (root.is_null())
        {
            if (!name.is_null())
            {
                auto res = root_map_->remove(name);
                MEMORIA_RETURN_IF_ERROR(res);
            }
            else {
                return VoidResult::make_error("Allocator directory removal attempted");
            }
        }
        else {
            if (!name.is_null())
            {
                auto res = root_map_->assign(name, root);
                MEMORIA_RETURN_IF_ERROR(res);
            }
            else {
                history_node_->root_id() = root;
            }
        }

        return VoidResult::of();
    }

    virtual BoolResult hasRoot(const CtrID& name) noexcept
    {
        if (MMA_UNLIKELY(!root_map_)) {
            return false;
        }

        if (!name.is_null())
        {
            auto iter = root_map_->ctr_map_find(name);
            MEMORIA_RETURN_IF_ERROR(iter);

            return BoolResult::of(iter.get()->is_found(name));
        }
        else {
            return BoolResult::of(!history_node_->root_id().is_null());
        }
    }

    virtual Result<CtrID> createCtrName() noexcept
    {
        return Result<CtrID>::of(IDTools<CtrID>::make_random());
    }


    virtual BoolResult check() noexcept
    {
        bool result = false;

        auto iter_res = root_map_->ctr_begin();
        MEMORIA_RETURN_IF_ERROR(iter_res);

        for (auto iter = iter_res.get(); !iter->is_end(); )
        {
            auto ctr_name = iter->key();

            auto block = this->getBlock(iter->value());
            MEMORIA_RETURN_IF_ERROR(block);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block.get()->ctr_type_hash());

            auto res = ctr_intf->check(ctr_name, this->shared_from_this());
            MEMORIA_RETURN_IF_ERROR(res);

            result = res.get() || result;

            auto next_res = iter->next();
            MEMORIA_RETURN_IF_ERROR(next_res);
        }

        return BoolResult::of(result);
    }

    U8String get_branch_suffix() const
    {
        return "";
    }

    virtual VoidResult walk_containers(ContainerWalker<ProfileT>* walker, const char* allocator_descr = nullptr) noexcept {
        return walkContainers(walker, allocator_descr);
    }

    virtual VoidResult walkContainers(ContainerWalker<ProfileT>* walker, const char* allocator_descr = nullptr) noexcept
    {
		if (allocator_descr != nullptr)
		{
            walker->beginSnapshot(fmt::format("Snapshot-{} -- {}", history_node_->snapshot_id(), allocator_descr).data());
		}
		else {
            walker->beginSnapshot(fmt::format("Snapshot-{}", history_node_->snapshot_id()).data());
		}

        auto iter = root_map_->ctr_begin();
        MEMORIA_RETURN_IF_ERROR(iter);

        while (!iter.get()->iter_is_end())
        {
            auto ctr_name   = iter.get()->key();
            auto root_id    = iter.get()->value();

            auto block = this->getBlock(root_id);
            MEMORIA_RETURN_IF_ERROR(block);

            auto ctr_hash   = block.get()->ctr_type_hash();
            auto ctr_intf   = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            auto res0 = ctr_intf->walk(ctr_name, this->shared_from_this(), walker);
            MEMORIA_RETURN_IF_ERROR(res0);

            auto res1 = iter.get()->next();
            MEMORIA_RETURN_IF_ERROR(res1);
        }

        walker->endSnapshot();

        return VoidResult::of();
    }



    virtual VoidResult dump_persistent_tree() noexcept {
        persistent_tree_.dump_tree();
        return VoidResult::of();
    }


    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id) noexcept
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_cxx_typedecl());
        return Result<CtrSharedPtr<CtrReferenceable<Profile>>>::of(
                factory->create_instance(this->shared_from_this(), ctr_id, decl)
        );
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> create(const LDTypeDeclarationView& decl) noexcept
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_cxx_typedecl());

        auto ctr_name = this->createCtrName();
        MEMORIA_RETURN_IF_ERROR(ctr_name);

        return Result<CtrSharedPtr<CtrReferenceable<Profile>>>::of(
            factory->create_instance(this->shared_from_this(), ctr_name.get(), decl)
        );
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> find(const CtrID& ctr_id) noexcept
    {
        using ResultT = Result<CtrSharedPtr<CtrReferenceable<Profile>>>;
        checkIfConainersOpeneingAllowed();

        auto root_id = getRootID(ctr_id);
        MEMORIA_RETURN_IF_ERROR(root_id);

        if (root_id.get().is_set())
        {
            auto block = this->getBlock(root_id.get());
            MEMORIA_RETURN_IF_ERROR(block);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block.get()->ctr_type_hash());

            auto ii = instance_map_.find(ctr_id);
            if (ii != instance_map_.end())
            {
                auto ctr_hash = block.get()->ctr_type_hash();
                auto instance_hash = ii->second->type_hash();

                if (instance_hash == ctr_hash) {
                    return ResultT::of(ii->second->shared_self());
                }
                else {
                    return VoidResult::make_error_tr(
                                "Exisitng ctr instance type hash mismatch: expected {}, actual {}",
                                ctr_hash,
                                instance_hash
                    );
                }
            }
            else {
                return ctr_intf->new_ctr_instance(block.get(), this->shared_from_this());
            }
        }
        else {
            return ResultT::of();
        }
    }

    void pack_store()
    {
        this->history_tree_raw_->pack();
    }

    virtual Result<U8String> ctr_type_name(const CtrID& name) noexcept
    {
        using ResultT = Result<U8String>;
        auto root_id = getRootID(name);
        MEMORIA_RETURN_IF_ERROR(root_id);

        if (root_id.get().is_set())
        {
            auto block = this->getBlock(root_id.get());
            MEMORIA_RETURN_IF_ERROR(block);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block.get()->ctr_type_hash());

            return ResultT::of(ctr_intf->ctr_type_name());
        }
        else {
            return ResultT::make_error("Can't find container with name {}", name);
        }
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> from_root_id(const CtrID& root_block_id, const CtrID& name) noexcept
    {
        using ResultT = Result<CtrSharedPtr<CtrReferenceable<Profile>>>;

        if (root_block_id.is_set())
        {
            auto block = this->getBlock(root_block_id);
            MEMORIA_RETURN_IF_ERROR(block);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block.get()->ctr_type_hash());

            return ctr_intf->new_ctr_instance(block.get(), this->shared_from_this());
        }
        else {
            return ResultT::of();
        }
    }

    Result<CtrBlockDescription<Profile>> describe_block(const CtrID& block_id) noexcept
    {
        auto block = this->getBlock(block_id);
        MEMORIA_RETURN_IF_ERROR(block);
        return describe_block(block.get());
    }

    Result<CtrBlockDescription<Profile>> describe_block(const BlockG& block) noexcept
    {
        auto ctr_intf = ProfileMetadata<Profile>::local()
                ->get_container_operations(block->ctr_type_hash());

        return ctr_intf->describe_block1(block->id(), this->shared_from_this());
    }

protected:

    SharedPtr<SnapshotMemoryStat<Profile>> do_compute_memory_stat()
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

    SharedPtr<SnapshotMemoryStat<Profile>> do_compute_memory_stat(_::BlockSet& visited_blocks)
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




    VoidResult clone_foreign_block(const BlockType* foreign_block) noexcept
    {
        auto new_block = clone_block(foreign_block);
        MEMORIA_RETURN_IF_ERROR(new_block);

        ptree_set_new_block(new_block.get());

        return VoidResult::of();
    }


    Result<BlockType*> clone_block(const BlockType* block)
    {
        using ResultT = Result<BlockType*>;

        char* buffer = (char*) this->malloc(block->memory_block_size());

        CopyByteBuffer(block, buffer, block->memory_block_size());
        BlockType* new_block = ptr_cast<BlockType>(buffer);

        auto new_block_id = newId();
        MEMORIA_RETURN_IF_ERROR(new_block_id);

        new_block->uuid() = new_block_id.get();

        return ResultT::of(new_block);
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
            MMA_THROW(Exception()) << WhatInfo(format_u8("Snapshot's {} data is locked", uuid()));
    	}
    }

    void checkIfConainersCreationAllowed()
    {
    	if (!is_active())
    	{
            MMA_THROW(Exception()) << WhatInfo(format_u8("Snapshot's {} data is not active, snapshot status = {}", uuid(), (int32_t)history_node_->status()));
    	}
    }


    void checkIfExportAllowed()
    {
    	if (history_node_->is_dropped())
    	{
    		// Double checking. This shouldn't happen
    		if (!history_node_->root())
    		{
                MMA_THROW(Exception()) << WhatInfo(format_u8("Snapshot {} has been cleared", uuid()));
    		}
    	}
    	else if (history_node_->is_active()) {
            MMA_THROW(Exception()) << WhatInfo(format_u8("Snapshot {} is still active", uuid()));
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
            MMA_THROW(Exception()) << WhatInfo(format_u8("Snapshot {} has been already committed or data is locked", uuid()));
        }
    }

    void checkUpdateAllowed(const CtrID& ctrName)
    {
    	checkReadAllowed();

    	if ((!history_node_->is_active()) && ctrName.is_set())
    	{
            MMA_THROW(Exception()) << WhatInfo(format_u8("Snapshot {} has been already committed or data is locked", uuid()));
    	}
    }

    void checkIfDataLocked()
    {
    	checkReadAllowed();

    	if (!history_node_->is_data_locked())
    	{
            MMA_THROW(Exception()) << WhatInfo(format_u8("Snapshot {} hasn't been locked", uuid()));
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

}}}
