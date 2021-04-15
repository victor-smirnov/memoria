
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

namespace memoria {

// cc2cd24f-6518-4977-81d3-dad21d4f45cc
constexpr UUID DirectoryCtrID = UUID(8595428187223239884ull, 14719257946640536449ull);

// a64d654b-ec9b-4ab7-870f-83816c8d0ce2
constexpr UUID AllocationMapCtrID = UUID(13207540296396918182ull, 16288549449461075847ull);

// 0bc70e1f-adaf-4454-afda-7f6ac7104790
constexpr UUID HistoryCtrID = UUID(6072171355687536395ull, 10396296713479379631ull);


template <typename Profile>
struct CommitCheckState {
    SWMRBlockCounters<Profile> counters;
};


template <typename Profile>
struct ReferenceCounterDelegate {
    using BlockID = ProfileBlockID<Profile>;

    virtual ~ReferenceCounterDelegate() noexcept {}

    virtual VoidResult ref_block(const BlockID& block_id) noexcept = 0;
    virtual VoidResult unref_block(const BlockID& block_id, const std::function<VoidResult()>& on_zero) noexcept = 0;
    virtual VoidResult unref_ctr_root(const BlockID& root_block_id) noexcept = 0;
};

template <typename Profile> class SWMRStoreBase;

template <typename Profile>
class SWMRStoreCommitBase:
        public ProfileAllocatorType<Profile>,
        public ISWMRStoreReadOnlyCommit<ApiProfile<Profile>>
{
    using Base = ProfileAllocatorType<Profile>;
protected:

    using typename Base::BlockType;
    using typename Base::BlockG;
    using typename Base::BlockID;
    using typename Base::SnapshotID;
    using typename Base::CtrID;
    using typename Base::Shared;

    using Store = SWMRStoreBase<Profile>;

    using ApiProfileT = ApiProfile<Profile>;

    using BlockCounterCallbackFn = std::function<BoolResult(const ApiProfileBlockID<ApiProfileT>&)>;

    using CommitID = typename ISWMRStoreCommitBase<ApiProfileT>::CommitID;
    using SequenceID = uint64_t;

    using CounterStorageT = CounterStorage<Profile>;

    using CommitDescriptorT         = CommitDescriptor<Profile>;
    using CtrReferenceableResult    = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;
    using AllocatorT                = Base;
    using Superblock                = SWMRSuperblock<Profile>;

    using CtrInstanceMap = std::unordered_map<CtrID, CtrReferenceable<ApiProfileT>*>;

    using DirectoryCtrType  = Map<CtrID, BlockID>;
    using DirectoryCtr      = ICtrApi<DirectoryCtrType, ApiProfileT>;

    using AllocationMapCtrType = AllocationMap;
    using AllocationMapCtr  = ICtrApi<AllocationMapCtrType, ApiProfileT>;

    using HistoryCtrType    = Map<BigInt, BigInt>;
    using HistoryCtr        = ICtrApi<HistoryCtrType, ApiProfileT>;

    static constexpr int32_t BASIC_BLOCK_SIZE            = Store::BASIC_BLOCK_SIZE;
    static constexpr int32_t SUPERBLOCK_SIZE             = BASIC_BLOCK_SIZE;
    static constexpr size_t  HEADER_SIZE                 = Store::HEADER_SIZE;
    static constexpr int32_t ALLOCATION_MAP_LEVELS       = Store::ALLOCATION_MAP_LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP    = Store::ALLOCATION_MAP_SIZE_STEP;
    static constexpr int32_t SUPERBLOCK_ALLOCATION_LEVEL = Log2(SUPERBLOCK_SIZE / BASIC_BLOCK_SIZE) - 1;
    static constexpr int32_t SUPERBLOCKS_RESERVED        = HEADER_SIZE / BASIC_BLOCK_SIZE;



    SharedPtr<Store> store_;

    CtrInstanceMap instance_map_;

    CommitDescriptorT* commit_descriptor_;
    Superblock* superblock_;

    Logger logger_;

    CtrSharedPtr<DirectoryCtr>      directory_ctr_;
    CtrSharedPtr<AllocationMapCtr>  allocation_map_ctr_;
    CtrSharedPtr<HistoryCtr>        history_ctr_;

    ReferenceCounterDelegate<Profile>* refcounter_delegate_;

    mutable ObjectPools object_pools_;



    template <typename> friend class SWMRMappedStoreHistoryView;

public:
    using Base::getBlock;

    SWMRStoreCommitBase(
            SharedPtr<Store> store,
            CommitDescriptorT* commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate
    ) noexcept:
        store_(store),
        commit_descriptor_(commit_descriptor),
        superblock_(commit_descriptor->superblock()),
        refcounter_delegate_(refcounter_delegate)
    {}

    static void init_profile_metadata() noexcept {
        DirectoryCtr::template init_profile_metadata<Profile>();
        AllocationMapCtr::template init_profile_metadata<Profile>();
    }

    SequenceID sequence_id() const noexcept {
        return commit_descriptor_->superblock()->sequence_id();
    }

    virtual ObjectPools& object_pools() const noexcept {
        return object_pools_;
    }

    virtual Result<BlockID> getRootID(const CtrID& ctr_id) noexcept
    {
        using ResultT = Result<BlockID>;

        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            return ResultT::of(superblock_->directory_root_id());
        }
        else if (MMA_UNLIKELY(ctr_id == AllocationMapCtrID))
        {
            return ResultT::of(superblock_->allocator_root_id());
        }
        else if (MMA_UNLIKELY(ctr_id == HistoryCtrID))
        {
            return ResultT::of(superblock_->history_root_id());
        }
        else if (directory_ctr_)
        {
            MEMORIA_TRY(iter, directory_ctr_->find(ctr_id));

            if (iter->is_found(ctr_id))
            {
                return ResultT::of(iter->value().view());
            }

            return ResultT::of();
        }

        return ResultT::of();
    }



    virtual VoidResult setRoot(const CtrID& ctr_id, const BlockID& root) noexcept
    {
        return MEMORIA_MAKE_GENERIC_ERROR("setRoot() is not implemented for ReadOnly commits");
    }

    virtual BoolResult hasRoot(const CtrID& ctr_id) noexcept
    {
        using ResultT = BoolResult;

        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            return ResultT::of(superblock_->directory_root_id().is_set());
        }
        else if (MMA_UNLIKELY(ctr_id == AllocationMapCtrID))
        {
            return ResultT::of(superblock_->allocator_root_id().is_set());
        }
        else if (MMA_UNLIKELY(ctr_id == HistoryCtrID))
        {
            return ResultT::of(superblock_->history_root_id().is_set());
        }
        else if (directory_ctr_)
        {
            MEMORIA_TRY(iter, directory_ctr_->find(ctr_id));

            if (iter->is_found(ctr_id))
            {
                return BoolResult::of(true);
            }
        }

        return BoolResult::of(false);
    }

    virtual Result<CtrID> createCtrName() noexcept {
        return Result<CtrID>::of(ProfileTraits<Profile>::make_random_ctr_id());
    }

    virtual VoidResult removeBlock(const BlockID& id) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("removeBlock() is not implemented for ReadOnly commits");
    }

    virtual Result<BlockG> createBlock(int32_t initial_size) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("createBlock() is not implemented for ReadOnly commits");
    }

    virtual Result<BlockG> cloneBlock(const BlockG& block) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("cloneBlock() is not implemented for ReadOnly commits");
    }

    virtual Result<BlockID> newId() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("newId() is not implemented for ReadOnly commits");
    }

    virtual SnapshotID currentTxnId() const noexcept {
        return superblock_->commit_uuid();
    }

    // memory pool allocator
    virtual Result<void*> allocateMemory(size_t size) noexcept {
        using ResultT = Result<void*>;
        return ResultT::of(allocate_system<uint8_t>(size).release());
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

    virtual VoidResult registerCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>* instance) noexcept
    {
        auto ii = instance_map_.find(ctr_id);
        if (ii == instance_map_.end())
        {
            instance_map_.insert({ctr_id, instance});
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Container with name {} has been already registered", ctr_id);
        }

        return VoidResult::of();
    }

    virtual VoidResult unregisterCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>*) noexcept
    {
        instance_map_.erase(ctr_id);
        return VoidResult::of();
    }

    virtual VoidResult flush_open_containers() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("cloneBlock() is not implemented for ReadOnly commits");
    }


    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> find(const CtrID& ctr_id) noexcept
    {
        using ResultT = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;

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
                return ctr_intf->new_ctr_instance(block, this->self_ptr());
            }
        }
        else {
            return ResultT::of();
        }
    }

    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> from_root_id(
            const BlockID& root_block_id,
            const CtrID& name
    ) noexcept
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

    virtual BoolResult check() noexcept  {
        bool result = false;

        MEMORIA_TRY(iter, directory_ctr_->iterator());

        while(!iter->is_end())
        {
            auto ctr_name = iter->key();

            MEMORIA_TRY(block, this->getBlock(iter->value()));

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            MEMORIA_TRY(res, ctr_intf->check(ctr_name, this->self_ptr()));

            result = res || result;

            MEMORIA_TRY_VOID(iter->next());
        }

        return BoolResult::of(result);
    }

    virtual VoidResult walkContainers(
            ContainerWalker<Profile>* walker, const
            char* allocator_descr = nullptr
    ) noexcept {
        return VoidResult::of();
    }

    virtual VoidResult walk_containers(
            ContainerWalker<Profile>* walker, const
            char* allocator_descr = nullptr
    ) noexcept
    {
        if (allocator_descr != nullptr)
        {
            walker->beginSnapshot(
                        fmt::format("Snapshot-{} -- {}", superblock_->commit_uuid(), allocator_descr).data()
            );
        }
        else {
            walker->beginSnapshot(
                        fmt::format("Snapshot-{}", superblock_->commit_uuid()).data()
            );
        }

        MEMORIA_TRY(iter, directory_ctr_->iterator());
        while (!iter->is_end())
        {
            auto ctr_name   = iter->key();
            auto root_id    = iter->value();

            MEMORIA_TRY(block, getBlock(root_id));

            auto ctr_hash   = block->ctr_type_hash();
            auto ctr_intf   = ProfileMetadata<Profile>::local()
                    ->get_container_operations(ctr_hash);

            MEMORIA_TRY_VOID(ctr_intf->walk(ctr_name, this->self_ptr(), walker));

            MEMORIA_TRY_VOID(iter->next());
        }

        walker->endSnapshot();

        return VoidResult::of();
    }

    virtual BoolResult drop_ctr(const CtrID& ctr_id) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("drop_ctr() is not implemented for ReadOnly commits");
    }

    virtual Result<U8String> ctr_type_name(const CtrID& ctr_id) noexcept
    {
        using ResultT = Result<U8String>;
        MEMORIA_TRY(root_id, this->getRootID(ctr_id));
        if (root_id.is_set())
        {
            MEMORIA_TRY(block, getBlock(root_id));

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ResultT::of(ctr_intf->ctr_type_name());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't find container with id {}", ctr_id);
        }
    }


    //=================================== R/O Commit Stuff ===========================

    virtual CommitID commit_id() noexcept {
        return superblock_->commit_id();
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


    virtual VoidResult dump_open_containers() noexcept
    {
        for (const auto& pair: instance_map_)
        {
            std::cout << pair.first << " -- " << pair.second->describe_type() << std::endl;
        }

        return VoidResult::of();
    }

    virtual BoolResult has_open_containers() noexcept {
        return Result<bool>::of(instance_map_.size() > 0);
    }

    virtual Result<std::vector<CtrID>> container_names() const noexcept {
        using ResultT = Result<std::vector<CtrID>>;

        std::vector<CtrID> names;

        MEMORIA_TRY(ii, directory_ctr_->iterator());

        while (!ii->is_end())
        {
            names.push_back(ii->key());
            MEMORIA_TRY_VOID(ii->next());
        }

        return ResultT::of(std::move(names));
    }

    virtual VoidResult drop() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("drop() has to be implemented!!");
    }


    virtual Result<Optional<U8String>> ctr_type_name_for(const CtrID& name) noexcept
    {
        using ResultT = Result<Optional<U8String>>;

        MEMORIA_TRY(root_id, getRootID(name));
        MEMORIA_TRY(block, getBlock(root_id));

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

    virtual Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>> snapshot_ref_opening_allowed() noexcept {
        using ResultT = Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>>;
        return ResultT::of();
    }




    virtual VoidResult ref_block(const BlockID& block_id) noexcept
    {
        if (refcounter_delegate_) {
            return refcounter_delegate_->ref_block(block_id);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("ref_block() is not implemented for ReadOnly commits");
        }
    }

    virtual VoidResult unref_block(const BlockID& block_id, std::function<VoidResult()> on_zero) noexcept
    {
        if (refcounter_delegate_)
        {
            return refcounter_delegate_->unref_block(block_id, on_zero);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("unref_block() is not implemented for ReadOnly commits");
        }
    }

    virtual VoidResult unref_ctr_root(const BlockID& root_block_id) noexcept
    {
        if (refcounter_delegate_)
        {
            return refcounter_delegate_->unref_ctr_root(root_block_id);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("unref_ctr_root() is not implemented for ReadOnly commits");
        }
    }

    virtual VoidResult traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    ) noexcept
    {
        MEMORIA_TRY(instance, from_root_id(root_block, CtrID{}));
        return instance->traverse_ctr(&node_handler);
    }

    virtual VoidResult check_updates_allowed() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("updated are not allowed for ReadOnly commits");
    }

    virtual VoidResult check(std::function<VoidResult(LDDocument&)> callback) noexcept {
        return VoidResult::of();
    }

    virtual VoidResult build_block_refcounters(SWMRBlockCounters<Profile>& counters) noexcept
    {
        auto counters_fn = [&](const ApiProfileBlockID<ApiProfileT>& block_id_holder) -> BoolResult {
            return wrap_throwing([&](){
                BlockID block_id;
                block_id_holder_to(block_id_holder, block_id);
                return BoolResult::of(counters.inc(block_id));
            });
        };

        MEMORIA_TRY_VOID(traverse_cow_containers(counters_fn));

        MEMORIA_TRY_VOID(traverse_ctr_cow_tree(
            commit_descriptor_->superblock()->history_root_id(),
            counters_fn
        ));

        MEMORIA_TRY_VOID(traverse_ctr_cow_tree(
            commit_descriptor_->superblock()->allocator_root_id(),
            counters_fn
        ));

        return VoidResult::of();
    }




    virtual VoidResult traverse_cow_containers(const BlockCounterCallbackFn& callback) noexcept {
        MEMORIA_TRY_VOID(traverse_ctr_cow_tree(commit_descriptor_->superblock()->directory_root_id(), callback));

        MEMORIA_TRY(iter, directory_ctr_->iterator());
        while (!iter->is_end())
        {
            auto root_id = iter->value();
            MEMORIA_TRY_VOID(traverse_ctr_cow_tree(root_id, callback));
            MEMORIA_TRY_VOID(iter->next());
        }

        return VoidResult::of();
    }


    VoidResult traverse_ctr_cow_tree(const BlockID& root_block_id, const BlockCounterCallbackFn& callback) noexcept {
        MEMORIA_TRY(ref, from_root_id(root_block_id, CtrID{}));
        MEMORIA_TRY(root_block, ref->root_block());
        return traverse_block_tree(root_block, callback);
    }

    VoidResult traverse_block_tree(
            CtrBlockPtr<ApiProfileT> block,
            const BlockCounterCallbackFn& callback) noexcept
    {
        MEMORIA_TRY(traverse, callback(block->block_id()));
        if (traverse) {
            MEMORIA_TRY(children, block->children());
            for (size_t c = 0; c < children.size(); c++) {
                MEMORIA_TRY_VOID(traverse_block_tree(children[c], callback));
            }
        }

        return VoidResult::of();
    }

    template<typename CtrName>
    Result<CtrSharedPtr<ICtrApi<CtrName, ApiProfileT>>> internal_find_by_root_typed(const BlockID& root_block_id) noexcept
    {
        auto ref = from_root_id(root_block_id, CtrID{});
        return memoria_static_pointer_cast<ICtrApi<CtrName, ApiProfileT>>(std::move(ref));
    }


    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> internal_create_by_name(
            const LDTypeDeclarationView& decl, const CtrID& ctr_id
    ) noexcept
    {
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(this, ctr_id, decl);
    }

    template<typename CtrName>
    Result<CtrSharedPtr<ICtrApi<CtrName, ApiProfileT>>> internal_create_by_name_typed(const CtrID& ctr_id) noexcept
    {
        using ResultT = Result<CtrSharedPtr<ICtrApi<CtrName, ApiProfileT>>>;
        return wrap_throwing([&]() -> ResultT {
            U8String signature = make_datatype_signature(CtrName{}).name();

            LDDocument doc = TypeSignature::parse(signature.to_std_string());
            LDTypeDeclarationView decl = doc.value().as_type_decl();

            MEMORIA_TRY(ctr_ref, internal_create_by_name(decl, ctr_id));
            (void)ctr_ref;

            return memoria_static_pointer_cast<ICtrApi<CtrName, ApiProfileT>>(std::move(ctr_ref_result));
        });
    }

    void set_superblock(Superblock* superblock) noexcept {
        this->superblock_ = superblock;
    }

    VoidResult for_each_history_entry(const std::function<VoidResult (CommitID, int64_t)>& fn) noexcept
    {
        MEMORIA_TRY_VOID(init_history_ctr());
        MEMORIA_TRY(scanner, history_ctr_->scanner_from(history_ctr_->iterator()));

        bool has_next;
        do {
            for (size_t c = 0; c < scanner.keys().size(); c++) {
                MEMORIA_TRY_VOID(fn(scanner.keys()[c], scanner.values()[c]));
            }

            MEMORIA_TRY(has_next_res, scanner.next_leaf());
            has_next = has_next_res;
        }
        while (has_next);

        return VoidResult::of();
    }

    VoidResult init_allocator_ctr() noexcept
    {
        if (!allocation_map_ctr_)
        {
            auto root_block_id = commit_descriptor_->superblock()->allocator_root_id();
            if (root_block_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<AllocationMapCtrType>(root_block_id);
                MEMORIA_RETURN_IF_ERROR(ctr_ref);
                allocation_map_ctr_ = ctr_ref.get();
                allocation_map_ctr_->internal_reset_allocator_holder();
            }
        }

        return VoidResult::of();
    }


    VoidResult init_history_ctr() noexcept
    {
        if (!history_ctr_)
        {
            auto root_block_id = commit_descriptor_->superblock()->history_root_id();
            if (root_block_id.is_set())
            {
                auto ctr_ref = this->template internal_find_by_root_typed<HistoryCtrType>(root_block_id);
                MEMORIA_RETURN_IF_ERROR(ctr_ref);
                history_ctr_ = ctr_ref.get();
                history_ctr_->internal_reset_allocator_holder();
            }
        }

        return VoidResult::of();
    }

    Result<Optional<int64_t>> find_root(const CommitID& commit_id) noexcept {
        MEMORIA_TRY_VOID(init_history_ctr());
        MEMORIA_TRY(iter, history_ctr_->find(commit_id));

        if (iter->is_found(commit_id)) {
            return Result<Optional<int64_t>>::of(iter->value().view());
        }

        return Result<Optional<int64_t>>::of(Optional<int64_t>{});
    }

    virtual VoidResult describe_to_cout() noexcept {
        if (allocation_map_ctr_) {
            MEMORIA_TRY(ii, allocation_map_ctr_->iterator());
            return ii->dump();
        }

        return VoidResult::of();
    }

    VoidResult for_each_root_block(const std::function<VoidResult (int64_t)>& fn) const noexcept
    {
        MEMORIA_TRY(scanner, history_ctr_->scanner_from(history_ctr_->iterator()));

        bool has_next;
        do {

            for (auto superblock_ptr: scanner.values())
            {
                MEMORIA_TRY_VOID(fn(superblock_ptr));
            }

            MEMORIA_TRY(has_next_res, scanner.next_leaf());
            has_next = has_next_res;
        }
        while (has_next);

        return VoidResult::of();
    }
};

}
