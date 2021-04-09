
// Copyright 2016-2020 Victor Smirnov
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

#include <memoria/store/memory_cow_lite/common/store_stat_cow.hpp>
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

#include <memoria/core/memory/ptr_cast.hpp>

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <vector>
#include <memory>
#include <mutex>

namespace memoria {
namespace store {
namespace memory_cow {

template <typename Profile, typename PersistentAllocator, typename SnapshotType>
class SnapshotBase:
        public ProfileAllocatorType<Profile>,
        public IMemorySnapshot<ApiProfile<Profile>>,
        public SnpSharedFromThis<SnapshotType>
{    
protected:
	using MyType			= SnapshotType;
    using Base              = ProfileAllocatorType<Profile>;

public:
    using ProfileT = Profile;
    using ApiProfileT = ApiProfile<ProfileT>;

    using typename Base::CtrID;
    using typename Base::BlockID;
    using typename Base::BlockType;
    using typename Base::SnapshotID;
protected:

	using HistoryNode		= typename PersistentAllocator::HistoryNode;
    


    using PersistentAllocatorPtr = AllocSharedPtr<PersistentAllocator>;
    using SnapshotPtr            = SnpSharedPtr<MyType>;
    using SnapshotApiPtr         = SnpSharedPtr<IMemorySnapshot<ApiProfileT>>;
    using AllocatorPtr           = AllocSharedPtr<Base>;

    using Status            = typename HistoryNode::Status;

    using CtrInstanceMap = std::unordered_map<CtrID, CtrReferenceable<ApiProfileT>*>;

public:

    template <typename CtrName>
    using CtrT = SharedCtr<CtrName, ProfileAllocatorType<Profile>, Profile>;

    template <typename CtrName>
    using CtrPtr = CtrSharedPtr<CtrT<CtrName>>;

    using typename Base::BlockG;

    using RootMapType = CtrT<Map<CtrID, BlockID>>;

protected:

    HistoryNode*            history_node_;
    PersistentAllocatorPtr  history_tree_;
    PersistentAllocator*    history_tree_raw_ = nullptr;

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

    bool snapshot_removal_{false};

    mutable ObjectPools object_pools_;

public:

    SnapshotBase(MaybeError& maybe_error, HistoryNode* history_node, const PersistentAllocatorPtr& history_tree):
        history_node_(history_node),
        history_tree_(history_tree),
        history_tree_raw_(history_tree.get()),
        logger_("PersistentInMemStoreSnp", Logger::DERIVED, &history_tree->logger_)
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
        logger_("PersistentInMemStoreSnp")
    {
        history_node_->ref();

        if (history_node->is_active())
        {
            history_tree_raw_->ref_active();
        }
    }
    
    VoidResult post_init() noexcept
    {
        auto ptr = this->shared_from_this();

        BlockID root_id = history_node_->root_id();

        MaybeError maybe_error;
        if (root_id.isSet())
        {
            MEMORIA_TRY(root_block, findBlock(root_id));
            root_map_ = ctr_make_shared<RootMapType>(maybe_error, ptr, root_block);
        }
        else {
            root_map_ = ctr_make_shared<RootMapType>(maybe_error, ptr, CtrID{}, Map<CtrID, BlockID>());
        }

        root_map_->reset_allocator_holder();

        if (!maybe_error) {
            return VoidResult::of();
        }
        else {
            return std::move(maybe_error.get());
        }
    }
    
    static void init_profile_metadata() {
        RootMapType::init_profile_metadata();
    }


    virtual ~SnapshotBase() noexcept {}

    virtual ObjectPools& object_pools() const noexcept {
        return object_pools_;
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

        MEMORIA_TRY(ii, root_map_->ctr_begin());

        while (!ii->iter_is_end())
        {
            names.push_back(ii->key());
            MEMORIA_TRY_VOID(ii->next());
        }

        return Result<std::vector<CtrID>>::of(std::move(names));
    }

    Result<std::vector<U8String>> container_names_str() const noexcept
    {
        std::vector<U8String> names;

        MEMORIA_TRY(ii, root_map_->ctr_begin());
        while (!ii->iter_is_end())
        {
            std::stringstream ss;
            ss << ii->key().view();

            names.push_back(U8String(ss.str()));
            MEMORIA_TRY_VOID(ii->next());
        }

        return Result<std::vector<U8String>>::of(std::move(names));
    }


    BoolResult drop_ctr(const CtrID& name) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());

        MEMORIA_TRY(root_id, getRootID(name));

        if (root_id.is_set())
        {
            MEMORIA_TRY(block, this->getBlock(root_id));

            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(block->ctr_type_hash());

            MEMORIA_TRY_VOID(ctr_intf->drop(name, this->shared_from_this()));
            return BoolResult::of(true);
        }
        else {
            return BoolResult::of(false);
        }
    }

    virtual VoidResult unref_ctr_root(const BlockID& root_block_id) noexcept
    {
        MEMORIA_TRY(block, this->getBlock(root_block_id));

        if (block->unref_block())
        {
            auto ctr_hash = block->ctr_type_hash();

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            MEMORIA_TRY(ctr, ctr_intf->new_ctr_instance(block, this));

            MEMORIA_TRY_VOID(ctr->internal_unref_cascade(block_id_holder_from(root_block_id)));
        }

        return VoidResult::of();
    }


    Result<Optional<U8String>> ctr_type_name_for(const CtrID& name) noexcept
    {
        using ResultT = Result<Optional<U8String>>;

        MEMORIA_TRY(root_id, this->getRootID(name));

        MEMORIA_TRY(block, this->getBlock(root_id));

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();

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
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot is already committed.");
        }
    }

    VoidResult for_each_ctr_node(const CtrID& name, typename ContainerOperations<Profile>::BlockCallbackFn fn) noexcept
    {
        MEMORIA_TRY(root_id, this->getRootID(name));
        MEMORIA_TRY(block, this->getBlock(root_id));

        if (block)
    	{
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->for_each_ctr_node(name, this->shared_from_this(), fn);
    	}
    	else {
            return MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot {}", name, history_node_->snapshot_id());
    	}
    }



    VoidResult import_new_ctr_from(SnapshotApiPtr ptr, const CtrID& name) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());

        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

        MEMORIA_TRY_VOID(txn->checkIfExportAllowed());
        MEMORIA_TRY(root_id, this->getRootID(name));

    	auto txn_id = currentTxnId();

        if (root_id.is_null())
    	{
            auto res = txn->for_each_ctr_node(name, [&](const BlockID&, const BlockID& id, const void*) noexcept -> VoidResult {
//                auto rc_handle = txn->export_block_rchandle(id);
//    			using Value = typename PersistentTreeT::Value;

//    			rc_handle->ref();

//    			auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

//                if (old_value.block_ptr())
//    			{
//                    return MEMORIA_MAKE_GENERIC_ERROR("Block with ID {} is not new in snapshot {}", id, txn_id);
//    			}

                return VoidResult::of();
    		});

            MEMORIA_RETURN_IF_ERROR(res);

            MEMORIA_TRY(root_id, txn->getRootID(name));

            if (root_id.is_set())
    		{
                MEMORIA_TRY_VOID(root_map_->assign(name, root_id));
    		}
    		else {
                return MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId());
    		}
    	}
    	else {
            return MEMORIA_MAKE_GENERIC_ERROR("Container with name {} already exists in snapshot {}", name, txn_id);
    	}

        return VoidResult::of();
    }


    VoidResult copy_new_ctr_from(SnapshotApiPtr ptr, const CtrID& name) noexcept
    {
        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

        MEMORIA_TRY_VOID(check_updates_allowed());

        MEMORIA_TRY(root_id, this->getRootID(name));

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
            auto res = txn->for_each_ctr_node(name, [&](const BlockID&, const BlockID&, const void* block_data) noexcept -> VoidResult {
                return clone_foreign_block(ptr_cast<const BlockType>(block_data));
    		});
            MEMORIA_RETURN_IF_ERROR(res);

            MEMORIA_TRY(root_id1, txn->getRootID(name));

            if (root_id1.is_set()) {
                MEMORIA_TRY_VOID(root_map_->assign(name, root_id1));
    		}
    		else {
                return MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId());
    		}
    	}
    	else {
            return MEMORIA_MAKE_GENERIC_ERROR("Container with name {} already exists in snapshot {}", name, txn_id);
    	}

        return VoidResult::of();
    }


    Result<void> import_ctr_from(SnapshotApiPtr ptr, const CtrID& name) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());

        SnapshotPtr txn = memoria_static_pointer_cast<MyType>(ptr);

        MEMORIA_TRY_VOID(txn->checkIfExportAllowed());

        MEMORIA_TRY(root_id, this->getRootID(name));

//    	auto txn_id = uuid();

        if (!root_id.is_null())
    	{
            auto res = txn->for_each_ctr_node(name, [&](const BlockID& uuid, const BlockID& id, const void*) noexcept -> VoidResult {
//                MEMORIA_TRY(block, this->getBlock(id));

//                if (block && block->uuid() == uuid)
//    			{
//                    return VoidResult::of();
//    			}

//                auto rc_handle = txn->export_block_rchandle(id);
//    			using Value = typename PersistentTreeT::Value;

//    			rc_handle->ref();

//    			auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

//                if (old_value.block_ptr())
//    			{
//                    if (old_value.block_ptr()->unref() == 0)
//    				{
//                        // FIXME: just delete the block?
//                        return MEMORIA_MAKE_GENERIC_ERROR("Unexpected refcount == 0 for block {}", old_value.block_ptr()->raw_data()->uuid());
//    				}
//    			}

                return VoidResult::of();
    		});
            MEMORIA_RETURN_IF_ERROR(res);

            MEMORIA_TRY(root_id, txn->getRootID(name));
            if (root_id.is_set())
    		{
                MEMORIA_TRY_VOID(root_map_->assign(name, root_id));
    		}
    		else {
                return MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId());
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

        MEMORIA_TRY_VOID(check_updates_allowed());

        MEMORIA_TRY(root_id, this->getRootID(name));

    	auto txn_id = currentTxnId();

        if (!root_id.is_null())
    	{
            auto res = txn->for_each_ctr_node(name, [&, this](const BlockID& uuid, const BlockID& id, const void* block_data) noexcept -> VoidResult {
                MEMORIA_TRY(block, this->getBlock(id));
                if (block && block->uuid() == uuid)
    			{
                    return VoidResult::of();
    			}

                return clone_foreign_block(ptr_cast<const BlockType>(block_data));
    		});

            MEMORIA_RETURN_IF_ERROR(res);

            MEMORIA_TRY(root_id1, txn->getRootID(name));
            if (root_id1.is_set())
    		{
                MEMORIA_TRY_VOID(root_map_->assign(name, root_id1));
    		}
    		else {
                return MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn_id);
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
        MEMORIA_TRY(root_id, this->getRootID(ctr_name));
        MEMORIA_TRY(block, this->getBlock(root_id));

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->clone_ctr(ctr_name, new_ctr_name, this->shared_from_this());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot {} ", ctr_name, history_node_->snapshot_id());
        }
    }

    Result<BlockG> findBlock(const BlockID& id) noexcept
    {
        using ResultT = Result<BlockG>;

        BlockType* block = value_cast<BlockType*>(id.value());
        return ResultT::of(BlockG{block});

        return ResultT::of();
    }

    virtual Result<BlockG> getBlock(const BlockID& id) noexcept
    {
        if (id.isSet())
        {
            MEMORIA_TRY(block, findBlock(id));

            if (block) {
                return block_result;
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Block is not found for the specified id: {}", id);
            }
        }
        else {
            return Result<BlockG>::of(BlockG());
        }
    }




    virtual Result<void> registerCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>* instance) noexcept
    {
        auto ii = instance_map_.find(ctr_id);
    	if (ii == instance_map_.end())
    	{
            instance_map_.insert({ctr_id, instance});
    	}
    	else {
            return MEMORIA_MAKE_GENERIC_ERROR("Container with name {} has been already registered", ctr_id);
    	}

        return Result<void>::of();
    }

    virtual VoidResult unregisterCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>*) noexcept
    {
        instance_map_.erase(ctr_id);
        return VoidResult::of();
    }

    virtual VoidResult traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    ) noexcept
    {
        MEMORIA_TRY(instance, from_root_id(root_block, CtrID{}));
        return instance->traverse_ctr(&node_handler);
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
            MEMORIA_TRY_VOID(pair.second->flush());
        }

        return VoidResult::of();
    }

    VoidResult dump_dictionary_blocks() noexcept
    {
        MEMORIA_TRY(ii, root_map_->ctr_begin());
        if (!ii->iter_is_end())
        {
            do {
                MEMORIA_TRY_VOID(ii->dump());

                MEMORIA_TRY(has_next, ii->iter_next_leaf());
                if (!has_next) break;
            }
            while (true);
        }

        return VoidResult::of();
    }

    virtual VoidResult ref_block(const BlockID& block_id) noexcept
    {
        MEMORIA_TRY(block, getBlock(block_id));
        block->ref_block(1);
        return VoidResult::of();
    }

    virtual VoidResult unref_block(const BlockID& block_id, std::function<VoidResult()> on_zero) noexcept
    {
        MEMORIA_TRY(block, getBlock(block_id));
        if (block->unref_block()) {
            return on_zero();
        }

        return VoidResult::of();
    }



    virtual VoidResult removeBlock(const BlockID& id) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());

        BlockType* block = value_cast<BlockType*>(id.value());
        freeMemory(block);

        return VoidResult::of();
    }



    virtual Result<BlockG> createBlock(int32_t initial_size) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());

        if (initial_size == -1)
        {
            initial_size = DEFAULT_BLOCK_SIZE;
        }

        MEMORIA_TRY(id, newId());

        void* buf = allocate_system<uint8_t>(static_cast<size_t>(initial_size)).release();
        memset(buf, 0, static_cast<size_t>(initial_size));

        BlockType* p = new (buf) BlockType(id);
        p->id_value() = id.value();
        p->id() = BlockID{value_cast<typename BlockID::ValueHolder>(p)};
        p->snapshot_id() = uuid();

        p->memory_block_size() = initial_size;

        return Result<BlockG>::of(BlockG{p});
    }


    virtual Result<BlockG> cloneBlock(const BlockG& block) noexcept
    {
        MEMORIA_TRY_VOID(check_updates_allowed());

        MEMORIA_TRY(new_id, newId());

        MEMORIA_TRY(new_block, this->clone_block(block.block()));

        new_block->id_value() = new_id.value();
        new_block->id() = BlockID{value_cast<typename BlockID::ValueHolder>(new_block)};

        return Result<BlockG>::of(BlockG{new_block});
    }



    virtual Result<BlockID> newId() noexcept {
        return Result<BlockID>::of(BlockID{history_tree_raw_->newBlockId()});
    }

    virtual CtrID currentTxnId() const noexcept {
        return history_node_->snapshot_id();
    }

    // memory pool allocator
    virtual Result<void*> allocateMemory(size_t size) noexcept {
        return wrap_throwing([&]() -> Result<void*> {
            return Result<void*>::of(allocate_system<uint8_t>(size).release());
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
            MEMORIA_TRY(iter, root_map_->ctr_map_find(name));

            if (iter->is_found(name))
            {
                return Result<BlockID>::of(iter->value().view());
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
                MEMORIA_TRY(prev_id, root_map_->remove_and_return(name));
                if (prev_id)
                {
                    MEMORIA_TRY_VOID(unref_ctr_root(prev_id.get()));
                }
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Allocator directory removal attempted");
            }
        }
        else {
            if (!name.is_null())
            {
                MEMORIA_TRY(root_block, findBlock(root));
                root_block->ref_block();

                MEMORIA_TRY(prev_id, root_map_->replace_and_return(name, root));

                if (prev_id)
                {
                    MEMORIA_TRY_VOID(unref_ctr_root(prev_id.get()));
                }
            }
            else {
                MEMORIA_TRY(root_id_to_remove, history_node_->update_directory_root(root));
                if (root_id_to_remove.isSet())
                {
                    MEMORIA_TRY(instance, from_root_id(root_id_to_remove, CtrID{}));
                    // TODO: Do we need to unref the block here first?
                    return instance->internal_unref_cascade(
                        block_id_holder_from(root_id_to_remove)
                    );
                }
            }
        }

        return VoidResult::of();
    }

    virtual BoolResult hasRoot(const CtrID& name) noexcept
    {
        if (MMA_UNLIKELY(!root_map_)) {
            return BoolResult::of(false);
        }

        if (!name.is_null())
        {
            MEMORIA_TRY(iter, root_map_->ctr_map_find(name));
            return BoolResult::of(iter->is_found(name));
        }
        else {
            return BoolResult::of(!history_node_->root_id().is_null());
        }
    }

    virtual Result<CtrID> createCtrName() noexcept
    {
        return Result<CtrID>::of(ProfileTraits<Profile>::make_random_ctr_id());
    }


    virtual BoolResult check() noexcept
    {
        bool result = false;

        MEMORIA_TRY(iter, root_map_->ctr_begin());

        while(!iter->is_end())
        {
            auto ctr_name = iter->key();

            MEMORIA_TRY(block, this->getBlock(iter->value()));

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            MEMORIA_TRY(res, ctr_intf->check(ctr_name, this->shared_from_this()));

            result = res || result;

            MEMORIA_TRY_VOID(iter->next());
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

        MEMORIA_TRY(iter, root_map_->ctr_begin());
        while (!iter->iter_is_end())
        {
            auto ctr_name   = iter->key();
            auto root_id    = iter->value();

            MEMORIA_TRY(block, this->getBlock(root_id));

            auto ctr_hash   = block->ctr_type_hash();
            auto ctr_intf   = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            MEMORIA_TRY_VOID(ctr_intf->walk(ctr_name, this->shared_from_this(), walker));

            MEMORIA_TRY_VOID(iter->next());
        }

        walker->endSnapshot();

        return VoidResult::of();
    }



    virtual VoidResult dump_persistent_tree() noexcept {
        return VoidResult::of();
    }


    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id) noexcept
    {
        MEMORIA_TRY_VOID(checkIfConainersCreationAllowed());
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(this->shared_from_this(), ctr_id, decl);
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> create(const LDTypeDeclarationView& decl) noexcept
    {
        MEMORIA_TRY_VOID(checkIfConainersCreationAllowed());
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_cxx_typedecl());

        MEMORIA_TRY(ctr_name, this->createCtrName());

        return factory->create_instance(this->shared_from_this(), ctr_name, decl);
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> find(const CtrID& ctr_id) noexcept
    {
        using ResultT = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;
        MEMORIA_TRY_VOID(checkIfConainersOpeneingAllowed());

        MEMORIA_TRY(root_id, getRootID(ctr_id));
        if (root_id.is_set())
        {
            MEMORIA_TRY(block, this->getBlock(root_id));

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            auto ii = instance_map_.find(ctr_id);
            if (ii != instance_map_.end())
            {
                auto ctr_hash = block->ctr_type_hash();
                auto instance_hash = ii->second->type_hash();

                if (instance_hash == ctr_hash) {
                    return ResultT::of(ii->second->shared_self());
                }
                else {
                    return MEMORIA_MAKE_GENERIC_ERROR(
                                "Exisitng ctr instance type hash mismatch: expected {}, actual {}",
                                ctr_hash,
                                instance_hash
                    );
                }
            }
            else {
                return ctr_intf->new_ctr_instance(block, this->shared_from_this());
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
        MEMORIA_TRY(root_id, getRootID(name));
        if (root_id.is_set())
        {
            MEMORIA_TRY(block, this->getBlock(root_id));

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ResultT::of(ctr_intf->ctr_type_name());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't find container with name {}", name);
        }
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> from_root_id(const BlockID& root_block_id, const CtrID& name) noexcept
    {
        using ResultT = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;

        if (root_block_id.is_set())
        {
            MEMORIA_TRY(block, this->getBlock(root_block_id));

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ctr_intf->new_ctr_instance(block, this);
        }
        else {
            return ResultT::of();
        }
    }

    Result<CtrBlockDescription<ApiProfileT>> describe_block(const CtrID& block_id) noexcept
    {
        MEMORIA_TRY(block, this->getBlock(block_id));
        return describe_block(block);
    }

    Result<CtrBlockDescription<ApiProfileT>> describe_block(const BlockG& block) noexcept
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
//            PersistentTreeT persistent_tree(node);
//            persistent_tree.conditional_tree_traverse(vp_accum);

            node = node->parent();
        }

        SnapshotStatsCountingConsumer<SnapshotBase> consumer(visited_blocks, this);

//        persistent_tree_.conditional_tree_traverse(consumer);

        return consumer.finish();
    }

    SharedPtr<SnapshotMemoryStat<ApiProfileT>> do_compute_memory_stat(_::BlockSet& visited_blocks)
    {
        SnapshotStatsCountingConsumer<SnapshotBase> consumer(visited_blocks, this);

//        persistent_tree_.conditional_tree_traverse(consumer);

        return consumer.finish();
    }

    VoidResult clone_foreign_block(const BlockType* foreign_block) noexcept
    {
        MEMORIA_TRY(new_block, clone_block(foreign_block));
        ptree_set_new_block(new_block);

        return VoidResult::of();
    }


    Result<BlockType*> clone_block(const BlockType* block)
    {
        using ResultT = Result<BlockType*>;

        char* buffer = (char*) this->malloc(block->memory_block_size());

        CopyByteBuffer(block, buffer, block->memory_block_size());
        BlockType* new_block = ptr_cast<BlockType>(buffer);

        MEMORIA_TRY(new_block_id, newId());
        new_block->uuid() = new_block_id;
        new_block->snapshot_id() = uuid();
        new_block->set_references(0);

        return ResultT::of(new_block);
    }



    void* malloc(size_t size)
    {
        return allocate_system<uint8_t>(size).release();
    }

    void ptree_set_new_block(BlockType* block)
    {

    }


    VoidResult checkIfConainersOpeneingAllowed()
    {
        return VoidResult::of();
    }

    VoidResult checkIfConainersCreationAllowed()
    {
    	if (!is_active())
    	{
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot's {} data is not active, snapshot status = {}", uuid(), (int32_t)history_node_->status());
    	}

        return VoidResult::of();
    }


    VoidResult checkIfExportAllowed()
    {
    	if (history_node_->is_dropped())
    	{
    		// Double checking. This shouldn't happen
            if (!history_node_->root_id().isSet())
    		{
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been cleared", uuid());
    		}
    	}
    	else if (history_node_->is_active()) {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is still active", uuid());
    	}

        return VoidResult::of();
    }




    VoidResult check_updates_allowed() noexcept
    {
        if (!(history_node_->is_active() || snapshot_removal_))
        {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been already committed. No updates permitted.", uuid());
        }

        return VoidResult::of();

    }



    void do_drop() noexcept
    {
        snapshot_removal_ = true;

        if (history_tree_raw_->is_dump_snapshot_lifecycle()) {
            std::cout << "MEMORIA: DROP snapshot's DATA: " << history_node_->snapshot_id() << std::endl;
        }

        BlockType* root_blk = value_cast<BlockType*>(history_node_->root_id().value());
        if (root_blk->unref_block())
        {
            root_map_->internal_unref_cascade(
                block_id_holder_from(root_blk->id())
            ).get_or_terminate();
        }

        history_node_->reset_root_id();
    }
};

}}}
