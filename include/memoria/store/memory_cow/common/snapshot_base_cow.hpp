
// Copyright 2016-2021 Victor Smirnov
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

#include <memoria/store/memory_cow/common/store_stat_cow.hpp>

#include <memoria/core/container/store.hpp>
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

namespace detail {

template <typename ID> struct IDValueHolderH;

template <typename ID>
using IDValueHolderT = typename IDValueHolderH<ID>::IDValueHolder;

template <typename ValueHolder>
struct IDValueHolderH<CowBlockID<ValueHolder>> {
    using IDType = CowBlockID<ValueHolder>;
    using IDValueHolder = ValueHolder;

    static IDValueHolder from_id(const IDType& id) noexcept {
        return id.value();
    }

    static IDType to_id(const IDValueHolder& id) noexcept {
        return IDType(id);
    }

    template <typename BlockType>
    static BlockType* get_block_ptr(IDType id) noexcept {
        return value_cast<BlockType*>(id.value());
    }

    template <typename BlockType>
    static IDType to_id(BlockType* ptr) noexcept {
        return IDType(value_cast<IDValueHolder>(ptr));
    }

    static IDValueHolder to_id_value(IDType id) noexcept {
        return id.value();
    }

    template <typename T>
    static IDValueHolder new_id(T* ptr) noexcept {
        return ++ptr->id_counter_;
    }
};

template <>
struct IDValueHolderH<CowBlockID<UUID>> {
    using IDType = CowBlockID<UUID>;
    using IDValueHolder = uint64_t;

    static IDValueHolder from_id(const IDType& id) noexcept {
        return id.value().lo();
    }

    static IDType to_id(const IDValueHolder& id) noexcept {
        return IDType(UUID{0, id});
    }

    template <typename BlockType>
    static BlockType* get_block_ptr(IDType id) noexcept {
        return value_cast<BlockType*>(id.value().lo());
    }

    template <typename BlockType>
    static IDType to_id(BlockType* ptr) noexcept {
        return IDType(UUID{0, value_cast<IDValueHolder>(ptr)});
    }


    static UUID to_id_value(IDType id) noexcept {
        return id.value();
    }

    template <typename T>
    static IDType new_id(T* ptr) noexcept {
        return IDType{UUID::make_random()};
    }
};

}

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
    using ApiProfileT = ApiProfile<ProfileT>;

    using typename Base::CtrID;
    using typename Base::BlockID;
    using typename Base::BlockGUID;
    using typename Base::BlockType;
    using typename Base::SnapshotID;
    using typename Base::Shared;

protected:

	using HistoryNode		= typename PersistentAllocator::HistoryNode;
    


    using PersistentAllocatorPtr = AllocSharedPtr<PersistentAllocator>;
    using SnapshotPtr            = SnpSharedPtr<MyType>;
    using SnapshotApiPtr         = SnpSharedPtr<IMemorySnapshot<ApiProfileT>>;
    using AllocatorPtr           = AllocSharedPtr<Base>;

    using Status            = typename HistoryNode::Status;

    using CtrInstanceMap = std::unordered_map<CtrID, CtrReferenceable<ApiProfileT>*>;

    using typename IMemorySnapshot<ApiProfile<Profile>>::ROStoreSnapshotPtr;

public:

    template <typename CtrName>
    using CtrT = SharedCtr<CtrName, ProfileStoreType<Profile>, Profile>;

    template <typename CtrName>
    using CtrPtr = CtrSharedPtr<CtrT<CtrName>>;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;

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
    mutable boost::object_pool<Shared> shared_pool_;


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


    virtual ~SnapshotBase() noexcept {}

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
        check_updates_allowed();

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

    virtual void unref_ctr_root(const BlockID& root_block_id)
    {
        auto block = this->getBlock(root_block_id);

        if (block.as_mutable()->unref_block())
        {
            auto ctr_hash = block->ctr_type_hash();

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            auto ctr = ctr_intf->new_ctr_instance(block, this);

            ctr->internal_unref_cascade(block_id_holder_from(root_block_id));
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



    void import_new_ctr_from(ROStoreSnapshotPtr ptr, const CtrID& name)
    {
        check_updates_allowed();

        SnapshotPtr txn = memoria_dynamic_pointer_cast<MyType>(ptr);

        txn->checkIfExportAllowed();
        auto root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

        if (root_id.is_null())
    	{
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

    void import_ctr_from(ROStoreSnapshotPtr ptr, const CtrID& name)
    {
        check_updates_allowed();

        SnapshotPtr txn = memoria_dynamic_pointer_cast<MyType>(ptr);

        txn->checkIfExportAllowed();

        auto root_id = this->getRootID(name);

        if (!root_id.is_null())
        {
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


    void copy_new_ctr_from(ROStoreSnapshotPtr ptr, const CtrID& name)
    {
        SnapshotPtr txn = memoria_dynamic_pointer_cast<MyType>(ptr);

        check_updates_allowed();

        auto root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&](const BlockGUID&, const BlockID&, const void* block_data){
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





    void copy_ctr_from(ROStoreSnapshotPtr ptr, const CtrID& name)
    {
        SnapshotPtr txn = memoria_dynamic_pointer_cast<MyType>(ptr);

        check_updates_allowed();

        auto root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

        if (!root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&, this](const BlockGUID& uuid, const BlockID& id, const void* block_data) {
                auto block = this->getBlock(id);
                if (block && block->uuid() == uuid)
                {
                    return;
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
        BlockType* block = value_cast<BlockType*>(detail::IDValueHolderH<BlockID>::from_id(id));
        if (block) {
            Shared* shared = shared_pool_.construct(id, block, 0);
            return SharedBlockConstPtr{shared};
        }
        else {
            return SharedBlockConstPtr{};
        }
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
            return SharedBlockConstPtr();
        }
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

    virtual void traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    )
    {
        auto instance = from_root_id(root_block);
        return instance->traverse_ctr(&node_handler);
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

    virtual void ref_block(const BlockID& block_id)
    {
        auto block = getBlock(block_id);
        block.as_mutable()->ref_block(1);
    }

    virtual void unref_block(const BlockID& block_id, std::function<void ()> on_zero)
    {
        auto block = getBlock(block_id);
        if (block.as_mutable()->unref_block()) {
            return on_zero();
        }
    }



    virtual void removeBlock(const BlockID& id)
    {
        check_updates_allowed();

        BlockType* block = detail::IDValueHolderH<BlockID>::template get_block_ptr<BlockType>(id);
        freeMemory(block);
    }



    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID&)
    {
        check_updates_allowed();

        if (initial_size == -1)
        {
            initial_size = DEFAULT_BLOCK_SIZE;
        }

        auto id = newId();

        void* buf = allocate_system<uint8_t>(static_cast<size_t>(initial_size)).release();
        memset(buf, 0, static_cast<size_t>(initial_size));

        BlockType* p = new (buf) BlockType(id, id.value(), id.value());
        p->id() = detail::IDValueHolderH<BlockID>::to_id(p);
        p->snapshot_id() = uuid();

        p->memory_block_size() = initial_size;

        Shared* shared = shared_pool_.construct(id, p, 0);

        return SharedBlockPtr{shared};
    }


    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID&)
    {
        check_updates_allowed();

        auto new_id = newId();

        auto new_block = this->clone_block(block.block());

        new_block->id_value() = detail::IDValueHolderH<BlockID>::to_id_value(new_id);
        new_block->id() = detail::IDValueHolderH<BlockID>::to_id(new_block);

        Shared* shared = shared_pool_.construct(new_id, new_block, 0);

        return SharedBlockPtr{shared};
    }



    virtual BlockID newId() noexcept {
        return BlockID{history_tree_raw_->newBlockId()};
    }

    virtual CtrID currentTxnId() const {
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
                return iter->value().view();
            }            
        }
        else {
            return history_node_->root_id();
        }

        return BlockID{};
    }

    virtual void setRoot(const CtrID& name, const BlockID& root)
    {
        if (root.is_null())
        {
            if (!name.is_null())
            {
                auto prev_id = root_map_->remove_and_return(name);
                if (prev_id)
                {
                    unref_ctr_root(prev_id.get());
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Allocator directory removal attempted").do_throw();
            }
        }
        else {
            if (!name.is_null())
            {
                auto root_block = findBlock(root);
                root_block.as_mutable()->ref_block();

                auto prev_id = root_map_->replace_and_return(name, root);

                if (prev_id)
                {
                    unref_ctr_root(prev_id.get());
                }
            }
            else {
                auto root_id_to_remove = history_node_->update_directory_root(root);
                if (root_id_to_remove.isSet())
                {
                    auto instance = from_root_id(root_id_to_remove);

                    // TODO: Do we need to unref the block here first?
                    return instance->internal_unref_cascade(
                        block_id_holder_from(root_id_to_remove)
                    );
                }
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
            return history_node_->root_id().is_null();
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
    }


    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(
                    this->shared_from_this(),
                    ctr_id,
                    decl
        );
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const LDTypeDeclarationView& decl)
    {
        checkIfConainersCreationAllowed();
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_cxx_typedecl());

        auto ctr_name = this->createCtrName();

        return factory->create_instance(
                    this->shared_from_this(),
                    ctr_name,
                    decl
        );
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

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> from_root_id(const BlockID& root_block_id)
    {
        if (root_block_id.is_set())
        {
            auto block = this->getBlock(root_block_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            CtrID ctr_id = ctr_intf->get_ctr_id(block);

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


    BlockType* clone_block(const BlockType* block)
    {
        char* buffer = (char*) this->malloc(block->memory_block_size());

        CopyByteBuffer(block, buffer, block->memory_block_size());
        BlockType* new_block = ptr_cast<BlockType>(buffer);

        auto new_block_id = newId();
        new_block->uuid() = new_block_id.value();
        new_block->snapshot_id() = uuid();
        new_block->set_references(0);

        return new_block;
    }



    void* malloc(size_t size)
    {
        return allocate_system<uint8_t>(size).release();
    }



    void checkIfConainersOpeneingAllowed()
    {
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
            if (!history_node_->root_id().isSet())
            {
                MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been cleared", uuid()).do_throw();
            }
    	}
    	else if (history_node_->is_active()) {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is still active", uuid()).do_throw();
    	}
    }




    void check_updates_allowed()
    {
        if (!(history_node_->is_active() || snapshot_removal_))
        {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been already committed. No updates permitted.", uuid()).do_throw();
        }
    }



    void do_drop() noexcept
    {
        snapshot_removal_ = true;

        if (history_tree_raw_->is_dump_snapshot_lifecycle()) {
            std::cout << "MEMORIA: DROP snapshot's DATA: " << history_node_->snapshot_id() << std::endl;
        }

        BlockType* root_blk = detail::IDValueHolderH<BlockID>::template get_block_ptr<BlockType>(history_node_->root_id());
        if (root_blk->unref_block())
        {
            root_map_->internal_unref_cascade(
                block_id_holder_from(root_blk->id())
            );
        }

        history_node_->reset_root_id();
    }

    virtual void releaseBlock(Shared* block) noexcept {
        shared_pool_.destroy(block);
    }

    virtual void updateBlock(Shared* block) {
        make_generic_error("updateBlock is not implemented!!!").do_throw();
    }

    void clone_foreign_block(const BlockType* foreign_block)
    {
    }
};

}}}
