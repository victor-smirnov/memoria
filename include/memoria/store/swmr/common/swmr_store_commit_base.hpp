
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

#include <memoria/api/map/map_api.hpp>
#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <memoria/store/swmr/common/swmr_store_commit_descriptor.hpp>
#include <memoria/store/swmr/common/swmr_store_counters.hpp>

#include <memoria/store/swmr/common/swmr_store_datatypes.hpp>
#include <memoria/store/swmr/common/swmr_store_smart_ptr.hpp>

namespace memoria {

// cc2cd24f-6518-4977-81d3-dad21d4f45cc
constexpr UUID DirectoryCtrID = UUID(8595428187223239884ull, 14719257946640536449ull);

// a64d654b-ec9b-4ab7-870f-83816c8d0ce2
constexpr UUID AllocationMapCtrID = UUID(13207540296396918182ull, 16288549449461075847ull);

// 0bc70e1f-adaf-4454-afda-7f6ac7104790
constexpr UUID HistoryCtrID = UUID(6072171355687536395ull, 10396296713479379631ull);

// 177b946a-f700-421f-b5fc-0a177195b82f
constexpr UUID BlockMapCtrID = UUID(2252363826283707159ull, 3438662628447812789ull);


template <typename Profile>
struct CommitCheckState {
    SWMRBlockCounters<Profile> counters{"CheckState"};
};


template <typename Profile>
struct ReferenceCounterDelegate {
    using BlockID = ProfileBlockID<Profile>;

    virtual ~ReferenceCounterDelegate() noexcept = default;

    virtual void ref_block(const BlockID& block_id) = 0;
    virtual void unref_block(const BlockID& block_id, const std::function<void ()>& on_zero) = 0;
    virtual void unref_ctr_root(const BlockID& root_block_id) = 0;
};

template <typename Profile> class SWMRStoreBase;

template <typename Profile>
class SWMRStoreCommitBase:
        public ProfileStoreType<Profile>,
        public ISWMRStoreReadOnlyCommit<ApiProfile<Profile>>
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
    using ApiBlockID = ApiProfileBlockID<ApiProfileT>;

    using BlockCounterCallbackFn = std::function<bool (const BlockID&, const BlockID&)>;
    using GraphVisitor = SWMRStoreGraphVisitor<ApiProfileT>;
    using VisitedBlocks = std::unordered_set<BlockID>;
    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;


    using CommitID = typename ISWMRStoreCommitBase<ApiProfileT>::CommitID;
    using CommitMetadataT = CommitMetadata<ApiProfileT>;
    using SequenceID = uint64_t;

    using CounterStorageT = CounterStorage<Profile>;

    using CommitDescriptorT         = CommitDescriptor<Profile>;
    using CtrReferenceableResult    = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;
    using StoreT                    = Base;
    using Superblock                = SWMRSuperblock<Profile>;

    using CtrInstanceMap = std::unordered_map<CtrID, CtrReferenceable<ApiProfileT>*>;

    using DirectoryCtrType  = Map<CtrID, BlockID>;
    using DirectoryCtr      = ICtrApi<DirectoryCtrType, ApiProfileT>;

    using AllocationMapCtrType = AllocationMap;
    using AllocationMapCtr  = ICtrApi<AllocationMapCtrType, ApiProfileT>;

    using HistoryCtrType    = Map<UUID, CommitMetadataDT<DataTypeFromProfile<ApiProfileT>>>;
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

    CtrInstanceMap instance_map_;

    CommitDescriptorT* commit_descriptor_;

    Logger logger_;

    CtrSharedPtr<DirectoryCtr>      directory_ctr_;
    CtrSharedPtr<AllocationMapCtr>  allocation_map_ctr_;
    CtrSharedPtr<HistoryCtr>        history_ctr_;

    ReferenceCounterDelegate<Profile>* refcounter_delegate_;

    mutable ObjectPools object_pools_;

public:
    using Base::getBlock;

    SWMRStoreCommitBase(
            SharedPtr<Store> store,
            CommitDescriptorT* commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate
    ) noexcept:
        store_(store),
        commit_descriptor_(commit_descriptor),
        refcounter_delegate_(refcounter_delegate)
    {}

    static void init_profile_metadata() noexcept {
        DirectoryCtr::template init_profile_metadata<Profile>();
        AllocationMapCtr::template init_profile_metadata<Profile>();
    }

    virtual SharedSBPtr<Superblock> get_superblock(uint64_t pos) = 0;
    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> new_ctr_instance(
            ContainerOperationsPtr<Profile> ctr_intf,
            SharedBlockConstPtr block
    ) = 0;

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const LDTypeDeclarationView& decl, const CtrID& ctr_id
    ) = 0;

    SharedSBPtr<Superblock> get_superblock() {
        return get_superblock(commit_descriptor_->superblock_ptr());
    }

    SharedSBPtr<Superblock> get_superblock() const {
        return const_cast<SWMRStoreCommitBase*>(this)->get_superblock(commit_descriptor_->superblock_ptr());
    }

    SequenceID sequence_id() noexcept {
        return get_superblock()->sequence_id();
    }

    virtual ObjectPools& object_pools() const noexcept {
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
                return iter->value().view();
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
        MEMORIA_MAKE_GENERIC_ERROR("removeBlock() is not implemented for ReadOnly commits").do_throw();
    }

    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID&) {
        MEMORIA_MAKE_GENERIC_ERROR("createBlock() is not implemented for ReadOnly commits").do_throw();
    }

    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID&) {
        MEMORIA_MAKE_GENERIC_ERROR("cloneBlock() is not implemented for ReadOnly commits").do_throw();
    }

    virtual BlockID newId() {
        MEMORIA_MAKE_GENERIC_ERROR("newId() is not implemented for ReadOnly commits").do_throw();
    }

    virtual SnapshotID currentTxnId() const noexcept {
        return get_superblock()->commit_id();
    }

    // memory pool allocator
    virtual void* allocateMemory(size_t size) {
        return allocate_system<uint8_t>(size).release();
    }

    virtual void freeMemory(void* ptr) noexcept {
        free_system(ptr);
    }

    virtual Logger& logger() noexcept {
        return logger_;
    }

    virtual bool isActive() const noexcept {
        return false;
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
                return new_ctr_instance(ctr_intf, block);
            }
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
                return new_ctr_instance(ctr_intf, block);
            }
        }

        return CtrSharedPtr<CtrReferenceable<ApiProfileT>>{};
    }

    virtual bool check() {
        bool result = false;

        auto iter = directory_ctr_->iterator();

        while(!iter->is_end())
        {
            auto ctr_name = iter->key();

            auto block = this->getBlock(iter->value());

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            auto res = ctr_intf->check(ctr_name, this->self_ptr());

            result = res || result;

            iter->next();
        }

        return result;
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
                        fmt::format("Snapshot-{} -- {}", sb->commit_id(), allocator_descr).data()
            );
        }
        else {
            walker->beginSnapshot(
                        fmt::format("Snapshot-{}", sb->commit_id()).data()
            );
        }

        auto iter = directory_ctr_->iterator();
        while (!iter->is_end())
        {
            auto ctr_name   = iter->key();
            auto root_id    = iter->value();

            auto block = getBlock(root_id);

            auto ctr_hash   = block->ctr_type_hash();
            auto ctr_intf   = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            ctr_intf->walk(ctr_name, this->self_ptr(), walker);

            iter->next();
        }

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

    virtual CommitID commit_id() noexcept {
        return get_superblock()->commit_id();
    }

    virtual bool is_committed() const noexcept {
        return true;
    }

    virtual bool is_active() const noexcept {
        return false;
    }

    virtual bool is_marked_to_clear() const noexcept {
        return false;
    }


    virtual void dump_open_containers()
    {
        for (const auto& pair: instance_map_)
        {
            std::cout << pair.first << " -- " << pair.second->describe_type() << std::endl;
        }
    }

    virtual bool has_open_containers() {
        return instance_map_.size() > 0;
    }

    virtual std::vector<CtrID> container_names() const {
        std::vector<CtrID> names;

        auto ii = directory_ctr_->iterator();

        while (!ii->is_end())
        {
            names.push_back(ii->key());
            ii->next();
        }

        return std::move(names);
    }

    virtual void drop() {
        MEMORIA_MAKE_GENERIC_ERROR("drop() has to be implemented!!").do_throw();
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

    virtual void unref_block(const BlockID& block_id, std::function<void ()> on_zero)
    {
        if (refcounter_delegate_)
        {
            return refcounter_delegate_->unref_block(block_id, on_zero);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("unref_block() is not implemented for ReadOnly commits").do_throw();
        }
    }

    virtual void unref_ctr_root(const BlockID& root_block_id)
    {
        if (refcounter_delegate_)
        {
            return refcounter_delegate_->unref_ctr_root(root_block_id);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("unref_ctr_root() is not implemented for ReadOnly commits").do_throw();
        }
    }

    virtual void traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    )
    {
        auto instance = from_root_id(root_block);
        return instance->traverse_ctr(&node_handler);
    }

    virtual void check_updates_allowed() {
        MEMORIA_MAKE_GENERIC_ERROR("updated are not allowed for ReadOnly commits").do_throw();
    }

    virtual void check(std::function<void (LDDocument&)> callback) {
    }

    virtual void build_block_refcounters(SWMRBlockCounters<Profile>& counters)
    {
        auto counters_fn = [&](const BlockID& block_id, const BlockID& parent_id) {
            return counters.inc(block_id);
        };

        traverse_cow_containers(counters, counters_fn);

        auto sb = get_superblock();

        traverse_ctr_cow_tree(
            sb->history_root_id(),
            counters_fn
        );

        traverse_ctr_cow_tree(
            sb->allocator_root_id(),
            counters_fn
        );

        if (get_superblock()->blockmap_root_id().is_set()) {
            traverse_ctr_cow_tree(
                sb->blockmap_root_id(),
                counters_fn
            );
        }
    }




    virtual void traverse_cow_containers(SWMRBlockCounters<Profile>& counters, const BlockCounterCallbackFn& callback)
    {
        auto directory_root_id = get_superblock()->directory_root_id();
        traverse_ctr_cow_tree(directory_root_id, callback);

        auto iter = directory_ctr_->iterator();
        while (!iter->is_end())
        {
            auto root_id = iter->value();

            if (counters.add_root(root_id)){
                traverse_ctr_cow_tree(root_id, callback);
            }
            iter->next();
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

            auto iter = directory_ctr_->iterator();
            while (!iter->is_end())
            {
                auto root_id = iter->value();
                traverse_ctr_cow_tree(root_id, vb, visitor, GraphVisitor::CtrType::DATA);
                iter->next();
            }
        }
    }


    void traverse_ctr_cow_tree(const BlockID& root_block_id, const BlockCounterCallbackFn& callback)
    {
        auto ref = from_root_id(root_block_id);
        auto root_block = ref->root_block();
        return traverse_block_tree(root_block, BlockID{}, callback);
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
            const BlockID& parent_id,
            const BlockCounterCallbackFn& callback)
    {
        BlockID block_id;
        block_id_holder_to(block->block_id(), block_id);

        auto traverse = callback(block_id, parent_id);
        if (traverse)
        {
            auto children = block->children();
            for (size_t c = 0; c < children.size(); c++) {
                traverse_block_tree(children[c], block_id, callback);
            }
        }
    }

    void traverse_block_tree(
            CtrBlockPtr<ApiProfileT> block,
            VisitedBlocks& vb,
            GraphVisitor& visitor)
    {
        BlockID block_id;
        block_id_holder_to(block->block_id(), block_id);

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

    void set_superblock(Superblock* superblock) noexcept {
        this->superblock_ = superblock;
    }

    void for_each_history_entry(const std::function<void (const CommitID&, const CommitMetadata<ApiProfileT>&)>& fn)
    {
        init_history_ctr();
        auto scanner = history_ctr_->scanner();

        bool has_next;
        do {
            for (size_t c = 0; c < scanner.keys().size(); c++) {
                const auto& meta = scanner.values()[c];
                fn(
                    scanner.keys()[c],
                    meta
                );
            }

            auto has_next_res = scanner.next_leaf();
            has_next = has_next_res;
        }
        while (has_next);
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
                allocation_map_ctr_->internal_reset_allocator_holder();
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
                history_ctr_->internal_reset_allocator_holder();
            }
        }
    }

    Optional<int64_t> find_root(const CommitID& commit_id) {
        init_history_ctr();
        auto iter = history_ctr_->find(commit_id);

        if (iter->is_found(commit_id)) {
            return iter->value().view();
        }

        return Optional<int64_t>{};
    }

    virtual void describe_to_cout() noexcept {
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

    struct ResolvedBlock {
        uint64_t file_pos;
        SharedBlockConstPtr block;
    };

    virtual ResolvedBlock resolve_block(const BlockID& block_id) = 0;

    SharedBlockConstPtr getBlock(const BlockID& block_id) {
        return resolve_block(block_id).block;
    }

    void for_each_history_entry_batch(const std::function<void (Span<const CommitID>, Span<const CommitMetadataT>)>& fn)
    {
        init_history_ctr();

        auto ss = history_ctr_->scanner();

        bool has_next;
        do {
            fn(ss.keys(), ss.values());
            has_next = ss.next_leaf();
        }
        while (has_next);
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
};

}
