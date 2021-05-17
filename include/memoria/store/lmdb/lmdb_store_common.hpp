
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
        public ProfileStoreType<Profile>,
        public ISWMRStoreReadOnlyCommit<ApiProfile<Profile>>
{
    using Base = ProfileStoreType<Profile>;
protected:

    using typename Base::BlockType;
    using typename Base::SharedBlockPtr;
    using typename Base::BlockID;
    using typename Base::SnapshotID;
    using typename Base::CtrID;

    using Base::getBlock;

    using ApiProfileT = ApiProfile<Profile>;

    using CommitID = typename ISWMRStoreCommitBase<ApiProfileT>::CommitID;

    using Store                     = LMDBStore<Profile>;
    using CtrReferenceableResult    = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;
    using StoreT                = Base;
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

    virtual BlockID getRootID(const CtrID& ctr_id)
    {
        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            return superblock_->directory_root_id();
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



    virtual bool hasRoot(const CtrID& ctr_id)
    {
        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            return superblock_->directory_root_id().is_set();
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

    virtual SnapshotID currentTxnId() const {
        return SnapshotID{0, mma_mdb_txn_id(transaction_)};
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




    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id)
    {
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
                return ctr_intf->new_ctr_instance(block, this->self_ptr());
            }
        }

        return CtrSharedPtr<CtrReferenceable<ApiProfileT>>{};
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> from_root_id(
            const BlockID& root_block_id,
            const CtrID& name
    )
    {
        if (root_block_id.is_set())
        {
            auto block = this->getBlock(root_block_id);

            auto ctr_intf = ProfileMetadata<Profile>::local()
                    ->get_container_operations(block->ctr_type_hash());

            return ctr_intf->new_ctr_instance(block, this);
        }

        return CtrSharedPtr<CtrReferenceable<ApiProfileT>>{};
    }

    virtual bool check() {
        bool result = false;

        auto iter = directory_ctr_->iterator();

        while(!iter->is_end())
        {
            auto ctr_name = iter->key();

            auto block = getBlock(iter->value());

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
        auto root_id = getRootID(ctr_id);
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

    virtual CommitID commit_id() {
        return uuid_pack_uint64_t(mma_mdb_txn_id(transaction_));
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

    virtual std::vector<CtrID> container_names() const
    {
        std::vector<CtrID> names;

        auto ii = directory_ctr_->iterator();

        while (!ii->is_end())
        {
            names.push_back(ii->key());
            ii->next();
        }

        return std::move(names);
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

    virtual SnpSharedPtr<StoreApiBase<ApiProfileT>> snapshot_ref_opening_allowed() {
        return SnpSharedPtr<StoreApiBase<ApiProfileT>>{};
    }


    virtual void traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    )
    {
        auto instance = from_root_id(root_block, CtrID{});
        return instance->traverse_ctr(&node_handler);
    }

    virtual void check_updates_allowed() {
        MEMORIA_MAKE_GENERIC_ERROR("updated are not allowed for ReadOnly commits").do_throw();
    }

    virtual void check(std::function<VoidResult(LDDocument&)> callback) {
    }

protected:





    template<typename CtrName>
    CtrSharedPtr<ICtrApi<CtrName, ApiProfileT>> internal_find_by_root_typed(const BlockID& root_block_id)
    {
        auto ref = from_root_id(root_block_id, CtrID{});
        return memoria_static_pointer_cast<ICtrApi<CtrName, ApiProfileT>>(std::move(ref));
    }


    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const LDTypeDeclarationView& decl, const CtrID& ctr_id
    )
    {
        auto factory = ProfileMetadata<Profile>::local()->get_container_factories(decl.to_cxx_typedecl());
        return factory->create_instance(this, ctr_id, decl);
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


    virtual void describe_to_cout() {
//        if (allocation_map_ctr_) {
//            MEMORIA_TRY(ii, allocation_map_ctr_->iterator());
//            return ii->dump();
//        }
    }

    MDB_val get_data_addr(const BlockID& block_id, MDB_dbi dbi)
    {
        MDB_val key = {sizeof(block_id), ptr_cast<void>(&block_id)};
        MDB_val data;

        if (int rc = mma_mdb_get(transaction_, dbi, &key, &data)) {
            if (rc == MDB_NOTFOUND) {
                //No value found
                return MDB_val{0, nullptr};
            }
            else {
                make_generic_error("Can't read data block {}, error = {}", block_id, mma_mdb_strerror(rc)).do_throw();
            }
        }
        else {
            return data;
        }
    }
};

}
