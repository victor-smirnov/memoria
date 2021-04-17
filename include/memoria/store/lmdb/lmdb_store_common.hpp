
// Copyright 2021 Victor Smirnov
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
#include <memoria/store/lmdb/lmdb_store_superblock.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/core/container/ctr_impl.hpp>

#include <memoria/api/map/map_api.hpp>

#include <boost/intrusive/list.hpp>

#include <atomic>
#include <memory>
#include <absl/container/btree_map.h>

#include <mdb/mma_lmdb.h>

namespace memoria {

// cc2cd24f-6518-4977-81d3-dad21d4f45cc
constexpr UUID DirectoryCtrID = UUID(8595428187223239884ull, 14719257946640536449ull);

template <typename Profile> class LMDBStore;

template <typename Profile>
class LMDBStoreCommitBase:
        public ProfileAllocatorType<Profile>,
        public ISWMRStoreReadOnlyCommit<ApiProfile<Profile>>
{
    using Base = ProfileAllocatorType<Profile>;
protected:

    using typename Base::BlockType;
    using typename Base::SharedBlockPtr;
    using typename Base::BlockID;
    using typename Base::SnapshotID;
    using typename Base::CtrID;

    using Base::getBlock;

    using ApiProfileT = ApiProfile<Profile>;

    using BlockCounterCallbackFn = std::function<BoolResult(const ApiProfileBlockID<ApiProfileT>&)>;

    using CommitID = typename ISWMRStoreCommitBase<ApiProfileT>::CommitID;

    using Store                     = LMDBStore<Profile>;
    using CtrReferenceableResult    = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;
    using AllocatorT                = Base;
    using Superblock                = LMDBSuperblock<Profile>;

    using CtrInstanceMap = std::unordered_map<CtrID, CtrReferenceable<ApiProfileT>*>;

    using DirectoryCtrType  = Map<CtrID, BlockID>;
    using DirectoryCtr      = ICtrApi<DirectoryCtrType, ApiProfileT>;

    static constexpr int32_t BASIC_BLOCK_SIZE = Store::BASIC_BLOCK_SIZE;

    CtrInstanceMap instance_map_;

    SharedPtr<Store> store_;
    Superblock* superblock_;

    Logger logger_;

    CtrSharedPtr<DirectoryCtr> directory_ctr_;

    mutable ObjectPools object_pools_;

    MDB_env* mdb_env_;
    MDB_txn* transaction_;
    MDB_dbi system_db_;
    MDB_dbi data_db_;

    template <typename> friend class SWMRMappedStoreHistoryView;

public:
    LMDBStoreCommitBase(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            MDB_env* mdb_env
    ) noexcept:
        store_(store),
        mdb_env_(mdb_env),
        transaction_(), system_db_(), data_db_()
    {}

    static void init_profile_metadata() noexcept {
        DirectoryCtr::template init_profile_metadata<Profile>();
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



    virtual BoolResult hasRoot(const CtrID& ctr_id) noexcept
    {
        using ResultT = BoolResult;

        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            return ResultT::of(superblock_->directory_root_id().is_set());
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

    virtual SnapshotID currentTxnId() const noexcept {
        return SnapshotID{0, mma_mdb_txn_id(transaction_)};
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
                        fmt::format("Snapshot-{} -- {}", commit_id(), allocator_descr).data()
            );
        }
        else {
            walker->beginSnapshot(
                        fmt::format("Snapshot-{}", commit_id()).data()
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
        return mma_mdb_txn_id(transaction_);
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

protected:





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


    virtual VoidResult describe_to_cout() noexcept {
//        if (allocation_map_ctr_) {
//            MEMORIA_TRY(ii, allocation_map_ctr_->iterator());
//            return ii->dump();
//        }

        return VoidResult::of();
    }

    Result<MDB_val> get_data_addr(const BlockID& block_id, MDB_dbi dbi) noexcept
    {
        MDB_val key = {sizeof(block_id), ptr_cast<void>(&block_id)};
        MDB_val data;

        if (int rc = mma_mdb_get(transaction_, dbi, &key, &data)) {
            if (rc == MDB_NOTFOUND) {
                //No value found
                return Result<MDB_val>::of(MDB_val{0, nullptr});
            }
            else {
                return make_generic_error("Can't read data block {}, error = {}", block_id, mma_mdb_strerror(rc));
            }
        }
        else {
            return Result<MDB_val>::of(data);
        }
    }
};

}
