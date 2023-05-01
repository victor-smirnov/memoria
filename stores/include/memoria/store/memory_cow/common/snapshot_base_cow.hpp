
// Copyright 2016-2022 Victor Smirnov
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
#include <memoria/core/tools/random.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/container/ctr_instance_pool.hpp>

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

    static IDValueHolder from_id(const IDType& id)  {
        return id.value();
    }

    static IDType to_id(const IDValueHolder& id)  {
        return IDType(id);
    }

    template <typename BlockType>
    static BlockType* get_block_ptr(IDType id)  {
        return value_cast<BlockType*>(id.value());
    }

    template <typename BlockType>
    static IDType to_id(BlockType* ptr)  {
        return IDType(value_cast<IDValueHolder>(ptr));
    }

    template <typename T>
    static IDValueHolder new_id(T* ptr)  {
        return ++ptr->id_counter_;
    }
};

template <>
struct IDValueHolderH<CowBlockID<UID256>> {
    using IDType = CowBlockID<UID256>;
    using IDValueHolder = uint64_t;

    static IDValueHolder from_id(const IDType& id)  {
        return id.value().counter();
    }

    static IDType to_id(const IDValueHolder& id)  {
        return IDType(UID256::make_type2(UID256{}, 0, id));
    }

    template <typename BlockType>
    static BlockType* get_block_ptr(IDType id)  {
        return value_cast<BlockType*>(id.value().counter());
    }

    template <typename BlockType>
    static IDType to_id(BlockType* ptr)  {
        return IDType(UID256::make_type2(UID256{}, 0, value_cast<IDValueHolder>(ptr)));
    }

    template <typename T>
    static IDType new_id(T* ptr)  {
        return IDType{UID256::make_random()};
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

    using HistoryNode = typename PersistentAllocator::HistoryNode;
    


    using PersistentAllocatorPtr = AllocSharedPtr<PersistentAllocator>;
    using SnapshotPtr            = SnpSharedPtr<MyType>;
    using SnapshotApiPtr         = SnpSharedPtr<IMemorySnapshot<ApiProfileT>>;
    using AllocatorPtr           = AllocSharedPtr<Base>;

    using Status = typename HistoryNode::Status;

    using typename IMemorySnapshot<ApiProfile<Profile>>::ROStoreSnapshotPtr;

public:

    template <typename CtrName>
    using CtrT = SharedCtr<CtrName, ProfileStoreType<Profile>, Profile>;
    template <typename CtrName>
    using RWCtrT = RWSharedCtr<CtrName, ProfileStoreType<Profile>, Profile>;

    template <typename CtrName>
    using CommonCtrT = ICtrApi<CtrName, ApiProfileT>;

    template <typename CtrName>
    using CtrPtr = CtrSharedPtr<CommonCtrT<CtrName>>;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;

    using RootMapType = Map<CtrID, BlockID>;

protected:

    mutable boost::object_pool<Shared> shared_pool_;
    mutable ObjectPools object_pools_;


    HistoryNode*            history_node_;
    PersistentAllocatorPtr  history_tree_;
    PersistentAllocator*    history_tree_raw_ = nullptr;

    template <typename>
    friend class ThreadsMemoryStoreImpl;
    
    template <typename>
    friend class FibersMemoryStoreImpl;
    
    
    template <typename, typename>
    friend class MemoryStoreBase;
    
    std::shared_ptr<CtrInstancePool<Profile>> instance_pool_;

    PairPtr pair_;
    
    CtrPtr<RootMapType> root_map_;

    bool snapshot_removal_{false};
    bool remove_cascade_{false};

public:

    SnapshotBase(
            MaybeError& maybe_error,
            HistoryNode* history_node,
            const PersistentAllocatorPtr& history_tree,
            bool remove_cascade
    ):
        history_node_(history_node),
        history_tree_(history_tree),
        history_tree_raw_(history_tree.get()),
        remove_cascade_(remove_cascade)
    {
        instance_pool_ = std::make_shared<CtrInstancePool<ProfileT>>();

        history_node_->ref();

        if (history_node->is_active())
        {
            history_tree_raw_->ref_active();
        }
    }

    SnapshotBase(
            MaybeError& maybe_error,
            HistoryNode* history_node,
            PersistentAllocator* history_tree,
            bool remove_cascade
    ):
        history_node_(history_node),
        history_tree_raw_(history_tree),
        remove_cascade_(remove_cascade)
    {
        instance_pool_ = std::make_shared<CtrInstancePool<ProfileT>>();

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
            auto root_block = findBlock(root_id);

            auto ctr_ref = find_ctr_instance(CtrID{}, root_block);
            root_map_ = memoria_static_pointer_cast<CommonCtrT<RootMapType>>(ctr_ref);
        }
        else {
            U8String signature = make_datatype_signature(RootMapType{}).name();
            auto doc = TypeSignature::parse(signature.to_std_string());
            auto decl = doc.root().as_datatype();
            auto ctr_ref = create_ctr_instance(decl, CtrID{});

            root_map_ = memoria_static_pointer_cast<CommonCtrT<RootMapType>>(ctr_ref);
        }

        root_map_->internal_detouch_from_store();
    }
    
    static void init_profile_metadata() {
        CtrT<RootMapType>::init_profile_metadata();
    }

    virtual ~SnapshotBase() noexcept {}

    CtrSharedPtr<CtrReferenceable<ApiProfileT>> create_ctr_instance(
        const hermes::Datatype& decl, const CtrID& ctr_id
    )
    {
        auto factory = ProfileMetadata<ProfileT>::local()->get_container_factories(decl.to_cxx_string());

        if (!instance_pool_->contains(ctr_id))
        {
            auto instance = factory->create_ctr_instance(
                this->shared_from_this(),
                ctr_id,
                decl,
                is_active()
            );

            return instance_pool_->put_new_instance(ctr_id, std::move(instance));
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Container instance {} is already registered", ctr_id).do_throw();
        }
    }

    CtrSharedPtr<CtrReferenceable<ApiProfileT>> find_ctr_instance(
        const CtrID& ctr_id,
        SharedBlockConstPtr root
    )
    {
        auto ptr = instance_pool_->get(ctr_id, this->self_ptr());
        if (ptr) {
            auto ctr_hash = root->ctr_type_hash();
            auto instance_hash = ptr->type_hash();

            if (instance_hash == ctr_hash) {
                return ptr;
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
            auto ctr_hash = root->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            auto instance = ctr_intf->create_ctr_instance(this->self_ptr(), root, is_active() || remove_cascade_);

            return instance_pool_->put_new_instance(ctr_id, std::move(instance));
        }
    }

    void on_ctr_drop(const CtrID& ctr_id) {
        instance_pool_->remove(ctr_id);
    }

    virtual ObjectPools& object_pools() const  {
        return object_pools_;
    }

    virtual SnpSharedPtr<ProfileStoreType<Profile>> self_ptr()  {
        return this->shared_from_this();
    }

    PairPtr& pair()  {
        return pair_;
    }

    const PairPtr& pair() const  {
        return pair_;
    }

    const CtrID& uuid() const  {
        return history_node_->snapshot_id();
    }

    bool is_active() const  {
        return history_node_->is_active();
    }

    bool is_data_locked() const  {
    	return history_node_->is_data_locked();
    }

    virtual bool isActive() const  {
        return is_active();
    }

    bool is_marked_to_clear() const  {
        return history_node_->is_dropped();
    }

    bool is_committed() const  {
        return history_node_->is_committed();
    }

    std::vector<CtrID> container_names() const
    {
        std::vector<CtrID> names;

        root_map_->for_each([&](auto ctr_name, auto root_id){
          names.push_back(ctr_name.value_t());
        });

        return std::move(names);
    }

    std::vector<U8String> container_names_str() const
    {
        std::vector<U8String> names;

        root_map_->for_each([&](auto ctr_name, auto root_id){
          std::stringstream ss;
          ss << ctr_name;

          names.push_back(U8String(ss.str()));
        });

        return std::move(names);
    }


    bool drop_ctr(const CtrID& ctr_id)
    {
        check_updates_allowed();

        auto ctr = find(ctr_id);
        if (ctr) {
            ctr->drop();
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



    void import_new_ctr_from(ROStoreSnapshotPtr ptr, const CtrID& name)
    {
        check_updates_allowed();

        SnapshotPtr txn = memoria_dynamic_pointer_cast<MyType>(ptr);

        txn->checkIfExportAllowed();
        auto root_id = this->getRootID(name);

    	auto txn_id = snaphsot_Id();

        if (root_id.is_null())
    	{
            auto root_id = txn->getRootID(name);

            if (root_id.is_set())
            {
                root_map_->upsert_key(name, root_id);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->snaphsot_Id()).do_throw();
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
                root_map_->upsert_key(name, root_id);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->snaphsot_Id()).do_throw();
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

    	auto txn_id = snaphsot_Id();

    	if (root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&](const BlockID&, const BlockID&, const void* block_data){
                return clone_foreign_block(ptr_cast<const BlockType>(block_data));
            });

            auto root_id1 = txn->getRootID(name);

            if (root_id1.is_set()) {
                root_map_->upsert_key(name, root_id1);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Unexpected empty root ID for container {} in snapshot {}", name, txn->snaphsot_Id()).do_throw();
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

    	auto txn_id = snaphsot_Id();

        if (!root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&, this](const BlockID& uid, const BlockID& id, const void* block_data) {
                auto block = this->getBlock(id);
                if (block && block->uid() == uid)
                {
                    return;
                }

                return clone_foreign_block(ptr_cast<const BlockType>(block_data));
            });

            auto root_id1 = txn->getRootID(name);
            if (root_id1.is_set())
            {
                root_map_->upsert_key(name, root_id1);
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
            Shared* shared = shared_pool_.construct(id, block);
            shared->set_store(this);
            shared->set_mutable(block->snapshot_id() == uuid());
            block->ref_block();
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

    virtual void traverse_ctr(
            const BlockID& block_id,
            BTreeTraverseNodeHandler<Profile>& node_handler
    )
    {
        if (!block_id.is_null())
        {
            if (node_handler.proceed_with(block_id))
            {
                const BlockType* blk = value_cast<const BlockType*>(detail::IDValueHolderH<BlockID>::from_id(block_id));

                node_handler.process_node(blk);

                auto blk_intf = ProfileMetadata<Profile>::local()
                        ->get_block_operations(blk->ctr_type_hash(), blk->block_type_hash());

                blk_intf->for_each_child(blk, [&](const BlockID& child_id){
                    this->traverse_ctr(child_id, node_handler);
                });
            }
        }
    }


public:
    bool has_open_containers() {
        return instance_pool_->count_open();
    }

    void dump_open_containers()
    {
        instance_pool_->for_each_open_ctr(this->self_ptr(), [](auto ctr_id, auto ii){
            println("{} -- {}", ctr_id, ii->describe_type());
        });
    }

    void flush_open_containers()
    {
        instance_pool_->for_each_open_ctr(this->self_ptr(), [](auto ctr_id, auto ii){
            ii->flush();
        });
    }

    void dump_dictionary_blocks()
    {
      auto ii = root_map_->first_entry();
      while (is_valid_chunk(ii))
      {
        ii->dump();
        ii = ii->next_chunk();
      }
    }

    virtual void ref_block(const BlockID& block_id)
    {
        auto block = getBlock(block_id);
        block->ref_block(1);
    }


    virtual void unref_block(const BlockID& block_id) {
      auto block = getBlock(block_id);
      block->unref_block();
    }


    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID&)
    {
        check_updates_allowed();

        if (initial_size == -1) {
            initial_size = DEFAULT_BLOCK_SIZE;
        }

        auto id = newId();

        auto buf = allocate_system<uint8_t>(initial_size);
        memset(buf.get(), 0, initial_size);

        BlockType* p = new (buf.get()) BlockType(id);
        p->id() = detail::IDValueHolderH<BlockID>::to_id(p);
        p->snapshot_id() = uuid();

        p->set_memory_block_size(initial_size);
        p->set_references(1);

        Shared* shared = shared_pool_.construct(p->id(), p);
        shared->set_store(this);
        shared->set_mutable(true);
        shared->set_orphan(true);

        buf.release();

        return SharedBlockPtr{shared};
    }


    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID&)
    {
        check_updates_allowed();

        auto new_block = this->clone_block(block.block());

        Shared* shared = shared_pool_.construct(new_block->id(), new_block.get());
        shared->set_store(this);
        shared->set_mutable(true);
        shared->set_orphan(true);

        new_block.release();

        return SharedBlockPtr{shared};
    }



    virtual BlockID newId()  {
        return BlockID{history_tree_raw_->newBlockId()};
    }

    virtual CtrID snaphsot_Id() const {
        return history_node_->snapshot_id();
    }

    // memory pool allocator
    virtual void* allocateMemory(size_t size) {
        return allocate_system<uint8_t>(size).release();
    }

    virtual void freeMemory(void* ptr) noexcept {
        free_system(ptr);
    }

    virtual BlockID getRootID(const CtrID& name)
    {
        if (!name.is_null())
        {
            auto iter = root_map_->find(name);

            if (iter->is_found(name)) {
                return iter->current_value();
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
                if (prev_id) {
                    unref_block(prev_id.get());
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
                    unref_block(prev_id.get());
                }
            }
            else {
                auto root_id_to_remove = history_node_->update_directory_root(root);
                if (root_id_to_remove.isSet())
                {
                    unref_block(root_id_to_remove);
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
            auto iter = root_map_->find(name);
            return iter->is_found(name);
        }
        else {
            return history_node_->root_id().is_null();
        }
    }

    virtual CtrID createCtrName() {
        return ProfileTraits<Profile>::make_random_ctr_id();
    }


    virtual void check(const CheckResultConsumerFn& consumer)
    {
      root_map_->for_each([&](auto ctr_name, auto root_id){
        auto block = this->getBlock(root_id);

        auto ctr_intf = ProfileMetadata<Profile>::local()
                ->get_container_operations(block->ctr_type_hash());

        ctr_intf->check(ctr_name, this->shared_from_this(), consumer);
      });
    }

    void check_storage(SharedBlockConstPtr block, const CheckResultConsumerFn& consumer) {}

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

        root_map_->for_each([&](auto ctr_name, auto root_id){
          auto block = this->getBlock(root_id);

          auto ctr_hash   = block->ctr_type_hash();
          auto ctr_intf   = ProfileMetadata<Profile>::local()
                  ->get_container_operations(ctr_hash);

          ctr_intf->walk(ctr_name, this->shared_from_this(), walker);
        });

        walker->endSnapshot();
    }



    virtual void dump_persistent_tree() {
    }


    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const hermes::Datatype& decl, const CtrID& ctr_id)
    {
        checkIfConainersCreationAllowed();
        return this->create_ctr_instance(decl, ctr_id);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const hermes::Datatype& decl)
    {
        checkIfConainersCreationAllowed();
        auto ctr_id = this->createCtrName();
        return this->create_ctr_instance(decl, ctr_id);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id)
    {
        checkIfConainersOpeneingAllowed();

        auto root_id = getRootID(ctr_id);
        if (root_id.is_set())
        {
            auto block = this->getBlock(root_id);
            return find_ctr_instance(ctr_id, block);
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

            return find_ctr_instance(ctr_id, block);
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
        detail::BlockSet visited_blocks;

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

    SharedPtr<SnapshotMemoryStat<ApiProfileT>> do_compute_memory_stat(detail::BlockSet& visited_blocks)
    {
        SnapshotStatsCountingConsumer<SnapshotBase> consumer(visited_blocks, this);

//        persistent_tree_.conditional_tree_traverse(consumer);

        return consumer.finish();
    }


    UniquePtr<BlockType> clone_block(const BlockType* block)
    {
        auto new_block = allocate_block_of_size<BlockType>(block->memory_block_size());

        CopyByteBuffer(block, new_block.get(), block->memory_block_size());

        auto new_block_id = newId();
        new_block->uid() = new_block_id;
        new_block->id() = detail::IDValueHolderH<BlockID>::to_id(new_block.get());
        new_block->snapshot_id() = uuid();
        new_block->set_references(1);

        return new_block;
    }



    void* malloc(size_t size) {
        return allocate_system<uint8_t>(size).release();
    }



    void checkIfConainersOpeneingAllowed(){}

    void checkIfConainersCreationAllowed()
    {
        if (!is_active()) {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot's {} data is not active, snapshot status = {}", uuid(), (int32_t)history_node_->status()).do_throw();
    	}
    }


    void checkIfExportAllowed()
    {
    	if (history_node_->is_dropped())
    	{
            // Double checking. This shouldn't happen
            if (!history_node_->root_id().isSet()) {
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



    void do_drop()
    {
        snapshot_removal_ = true;

        if (history_tree_raw_->is_dump_snapshot_lifecycle()) {
            std::cout << "MEMORIA: DROP snapshot's DATA: " << history_node_->snapshot_id() << std::endl;
        }

        unref_block(history_node_->root_id());
        history_node_->reset_root_id();
    }

    virtual void releaseBlock(Shared* block) noexcept
    {
      if (block->get()->unref_block())
      {
        auto blk = block->get();

        auto blk_intf = ProfileMetadata<Profile>::local()
                ->get_block_operations(blk->ctr_type_hash(), blk->block_type_hash());

        blk_intf->for_each_child(blk, [&](const BlockID& child_id){
          this->unref_block(child_id);
        });

        ::free(blk);
      }

      shared_pool_.destroy(block);
    }

    virtual void updateBlock(Shared* block) {
        //make_generic_error("updateBlock is not implemented!!!").do_throw();
    }

    void clone_foreign_block(const BlockType* foreign_block)
    {
    }

    void start_no_reentry(const CtrID& ctr_id) {}
    void finish_no_reentry(const CtrID& ctr_id) noexcept {}
};

}}}
