
// Copyright 2020-2025 Victor Smirnov
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

#include <memoria/api/store/swmr_store_api.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/core/container/ctr_impl.hpp>
#include <memoria/core/container/ctr_instance_pool.hpp>



#include <memoria/api/map/map_api.hpp>
#include <memoria/api/vector/vector_api.hpp>
#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <memoria/store/oltp/oltp_store_snapshot_descriptor.hpp>

#include <memoria/store/swmr/common/swmr_store_snapshot_base.hpp>
#include <memoria/store/swmr/common/lite_allocation_map.hpp>
#include <memoria/store/swmr/common/allocation_pool.hpp>

#include <memoria/core/tools/uid_256.hpp>

#include <memoria/store/oltp/oltp_store_snapshot_base.hpp>

#include <memoria/store/oltp/oltp_superblock.hpp>

#include <memoria/api/io/block_level.hpp>

namespace memoria {

template <typename Profile> class OLTPStoreBase;

// {1|8e66ccf646abbba916719c120bd576ab81b79d9b36d3f8bf59864323fcae6a}
constexpr UID256 EvcQueueCtrID = {11149710243020957416ull, 13431807424718247777ull, 18126774523718630168ull, 119040615793322133ull};


template <typename Profile>
class OLTPStoreSnapshotBase:
        public ProfileStoreType<Profile>,
        public ISWMRStoreReadOnlySnapshot<ApiProfile<Profile>>
{
    using Base = ProfileStoreType<Profile>;
protected:

    using typename Base::BlockType;
    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockID;
    using typename Base::SnapshotID;
    using typename Base::CtrID;
    using typename Base::Shared;

    using BlockIDValueHolder = typename BlockID::ValueHolder;

    using Store = OLTPStoreBase<Profile>;
    using ApiProfileT = ApiProfile<Profile>;

    using GraphVisitor           = SWMRStoreGraphVisitor<ApiProfileT>;
    using VisitedBlocks          = std::unordered_set<BlockID>;
    using AllocationMetadataT    = AllocationMetadata<ApiProfileT>;

    using SequenceID = uint64_t;

    using SnapshotDescriptorT       = OLTPSnapshotDescriptor<Profile>;
    using CDescrPtr                 = typename SnapshotDescriptorT::SharedPtrT;

    using CtrReferenceableResult    = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;
    using StoreT                    = Base;
    using Superblock                = OLTPSuperblock<Profile>;
    using CtrSizeT                  = ProfileCtrSizeT<Profile>;

    using DirectoryCtrType  = Map<CtrID, BlockID>;
    using DirectoryCtr      = ICtrApi<DirectoryCtrType, ApiProfileT>;



    using EvcQueueCtrType   = Vector<UBigInt>;
    using EvcQueueCtr       = ICtrApi<EvcQueueCtrType, ApiProfileT>;

    static constexpr int32_t BASIC_BLOCK_SIZE            = Store::BASIC_BLOCK_SIZE;
    static constexpr int32_t SUPERBLOCK_SIZE             = BASIC_BLOCK_SIZE;
    static constexpr size_t  HEADER_SIZE                 = Store::HEADER_SIZE;


    SharedPtr<Store> store_;

    mutable ObjectPools object_pools_;
    mutable std::shared_ptr<CtrInstancePool<Profile>> instance_pool_;

    CDescrPtr snapshot_descriptor_;

    CtrSharedPtr<DirectoryCtr>      directory_ctr_;    
    CtrSharedPtr<EvcQueueCtr>       evc_queue_ctr_;

    hermes::HermesCtr metadata_;

    bool writable_{false};

    std::shared_ptr<io::BlockIOProvider> block_provider_;

public:
    using Base::getBlock;

    OLTPStoreSnapshotBase(
            SharedPtr<Store> store,
            CDescrPtr& snapshot_descriptor
    ) :
        store_(store),
        snapshot_descriptor_(snapshot_descriptor)
    {
        instance_pool_ = std::make_shared<CtrInstancePool<Profile>>();

        if (MMA_UNLIKELY(!snapshot_descriptor_)) {
            MEMORIA_MAKE_GENERIC_ERROR("snapshot_descriptor is null").do_throw();
        }
    }

    static void init_profile_metadata()  {
        DirectoryCtr::template init_profile_metadata<Profile>();
    }

    CtrInstancePool<Profile>& instance_pool() const {
        return *instance_pool_;
    }


    CtrSharedPtr<CtrReferenceable<ApiProfileT>> create_ctr_instance(
        const hermes::Datatype& decl, const CtrID& ctr_id
    )
    {
        auto& instance_pool = this->instance_pool();

        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_string());

        if (!instance_pool.contains(ctr_id))
        {
            auto instance = factory->create_ctr_instance(
                this->self_ptr(),
                ctr_id,
                decl,
                writable_
            );

            return instance_pool.put_new_instance(ctr_id, std::move(instance));
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
        auto& instance_pool = this->instance_pool();

        auto ptr = instance_pool.get(ctr_id, this->self_ptr());
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

            auto instance = ctr_intf->create_ctr_instance(this->self_ptr(), root, writable_);

            return instance_pool.put_new_instance(ctr_id, std::move(instance));
        }
    }

    void on_ctr_drop(const CtrID& ctr_id) {
        instance_pool_->remove(ctr_id);
    }

    virtual hermes::HermesCtr metadata() {
        return metadata_;
    }



    virtual io::BlockPtr<Superblock> get_superblock(uint64_t pos) = 0;

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const hermes::Datatype& decl, const CtrID& ctr_id
    ) = 0;

    virtual AllocationMetadataT get_allocation_metadata(const BlockID& block_id) = 0;

    virtual void check_storage(SharedBlockConstPtr block, const CheckResultConsumerFn& consumer) {}


    virtual void check_storage_specific(
            SharedBlockConstPtr block,
            const CheckResultConsumerFn& consumer
    ) = 0;

    virtual void register_allocation(
            const AllocationMetadataT& alc
    ) = 0;


    io::BlockPtr<Superblock> get_superblock() {
        return get_superblock(snapshot_descriptor_->superblock_id());
    }

    io::BlockPtr<Superblock> get_superblock() const {
        return const_cast<OLTPStoreSnapshotBase*>(this)->get_superblock(snapshot_descriptor_->superblock_id());
    }

    SequenceID sequence_id()  {
        return get_superblock()->sequence_id();
    }



    virtual ObjectPools& object_pools() const  {
        return object_pools_;
    }

    virtual BlockID getRootID(const CtrID& ctr_id)
    {
        auto sb = get_superblock();

        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            return sb->directory_root_id();
        }
        else if (MMA_UNLIKELY(ctr_id == AllocationMapCtrID))
        {
            return sb->allocator_root_id();
        }
        else if (MMA_UNLIKELY(ctr_id == EvcQueueCtrID))
        {
            return sb->evc_queue_root_id();
        }
        else if (directory_ctr_)
        {
            auto iter = directory_ctr_->find(ctr_id);

            if (iter->is_found(ctr_id))
            {
                return iter->current_value();
            }
        }

        return BlockID{};
    }



    virtual void setRoot(const CtrID& ctr_id, const BlockID& root)
    {
        MEMORIA_MAKE_GENERIC_ERROR("setRoot() is not implemented for ReadOnly commits").do_throw();
    }

    virtual bool hasRoot(const CtrID& ctr_id)
    {
        auto sb = get_superblock();

        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            return sb->directory_root_id().is_set();
        }
        else if (MMA_UNLIKELY(ctr_id == AllocationMapCtrID))
        {
            return sb->allocator_root_id().is_set();
        }
        else if (MMA_UNLIKELY(ctr_id == EvcQueueCtrID))
        {
            return sb->evc_queue_root_id().is_set();
        }
        else if (directory_ctr_)
        {
            auto iter = directory_ctr_->find(ctr_id);

            if (iter->is_found(ctr_id))
            {
                return true;
            }
        }

        return false;
    }

    virtual CtrID createCtrName() {
        return ProfileTraits<Profile>::make_random_ctr_id();
    }

    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID&) {
        MEMORIA_MAKE_GENERIC_ERROR("createBlock() is not implemented for ReadOnly snapshots").do_throw();
    }

    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID&) {
        MEMORIA_MAKE_GENERIC_ERROR("cloneBlock() is not implemented for ReadOnly snapshots").do_throw();
    }

    virtual BlockID newId() {
        MEMORIA_MAKE_GENERIC_ERROR("newId() is not implemented for ReadOnly snapshots").do_throw();
    }

    virtual SnapshotID snaphsot_Id() const  {
        return get_superblock()->snapshot_id();
    }

    // memory pool allocator
    virtual void* allocateMemory(size_t size) {
        return allocate_system<uint8_t>(size).release();
    }

    virtual void freeMemory(void* ptr) noexcept {
        free_system(ptr);
    }

    virtual bool isActive() const  {
        return false;
    }

    virtual void flush_open_containers() {
        MEMORIA_MAKE_GENERIC_ERROR("flush_open_containers() is not implemented for ReadOnly commits");
    }


    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id)
    {
        auto sb = get_superblock();

        BlockID root_id;
        if (MMA_UNLIKELY(ctr_id == EvcQueueCtrID)) {
            root_id = sb->evc_queue_root_id();
        }
        else if (MMA_UNLIKELY(ctr_id == DirectoryCtrID)) {
            root_id = sb->directory_root_id();
        }
        else if (MMA_UNLIKELY(ctr_id == AllocationMapCtrID)) {
            root_id = sb->allocator_root_id();
        }
        else {
            root_id = getRootID(ctr_id);
        }

        if (root_id.is_set())
        {
            auto block = this->getBlock(root_id);
            return this->find_ctr_instance(ctr_id, block);
        }

        return CtrSharedPtr<CtrReferenceable<ApiProfileT>>{};
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> from_root_id(
            const BlockID& root_block_id
    )
    {
        if (root_block_id.is_set())
        {
            auto block = this->getBlock(root_block_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            CtrID ctr_id = ctr_intf->get_ctr_id(block);
            return this->find_ctr_instance(ctr_id, block);
        }

        return CtrSharedPtr<CtrReferenceable<ApiProfileT>>{};
    }

    virtual void check(const CheckResultConsumerFn& consumer)
    {
        directory_ctr_->check(consumer);

        init_evc_queue_ctr();
        evc_queue_ctr_->check(consumer);

        check_allocation_map(consumer);


        directory_ctr_->for_each([&](auto ctr_name, auto block_id){
          auto block = this->getBlock(block_id);

          auto ctr_intf = ProfileMetadata<Profile>::local()
                  ->get_container_operations(block->ctr_type_hash());

          ctr_intf->check(ctr_name, this->self_ptr(), consumer);
        });
    }

    virtual void check_allocation_map(const CheckResultConsumerFn& consumer) {}




    virtual void walkContainers(
            ContainerWalker<Profile>* walker, const
            char* allocator_descr = nullptr
    ) {
        return walk_containers(walker, allocator_descr);
    }

    virtual void walk_containers(
            ContainerWalker<Profile>* walker, const
            char* allocator_descr = nullptr
    )
    {
        auto sb = get_superblock();

        if (allocator_descr != nullptr)
        {
            walker->beginSnapshot(
                        fmt::format("Snapshot-{} -- {}", sb->snapshot_id(), allocator_descr).data()
            );
        }
        else {
            walker->beginSnapshot(
                        fmt::format("Snapshot-{}", sb->snapshot_id()).data()
            );
        }

        directory_ctr_->for_each([&](auto ctr_name, auto root_id){
          auto block = getBlock(root_id);

          auto ctr_hash   = block->ctr_type_hash();
          auto ctr_intf   = ProfileMetadata<Profile>::local()
                  ->get_container_operations(ctr_hash);

          ctr_intf->walk(ctr_name, this->self_ptr(), walker);
        });

        walker->endSnapshot();
    }

    virtual bool drop_ctr(const CtrID& ctr_id) {
        MEMORIA_MAKE_GENERIC_ERROR("drop_ctr() is not implemented for ReadOnly commits").do_throw();
    }

    virtual U8String ctr_type_name(const CtrID& ctr_id)
    {
        auto root_id = this->getRootID(ctr_id);
        if (root_id.is_set())
        {
            auto block = getBlock(root_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ctr_intf->ctr_type_name();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Can't find container with id {}", ctr_id).do_throw();
        }
    }


    //=================================== R/O Commit Stuff ===========================

    virtual SnapshotID snapshot_id()  {
        return get_superblock()->snapshot_id();
    }

    virtual bool is_committed() const  {
        return true;
    }

    virtual bool is_active() const  {
        return false;
    }

    virtual bool is_marked_to_clear() const  {
        return false;
    }


    virtual void dump_open_containers()
    {
        instance_pool().for_each_open_ctr(this->self_ptr(), [](auto ctr_id, auto ctr){
            println("{} -- {}", ctr_id, ctr->describe_type());
        });
    }

    virtual bool has_open_containers() {
        return instance_pool().count_open() > 0;
    }

    virtual std::vector<CtrID> container_names() const
    {
        std::vector<CtrID> names;

        directory_ctr_->for_each([&](auto ctr_name, auto block_id){
          names.push_back(ctr_name.value_t());
        });
        return std::move(names);
    }

    virtual void drop() {
        MEMORIA_MAKE_GENERIC_ERROR("drop() is not supported for OLTPStore snapshots").do_throw();
    }


    virtual Optional<U8String> ctr_type_name_for(const CtrID& name)
    {
        auto root_id = getRootID(name);
        auto block = getBlock(root_id);

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            return ctr_intf->ctr_type_name();
        }

        return Optional<U8String>{};
    }

    virtual SnpSharedPtr<IStoreApiBase<ApiProfileT>> snapshot_ref_opening_allowed() {
        return SnpSharedPtr<IStoreApiBase<ApiProfileT>>{};
    }

    virtual void ref_block(const BlockID& block_id)
    {
        MEMORIA_MAKE_GENERIC_ERROR("uref_block() is not implemented for ReadOnly commits").do_throw();
    }

    virtual void unref_block(const BlockID& block_id)
    {
        MEMORIA_MAKE_GENERIC_ERROR("unref_block() is not implemented for ReadOnly commits").do_throw();
    }

    virtual void unref_ctr_root(const BlockID& root_block_id)
    {
        MEMORIA_MAKE_GENERIC_ERROR("unref_ctr_root() is not implemented for ReadOnly commits").do_throw();
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
                auto block = getBlock(block_id);

                node_handler.process_node(block.block());

                auto blk_intf = ProfileMetadata<Profile>::local()
                        ->get_block_operations(block->ctr_type_hash(), block->block_type_hash());

                blk_intf->for_each_child(block.block(), [&](const BlockID& child_id){
                    this->traverse_ctr(child_id, node_handler);
                });
            }
        }
    }

    virtual void check_updates_allowed() {
        MEMORIA_MAKE_GENERIC_ERROR("updated are not allowed for ReadOnly commits").do_throw();
    }

    bool contains_or_add(VisitedBlocks& vb, const BlockID& id)
    {
        if (!contains(vb, id)) {
            vb.insert(id);
            return false;
        }
        return true;
    }

    bool contains(VisitedBlocks& vb, const BlockID& id)
    {
        auto ii = vb.find(id);
        return ii != vb.end();
    }


    virtual void traverse_cow_containers(VisitedBlocks& vb, GraphVisitor& visitor)
    {
        auto sb = get_superblock();
        if (sb->directory_root_id().is_set())
        {
            traverse_ctr_cow_tree(sb->directory_root_id(), vb, visitor, GraphVisitor::CtrType::DIRECTORY);

            directory_ctr_->for_each([&](auto ctr_name, auto root_id){
                traverse_ctr_cow_tree(root_id, vb, visitor, GraphVisitor::CtrType::DATA);
            });
        }
    }


    void traverse_ctr_cow_tree(const BlockID& root_block_id, VisitedBlocks& vb, GraphVisitor& visitor, typename GraphVisitor::CtrType ctr_type)
    {
        bool updated = contains(vb, root_block_id);
        if (ctr_type != GraphVisitor::CtrType::DATA || !updated)
        {
            auto ref = from_root_id(root_block_id);

            visitor.start_ctr(ref, updated, ctr_type);

            auto root_block = ref->root_block();
            traverse_block_tree(root_block, vb, visitor);

            visitor.end_ctr();
        }
    }

    void traverse_block_tree(
            CtrBlockPtr<ApiProfileT> block,
            VisitedBlocks& vb,
            GraphVisitor& visitor)
    {
        BlockID block_id = cast_to<BlockID>(block->block_id());

        bool updated = contains_or_add(vb, block_id);
        visitor.start_block(block, updated, 0); //store_->count_blocks(block_id)

        if (!updated)
        {
            auto children = block->children();
            for (size_t c = 0; c < children.size(); c++) {
                traverse_block_tree(children[c], vb, visitor);
            }
        }

        visitor.end_block();
    }

    template<typename CtrName>
    CtrSharedPtr<ICtrApi<CtrName, ApiProfileT>> internal_find_by_root_typed(const BlockID& root_block_id)
    {
        auto ref = from_root_id(root_block_id);
        ref->internal_detouch_from_store();
        return memoria_static_pointer_cast<ICtrApi<CtrName, ApiProfileT>>(std::move(ref));
    }


    template<typename CtrName>
    CtrSharedPtr<ICtrApi<CtrName, ApiProfileT>> internal_create_by_name_typed(const CtrID& ctr_id)
    {
        U8String signature = make_datatype_signature(CtrName{}).name();

        auto doc = TypeSignature::parse(signature.to_std_string());
        auto decl = doc.root().value().as_datatype();

        auto ctr_ref = internal_create_by_name(decl, ctr_id);

        return memoria_static_pointer_cast<ICtrApi<CtrName, ApiProfileT>>(std::move(ctr_ref));
    }

    void set_superblock(Superblock* superblock)  {
        this->superblock_ = superblock;
    }

    void init_evc_queue_ctr()
    {
        if (!evc_queue_ctr_)
        {
            auto root_block_id = get_superblock()->evc_queue_root_id();
            if (root_block_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<EvcQueueCtrType>(root_block_id);
                evc_queue_ctr_ = ctr_ref;
                evc_queue_ctr_->internal_detouch_from_store();
            }
        }
    }



    virtual void describe_to_cout() {}



    int32_t allocation_level(size_t size) const  {
        return CustomLog2(size / BASIC_BLOCK_SIZE);
    }

    static constexpr int32_t CustomLog2(int32_t value)  {
        return 31 - CtLZ((uint32_t)value);
    }

    struct ResolvedBlock {
        io::DevSizeT file_pos;
        SharedBlockConstPtr block;
    };

    virtual ResolvedBlock resolve_block(const BlockID& block_id) = 0;
    virtual AllocationMetadataT resolve_block_allocation(const BlockID& block_id) = 0;

    SharedBlockConstPtr getBlock(const BlockID& block_id) {
        return resolve_block(block_id).block;
    }
};

}
