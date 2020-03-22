
// Copyright 2020 Victor Smirnov
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
#include <memoria/store/swmr/common/superblock.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/core/container/ctr_impl.hpp>

#include <memoria/api/map/map_api.hpp>
#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <boost/intrusive/list.hpp>

#include <atomic>
#include <memory>

namespace memoria {

// cc2cd24f-6518-4977-81d3-dad21d4f45cc
constexpr UUID DirectoryCtrID = UUID(8595428187223239884ull, 14719257946640536449ull);

// a64d654b-ec9b-4ab7-870f-83816c8d0ce2
constexpr UUID AllocationMapCtrID = UUID(13207540296396918182ull, 16288549449461075847ull);

// 58befbb8-aa33-4f3b-9a7f-a79f24ef3e5e
constexpr UUID RefcountersCtrID = UUID(4273691379738852952ull, 6791128228697702298ull);

// 0bc70e1f-adaf-4454-afda-7f6ac7104790
constexpr UUID HistoryCtrID = UUID(6072171355687536395ull, 10396296713479379631ull);

template <typename Profile> class MappedSWMRStore;

template <typename Profile>
class CommitDescriptor: public boost::intrusive::list_base_hook<> {
    using Superblock = SWMRSuperblock<Profile>;
    using BlockID = ProfileBlockID<Profile>;

    std::atomic<int32_t> uses_{};
    Superblock* superblock_;
public:
    CommitDescriptor() noexcept:
        superblock_(nullptr)
    {}

    CommitDescriptor(Superblock* superblock) noexcept:
        superblock_(superblock)
    {}

    Superblock* superblock() noexcept {
        return superblock_;
    }

    void set_superblock(Superblock* superblock) noexcept {
        superblock_ = superblock;
    }
};

template <typename Profile>
using CommitDescriptorsList = boost::intrusive::list<CommitDescriptor<Profile>>;

template <typename Profile>
class MappedSWMRStoreCommitBase:
        public ProfileAllocatorType<Profile>,
        public ISWMRStoreReadOnlyCommit<Profile>
{
    using Base = ProfileAllocatorType<Profile>;
protected:

    using typename Base::BlockType;
    using typename Base::BlockG;
    using typename Base::BlockID;
    using typename Base::SnapshotID;
    using typename Base::CtrID;

    using CommitID = typename ISWMRStoreCommitBase<Profile>::CommitID;

    using Store                     = MappedSWMRStore<Profile>;
    using CommitDescriptorT         = CommitDescriptor<Profile>;
    using CtrReferenceableResult    = Result<CtrSharedPtr<CtrReferenceable<Profile>>>;
    using AllocatorT                = Base;
    using Superblock                = SWMRSuperblock<Profile>;

    using CtrInstanceMap = std::unordered_map<CtrID, CtrReferenceable<Profile>*>;


    using DirectoryCtrType  = Map<CtrID, BlockID>;
    using DirectoryCtr      = ICtrApi<DirectoryCtrType, Profile>;

    using AllocationMapCtrType = AllocationMap;
    using AllocationMapCtr  = ICtrApi<AllocationMapCtrType, Profile>;

    using RefcountersCtrType = Map<BigInt, BigInt>;
    using RefcountersCtr     = ICtrApi<RefcountersCtrType, Profile>;

    using HistoryCtrType    = Map<BigInt, BigInt>;
    using HistoryCtr        = ICtrApi<HistoryCtrType, Profile>;

    static constexpr int32_t BASIC_BLOCK_SIZE = Store::BASIC_BLOCK_SIZE;

    CtrInstanceMap instance_map_;

    SharedPtr<Store> store_;
    Span<uint8_t> buffer_;

    CommitDescriptorT* commit_descriptor_;
    Superblock* superblock_;

    Logger logger_;

    CtrSharedPtr<DirectoryCtr> directory_ctr_;


public:
    MappedSWMRStoreCommitBase(MaybeError& maybe_error, SharedPtr<Store> store, Span<uint8_t> buffer, CommitDescriptorT* commit_descriptor) noexcept:
        store_(store),
        buffer_(buffer),
        commit_descriptor_(commit_descriptor),
        superblock_(commit_descriptor->superblock())
    {

    }

    static void init_profile_metadata() noexcept {
        DirectoryCtr::init_profile_metadata();
        RefcountersCtr::init_profile_metadata();
        AllocationMapCtr::init_profile_metadata();
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
        else if (MMA_UNLIKELY(ctr_id == RefcountersCtrID))
        {
            return ResultT::of(superblock_->counters_root_id());
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
        else if (MMA_UNLIKELY(ctr_id == RefcountersCtrID))
        {
            return ResultT::of(superblock_->counters_root_id().is_set());
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

    virtual Result<BlockG> getBlock(const BlockID& id) noexcept
    {
        using ResultT = Result<BlockG>;
        BlockType* block = ptr_cast<BlockType>(buffer_.data() + id.value() * BASIC_BLOCK_SIZE);
        return ResultT::of(BlockG{block});
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

    virtual VoidResult registerCtr(const CtrID& ctr_id, CtrReferenceable<Profile>* instance) noexcept
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

    virtual VoidResult unregisterCtr(const CtrID& ctr_id, CtrReferenceable<Profile>*) noexcept
    {
        instance_map_.erase(ctr_id);
        return VoidResult::of();
    }

    virtual VoidResult flush_open_containers() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("cloneBlock() is not implemented for ReadOnly commits");
    }


    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> find(const CtrID& ctr_id) noexcept
    {
        using ResultT = Result<CtrSharedPtr<CtrReferenceable<Profile>>>;

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

    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> from_root_id(
            const BlockID& root_block_id,
            const CtrID& name
    ) noexcept
    {
        using ResultT = Result<CtrSharedPtr<CtrReferenceable<Profile>>>;

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

            MEMORIA_TRY(block, getBlock(iter->value()));

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
        MEMORIA_TRY(root_id, getRootID(ctr_id));
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

    virtual Result<SnpSharedPtr<AllocatorT>> snapshot_ref_opening_allowed() noexcept {
        using ResultT = Result<SnpSharedPtr<AllocatorT>>;
        return ResultT::of();
    }




    virtual VoidResult ref_block(BlockG block, int64_t amount = 1) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("ref_block() is not implemented for ReadOnly commits");
    }
    virtual BoolResult unref_block(BlockG block) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("unref_block() is not implemented for ReadOnly commits");
    }

    virtual VoidResult unref_ctr_root(const BlockID& root_block_id) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("unref_ctr_root() is not implemented for ReadOnly commits");
    }

    virtual VoidResult traverse_ctr(
            BlockID root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    ) noexcept
    {
        MEMORIA_TRY(instance, from_root_id(root_block, CtrID{}));
        return instance->traverse_ctr(node_handler);
    }

    virtual VoidResult check_updates_allowed() noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("updated are not allowed for ReadOnly commits");
    }

protected:

    template<typename CtrName>
    Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> internal_find_by_root_typed(const BlockID& root_block_id) noexcept
    {
        auto ref = from_root_id(root_block_id, CtrID{});
        return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ref));
    }


    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> internal_create_by_name(
            const LDTypeDeclarationView& decl, const CtrID& ctr_id
    ) noexcept
    {
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(this, ctr_id, decl);
    }

    template<typename CtrName>
    Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> internal_create_by_name_typed(const CtrID& ctr_id) noexcept
    {
        using ResultT = Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>>;
        return wrap_throwing([&]() -> ResultT {
            U8String signature = make_datatype_signature(CtrName{}).name();

            LDDocument doc = TypeSignature::parse(signature.to_std_string());
            LDTypeDeclarationView decl = doc.value().as_type_decl();

            MEMORIA_TRY(ctr_ref, internal_create_by_name(decl, ctr_id));
            (void)ctr_ref;

            return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ctr_ref_result));
        });
    }

    void set_superblock(Superblock* superblock) noexcept {
        this->superblock_ = superblock;
    }
};

}
