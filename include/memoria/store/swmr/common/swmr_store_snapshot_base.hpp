
// Copyright 2020-2021 Victor Smirnov
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
#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <memoria/store/swmr/common/swmr_store_snapshot_descriptor.hpp>
#include <memoria/store/swmr/common/swmr_store_counters.hpp>

#include <memoria/store/swmr/common/swmr_store_datatypes.hpp>
#include <memoria/store/swmr/common/swmr_store_smart_ptr.hpp>
#include <memoria/store/swmr/common/lite_allocation_map.hpp>
#include <memoria/store/swmr/common/allocation_pool.hpp>

#include <memoria/core/tools/uid_256.hpp>

namespace memoria {

// {1|942dcb76868c1d3cada1947784ca9146995aa486d7965fddc153a57846a6cf}
constexpr UID256 DirectoryCtrID = UID256(14110279458271056457ull, 7212985705449462490ull, 15993805639017735577ull, 143106268169123100ull);

// {1|4b9352517f5be15025dcd68b7d490ed14a55d1675218d872b489a6c5a4c82f}
constexpr UID256 AllocationMapCtrID = UID256(368932292307270068ull, 2152884276116180306ull, 2850076137090799012ull, 140328789407864907ull);

// {1|cd18ea71da4c4ff035824a9fabcf3cd9f028eeacb6226b765411158ad17f55}
constexpr UID256 HistoryCtrID = UID256(1149760052592017884ull, 11368207764395665491ull, 7473198478029390351ull, 96254673808331077ull);

// {1|30e0ee4a7012fde4fc983c6d4d8aeb95bf63183fa51816298b582da10c0803}
constexpr UID256 BlockMapCtrID = UID256(5683297571480407555ull, 6466791747040283087ull, 10547854029910783739ull, 85709955492119992ull);


template <typename Profile>
struct SnapshotCheckState {
    SWMRBlockCounters<Profile> counters;
};


template <typename Profile>
struct ReferenceCounterDelegate {
    using BlockID = ProfileBlockID<Profile>;

    virtual ~ReferenceCounterDelegate() noexcept = default;

    virtual void ref_block(const BlockID& block_id) = 0;
    virtual void unref_block(const BlockID& block_id, const std::function<void ()>& on_zero) = 0;
    virtual Optional<int64_t> count_refs(const BlockID& block_id) = 0;
};

template <typename Profile> class SWMRStoreBase;

template <typename Profile>
class SWMRStoreSnapshotBase:
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

    using Store = SWMRStoreBase<Profile>;

    using ApiProfileT = ApiProfile<Profile>;

    using BlockCounterCallbackFn = std::function<bool (const BlockID&, const BlockID&)>;
    using GraphVisitor = SWMRStoreGraphVisitor<ApiProfileT>;
    using VisitedBlocks = std::unordered_set<BlockID>;
    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;

    using SnapshotMetadataT = SWMRSnapshotMetadata<ApiProfileT>;
    using SequenceID = uint64_t;

    using CounterStorageT = CounterStorage<Profile>;

    using SnapshotDescriptorT       = SnapshotDescriptor<Profile>;
    using CDescrPtr                 = typename SnapshotDescriptorT::SharedPtrT;

    using CtrReferenceableResult    = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;
    using StoreT                    = Base;
    using Superblock                = SWMRSuperblock<Profile>;
    using CtrSizeT                  = ProfileCtrSizeT<Profile>;

    using DirectoryCtrType  = Map<CtrID, BlockID>;
    using DirectoryCtr      = ICtrApi<DirectoryCtrType, ApiProfileT>;

    using AllocationMapCtrType = AllocationMap;
    using AllocationMapCtr  = ICtrApi<AllocationMapCtrType, ApiProfileT>;

    using HistoryCtrType    = Map<SnapshotID, SnapshotMetadataDT<DataTypeFromProfile<ApiProfileT>>>;
    using HistoryCtr        = ICtrApi<HistoryCtrType, ApiProfileT>;

    static constexpr int32_t BASIC_BLOCK_SIZE            = Store::BASIC_BLOCK_SIZE;
    static constexpr int32_t SUPERBLOCK_SIZE             = BASIC_BLOCK_SIZE;
    static constexpr size_t  HEADER_SIZE                 = Store::HEADER_SIZE;
    static constexpr int32_t ALLOCATION_MAP_LEVELS       = Store::ALLOCATION_MAP_LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP    = Store::ALLOCATION_MAP_SIZE_STEP;
    static constexpr int32_t SUPERBLOCK_ALLOCATION_LEVEL = Log2(SUPERBLOCK_SIZE / BASIC_BLOCK_SIZE) - 1;
    static constexpr int32_t SUPERBLOCKS_RESERVED        = HEADER_SIZE / BASIC_BLOCK_SIZE;

    static constexpr size_t MAX_BLOCK_SIZE               = (1ull << (ALLOCATION_MAP_LEVELS - 1)) * BASIC_BLOCK_SIZE;

    SharedPtr<Store> store_;

    mutable ObjectPools object_pools_;
    mutable std::shared_ptr<CtrInstancePool<Profile>> instance_pool_;

    CDescrPtr snapshot_descriptor_;

    CtrSharedPtr<DirectoryCtr>      directory_ctr_;
    CtrSharedPtr<AllocationMapCtr>  allocation_map_ctr_;
    CtrSharedPtr<HistoryCtr>        history_ctr_;

    ReferenceCounterDelegate<Profile>* refcounter_delegate_;

    LDDocumentView metadata_;

    bool writable_{false};

public:
    using Base::getBlock;

    SWMRStoreSnapshotBase(
            SharedPtr<Store> store,
            CDescrPtr& snapshot_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate
    ) :
        store_(store),
        snapshot_descriptor_(snapshot_descriptor),
        refcounter_delegate_(refcounter_delegate)
    {
        instance_pool_ = std::make_shared<CtrInstancePool<Profile>>();

        if (MMA_UNLIKELY(!snapshot_descriptor_)) {
            MEMORIA_MAKE_GENERIC_ERROR("snapshot_descriptor is null").do_throw();
        }
    }

    static void init_profile_metadata()  {
        DirectoryCtr::template init_profile_metadata<Profile>();
        AllocationMapCtr::template init_profile_metadata<Profile>();
    }

    CtrInstancePool<Profile>& instance_pool() const {
        return *instance_pool_;
    }


    CtrSharedPtr<CtrReferenceable<ApiProfileT>> create_ctr_instance(
        const LDTypeDeclarationView& decl, const CtrID& ctr_id
    )
    {
        auto& instance_pool = this->instance_pool();

        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());

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

    virtual LDDocumentView metadata() {
        return metadata_;
    }

    virtual bool is_allocated(const BlockID& block_id)
    {
        init_allocator_ctr();

        auto alc = get_allocation_metadata(block_id);

        return allocation_map_ctr_->check_allocated(alc);
    }

    virtual SharedSBPtr<Superblock> get_superblock(uint64_t pos) = 0;

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const LDTypeDeclarationView& decl, const CtrID& ctr_id
    ) = 0;

    virtual AllocationMetadataT get_allocation_metadata(const BlockID& block_id) = 0;

    virtual void check_storage_specific(
            SharedBlockConstPtr block,
            const CheckResultConsumerFn& consumer
    ) = 0;

    virtual void register_allocation(
            const AllocationMetadataT& alc
    ) = 0;


    SharedSBPtr<Superblock> get_superblock() {
        return get_superblock(snapshot_descriptor_->superblock_ptr());
    }

    SharedSBPtr<Superblock> get_superblock() const {
        return const_cast<SWMRStoreSnapshotBase*>(this)->get_superblock(snapshot_descriptor_->superblock_ptr());
    }

    SequenceID sequence_id()  {
        return get_superblock()->sequence_id();
    }

    CtrSizeT allocation_map_size() {
        init_allocator_ctr();
        return allocation_map_ctr_->size();
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
        else if (MMA_UNLIKELY(ctr_id == HistoryCtrID))
        {
            return sb->history_root_id();
        }
        else if (MMA_UNLIKELY(ctr_id == BlockMapCtrID))
        {
            return sb->blockmap_root_id();
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
        else if (MMA_UNLIKELY(ctr_id == HistoryCtrID))
        {
            return sb->history_root_id().is_set();
        }
        else if (MMA_UNLIKELY(ctr_id == BlockMapCtrID))
        {
            return sb->blockmap_root_id().is_set();
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

    virtual void removeBlock(const BlockID& id) {
        MEMORIA_MAKE_GENERIC_ERROR("removeBlock() is not implemented for ReadOnly snapshots").do_throw();
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
        if (ctr_id == HistoryCtrID) {
            root_id = sb->history_root_id();
        }
        else if (ctr_id == DirectoryCtrID) {
            root_id = sb->directory_root_id();
        }
        else if (ctr_id == AllocationMapCtrID) {
            root_id = sb->allocator_root_id();
        }
        else if (ctr_id == BlockMapCtrID) {
            root_id = sb->blockmap_root_id();
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

        init_history_ctr();

        history_ctr_->check(consumer);

        init_allocator_ctr();

        allocation_map_ctr_->check(consumer);
        check_allocation_pool(consumer);

        directory_ctr_->for_each([&](auto ctr_name, auto block_id){
          auto block = this->getBlock(block_id);

          auto ctr_intf = ProfileMetadata<Profile>::local()
                  ->get_container_operations(block->ctr_type_hash());

          ctr_intf->check(ctr_name, this->self_ptr(), consumer);
        });
    }



    void check_storage(SharedBlockConstPtr block, const CheckResultConsumerFn& consumer)
    {
        init_allocator_ctr();
        check_storage_specific(block, consumer);

        AllocationMetadataT alc = get_allocation_metadata(block->id());
        Optional<AllocationMapEntryStatus> status = allocation_map_ctr_->get_allocation_status(alc.level(), alc.position());

        if (status.is_initialized())
        {
            if (status.get() == AllocationMapEntryStatus::FREE) {
                consumer(
                    CheckSeverity::ERROR,
                    make_string_document("Missing allocation info for {}", alc));
            }
        }
        else {
            consumer(
                CheckSeverity::ERROR,
                make_string_document("Missing allocation info for {}", alc));
        }

        register_allocation(alc);
    }

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
          names.push_back(ctr_name);
        });
        return std::move(names);
    }

    virtual void drop() {
        MEMORIA_MAKE_GENERIC_ERROR("drop() is not supported for SWMRStore snapshots").do_throw();
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
        if (refcounter_delegate_) {
            refcounter_delegate_->ref_block(block_id);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("ref_block() is not implemented for ReadOnly commits").do_throw();
        }
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

    class RebuildRefcountersHandler: public BTreeTraverseNodeHandler<Profile> {
        using Base = BTreeTraverseNodeHandler<Profile>;
        using typename Base::BlockType;
        //using BlockID   = ProfileBlockID<Profile>;

        SWMRBlockCounters<Profile>& counters_;
    public:
        RebuildRefcountersHandler(SWMRBlockCounters<Profile>& counters):
            counters_(counters)
        {}

        virtual void process_node(const BlockType* block) {}

        virtual bool proceed_with(const BlockID& block_id) const {
            return counters_.inc(block_id);
        }
    };


    virtual void build_block_refcounters(SWMRBlockCounters<Profile>& counters)
    {
        RebuildRefcountersHandler handler(counters);

        auto sb = get_superblock();

        traverse_ctr(sb->directory_root_id(), handler);

        traverse_ctr(sb->history_root_id(), handler);
        traverse_ctr(sb->allocator_root_id(), handler);

        if (get_superblock()->blockmap_root_id().is_set()) {
            traverse_ctr(sb->blockmap_root_id(), handler);
        }
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
        visitor.start_block(block, updated, store_->count_blocks(block_id));

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

        LDDocument doc = TypeSignature::parse(signature.to_std_string());
        LDTypeDeclarationView decl = doc.value().as_type_decl();

        auto ctr_ref = internal_create_by_name(decl, ctr_id);

        return memoria_static_pointer_cast<ICtrApi<CtrName, ApiProfileT>>(std::move(ctr_ref));
    }

    void set_superblock(Superblock* superblock)  {
        this->superblock_ = superblock;
    }

    void for_each_history_entry(const std::function<void (const SnapshotID&, const SWMRSnapshotMetadata<ApiProfileT>&)>& fn)
    {
        init_history_ctr();
        history_ctr_->for_each([&](auto snp_id, auto snp_metadata){
          fn(snp_id, snp_metadata);
        });
    }

    void init_allocator_ctr()
    {
        if (!allocation_map_ctr_)
        {
            auto root_block_id = get_superblock()->allocator_root_id();
            if (root_block_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<AllocationMapCtrType>(root_block_id);

                allocation_map_ctr_ = ctr_ref;
                allocation_map_ctr_->internal_detouch_from_store();
            }
        }
    }


    void init_history_ctr()
    {
        if (!history_ctr_)
        {
            auto root_block_id = get_superblock()->history_root_id();
            if (root_block_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<HistoryCtrType>(root_block_id);
                history_ctr_ = ctr_ref;
                history_ctr_->internal_detouch_from_store();
            }
        }
    }

    Optional<int64_t> find_root(const SnapshotID& snapshot_id) {
        init_history_ctr();
        auto iter = history_ctr_->find(snapshot_id);

        if (iter->is_found(snapshot_id)) {
            return iter->value().view();
        }

        return Optional<int64_t>{};
    }

    virtual void describe_to_cout()  {
        if (allocation_map_ctr_) {
            auto ii = allocation_map_ctr_->iterator();
            return ii->dump();
        }
    }

    void for_each_root_block(const std::function<void (int64_t)>& fn) const
    {
        init_history_ctr();

        auto scanner = history_ctr_->scanner_from(history_ctr_->iterator());

        bool has_next;
        do {

            for (auto superblock_ptr: scanner.values())
            {
                fn(superblock_ptr);
            }

            auto has_next_res = scanner.next_leaf();
            has_next = has_next_res;
        }
        while (has_next);
    }

    int32_t allocation_level(size_t size) const  {
        return CustomLog2(size / BASIC_BLOCK_SIZE);
    }

    static constexpr int32_t CustomLog2(int32_t value)  {
        return 31 - CtLZ((uint32_t)value);
    }

    struct ResolvedBlock {
        uint64_t file_pos;
        SharedBlockConstPtr block;
    };

    virtual ResolvedBlock resolve_block(const BlockID& block_id) = 0;
    virtual AllocationMetadataT resolve_block_allocation(const BlockID& block_id) = 0;

    SharedBlockConstPtr getBlock(const BlockID& block_id) {
        return resolve_block(block_id).block;
    }

    void for_each_history_entry_batch(const std::function<void (Span<const SnapshotID>, Span<const SnapshotMetadataT>)>& fn)
    {
        init_history_ctr();
        history_ctr_->for_each_chunk([&](auto snp_id_span, auto snp_metadata_span){
          fn(snp_id_span, snp_metadata_span);
        });
    }

    bool find_unallocated(int32_t level, ProfileCtrSizeT<Profile> size, ArenaBuffer<AllocationMetadata<ApiProfileT>>& arena)
    {
        init_allocator_ctr();
        return allocation_map_ctr_->find_unallocated(0, level, size, arena);
    }

    void scan_unallocated(const std::function<bool (Span<AllocationMetadataT>)>& fn) {
        init_allocator_ctr();
        return allocation_map_ctr_->scan(fn);
    }

    void check_allocation_pool(const CheckResultConsumerFn& consumer)
    {
        auto sb = get_superblock();
        AllocationPool<ApiProfileT, ALLOCATION_MAP_LEVELS> pool;
        pool.load(sb->allocation_pool_data());

        init_allocator_ctr();
        pool.for_each([&](const AllocationMetadataT& meta) {
            if (!allocation_map_ctr_->check_allocated(meta))
            {
                consumer(
                    CheckSeverity::ERROR,
                    make_string_document("AllocationMap entry check failure: {}", meta)
                );
            }
        });
    }

    void add_superblock(LiteAllocationMap<ApiProfileT>& allocations)
    {
        uint64_t sb_ptr = snapshot_descriptor_->superblock_ptr() / BASIC_BLOCK_SIZE;
        allocations.append(AllocationMetadataT((CtrSizeT)sb_ptr, 1, SUPERBLOCK_ALLOCATION_LEVEL));
    }

    void add_system_blocks(LiteAllocationMap<ApiProfileT>& allocations)
    {
        auto sb = get_superblock();
        allocations.append(AllocationMetadataT{0, 2, SUPERBLOCK_ALLOCATION_LEVEL});

        allocations.append(AllocationMetadataT{
            (CtrSizeT)sb->global_block_counters_file_pos() / BASIC_BLOCK_SIZE,
            (CtrSizeT)sb->global_block_counters_blocks(),
            0
        });

        AllocationPool<ApiProfileT, ALLOCATION_MAP_LEVELS> pool;
        pool.load(sb->allocation_pool_data());

        pool.for_each([&](const AllocationMetadataT& alc){
            allocations.append(alc);
        });
    }

    void dump_allocation_map(int64_t num = -1) {
        init_allocator_ctr();
        allocation_map_ctr_->dump_leafs(num);
    }

    void check_allocation_map(
        const LiteAllocationMap<ApiProfileT>& allocations,
        const CheckResultConsumerFn& consumer
    )
    {
        init_allocator_ctr();

        allocation_map_ctr_->compare_with(allocations.ctr(), [&](auto& helper){
            consumer(CheckSeverity::ERROR, make_string_document(
                         "Allocation map mismatch at {} :: {}/{} :: {}, level {}. Expected: {}, actual: {}",
                         helper.my_base(),
                         helper.my_idx(),
                         helper.other_base(),
                         helper.other_idx(),
                         helper.level(),
                         helper.other_bit(), helper.my_bit()
            ));

            return true; // continue
        });
    }
};

}
