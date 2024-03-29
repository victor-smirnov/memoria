
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
#include <memoria/core/container/ctr_instance_pool.hpp>


#include <memoria/api/map/map_api.hpp>

#include <boost/intrusive/list.hpp>

#include <atomic>
#include <memory>
#include <absl/container/btree_map.h>

#include <lmdb.h>

namespace memoria {

// {1|5251c5c1590990a213b87c5e741feed021fab6bf71fe7bfd52154a7344eb5c}
constexpr UID256 DirectoryCtrID = UID256(3029111194483692837ull, 1004005058050231089ull, 16120616277477404434ull, 127717364650496293ull);

template <typename Profile> class LMDBStore;

template <typename Profile>
class LMDBStoreSnapshotBase:
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

    using Base::getBlock;

    using ApiProfileT = ApiProfile<Profile>;

    using Store                     = LMDBStore<Profile>;
    using CtrReferenceableResult    = Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>>;
    using StoreT                    = Base;

    using Superblock                = LMDBSuperblock<Profile>;

    using CtrInstanceMap = std::unordered_map<CtrID, CtrReferenceable<ApiProfileT>*>;

    using DirectoryCtrType  = Map<CtrID, BlockID>;
    using DirectoryCtr      = ICtrApi<DirectoryCtrType, ApiProfileT>;

    static constexpr size_t BASIC_BLOCK_SIZE = Store::BASIC_BLOCK_SIZE;

    SharedPtr<Store> store_;
    Superblock* superblock_;

    CtrSharedPtr<DirectoryCtr> directory_ctr_;

    mutable ObjectPools object_pools_;
    mutable std::shared_ptr<CtrInstancePool<Profile>> instance_pool_;


    MDB_env* mdb_env_;
    MDB_txn* transaction_{};
    MDB_dbi system_db_;
    MDB_dbi data_db_;

    bool mutable_{false};

    hermes::HermesCtr metadata_;

    template <typename> friend class SWMRMappedStoreHistoryView;

public:
    LMDBStoreSnapshotBase(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            MDB_env* mdb_env
    ):
        store_(store),
        mdb_env_(mdb_env),
        transaction_(), system_db_(), data_db_()
    {
        instance_pool_ = std::make_shared<CtrInstancePool<Profile>>();
    }

    static void init_profile_metadata() noexcept {
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
                mutable_
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

            auto instance = ctr_intf->create_ctr_instance(this->self_ptr(), root, mutable_);

            return instance_pool.put_new_instance(ctr_id, std::move(instance));
        }
    }

    void on_ctr_drop(const CtrID& ctr_id) {
        instance_pool_->remove(ctr_id);
    }


    hermes::HermesCtr metadata() {
        return metadata_;
    }

    virtual ObjectPools& object_pools() const {
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
                return iter->current_value();
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

    virtual SnapshotID snaphsot_Id() const {
        return SnapshotID{0, mdb_txn_id(transaction_)};
    }

    // memory pool allocator
    virtual void* allocateMemory(size_t size) {
        return allocate_system<uint8_t>(size).release();
    }

    virtual void freeMemory(void* ptr) noexcept {
        free_system(ptr);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id)
    {
        auto root_id = getRootID(ctr_id);
        if (root_id.is_set())
        {
            auto block = getBlock(root_id);
            return this->find_ctr_instance(ctr_id, block);
        }

        return CtrSharedPtr<CtrReferenceable<ApiProfileT>>{};
    }


    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const hermes::Datatype& decl, const CtrID& ctr_id
    ) = 0;


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
      directory_ctr_->for_each([&](auto ctr_name, auto block_id){
        auto block = getBlock(block_id);

        auto ctr_intf = ProfileMetadata<Profile>::local()
                ->get_container_operations(block->ctr_type_hash());

        ctr_intf->check(ctr_name, this->self_ptr(), consumer);
      });
    }

    void check_storage(SharedBlockConstPtr block, const CheckResultConsumerFn& consumer) {}

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
                        fmt::format("Snapshot-{} -- {}", snapshot_id(), allocator_descr).data()
            );
        }
        else {
            walker->beginSnapshot(
                        fmt::format("Snapshot-{}", snapshot_id()).data()
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

    virtual SnapshotID snapshot_id() {
        return SnapshotID::make_type2(SnapshotID(), 0, mdb_txn_id(transaction_));
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


    virtual void traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    )
    {
//        auto instance = from_root_id(root_block);
//        return instance->traverse_ctr(&node_handler);
    }

    virtual void check_updates_allowed() {
        MEMORIA_MAKE_GENERIC_ERROR("updated are not allowed for ReadOnly commits").do_throw();
    }

    virtual void check(std::function<VoidResult(hermes::HermesCtr&)> callback) {
    }

protected:

    template<typename CtrName>
    CtrSharedPtr<ICtrApi<CtrName, ApiProfileT>> internal_find_by_root_typed(const BlockID& root_block_id)
    {
        auto ref = from_root_id(root_block_id);
        if (ref) {
            ref->internal_detouch_from_store();
        }
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

    void set_superblock(Superblock* superblock) {
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

        if (int rc = mdb_get(transaction_, dbi, &key, &data)) {
            if (rc == MDB_NOTFOUND) {
                //No value found
                return MDB_val{0, nullptr};
            }
            else {
                make_generic_error("Can't read data block {}, error = {}", block_id, mdb_strerror(rc)).do_throw();
            }
        }
        else {
            return data;
        }
    }

    /*
    std::enable_if_t<!std::is_same_v<CtrID, BlockID>, MDB_val> get_data_addr(const CtrID& ctr_id, MDB_dbi dbi)
    {
        MDB_val key = {sizeof(ctr_id), ptr_cast<void>(&ctr_id)};
        MDB_val data;

        if (int rc = mma_mdb_get(transaction_, dbi, &key, &data)) {
            if (rc == MDB_NOTFOUND) {
                //No value found
                return MDB_val{0, nullptr};
            }
            else {
                make_generic_error("Can't read data block {}, error = {}", ctr_id, mma_mdb_strerror(rc)).do_throw();
            }
        }
        else {
            return data;
        }
    }*/

    void start_no_reentry(const CtrID& ctr_id) {}
    void finish_no_reentry(const CtrID& ctr_id) noexcept {}
};

}
