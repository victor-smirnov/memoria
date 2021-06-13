
// Copyright 2013-2021 Victor Smirnov
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

#include <memoria/core/types.hpp>
#include <memoria/profiles/common/common.hpp>

#include <memoria/core/container/names.hpp>
#include <memoria/profiles/common/block.hpp>
#include <memoria/core/container/ctr_referenceable.hpp>

#include <memoria/core/memory/memory.hpp>
#include <memoria/profiles/common/container_operations.hpp>

#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/object_pool.hpp>

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <memory>
#include <typeinfo>

namespace memoria {

template <typename Profile>
struct IROStoreBase: ROStoreApiBase<ApiProfile<Profile>> {

    using ApiProfileT = ApiProfile<Profile>;

    using MyType        = IROStoreBase<Profile>;

    using BlockType     = ProfileBlockType<Profile>;
    using ID            = ProfileBlockID<Profile>;
    using BlockID       = ProfileBlockID<Profile>;
    using BlockGUID     = ProfileBlockGUID<Profile>;
    using SnapshotID    = ProfileSnapshotID<Profile>;
    using CtrID         = ProfileCtrID<Profile>;

    using SharedBlockPtr        = typename ProfileTraits<Profile>::SharedBlockPtr;
    using SharedBlockConstPtr   = typename ProfileTraits<Profile>::SharedBlockConstPtr;
    
    virtual BlockID getRootID(const CtrID& ctr_id) = 0;
    virtual void setRoot(const CtrID& ctr_id, const BlockID& root) = 0;

    virtual bool hasRoot(const CtrID& ctr_id) = 0;
    virtual CtrID createCtrName() = 0;

    virtual SharedBlockConstPtr getBlock(const BlockID& id) = 0;

    virtual void removeBlock(const BlockID& id) = 0;
    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID& ctr_id) = 0;
    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID& ctr_id) = 0;

    virtual BlockID newId() = 0;
    virtual SnapshotID currentTxnId() const = 0;

    // memory pool allocator
    virtual void* allocateMemory(size_t size) = 0;
    virtual void freeMemory(void* ptr) noexcept = 0;

    virtual Logger& logger() noexcept = 0;

    virtual bool isActive() const = 0;

    virtual void registerCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>* instance) = 0;
    virtual void unregisterCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>* instance) = 0;
    virtual void flush_open_containers() = 0;

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id) = 0;
    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> from_root_id(const BlockID& root_block_id) = 0;

    virtual bool check() = 0;
    virtual void walkContainers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) = 0;

    virtual bool drop_ctr(const CtrID& ctr_id) = 0;

    virtual U8String ctr_type_name(const CtrID& ctr_id) = 0;
};



template <typename Profile>
struct IRWStoreBase: RWStoreApiBase<ApiProfile<Profile>> {
/*
    using ApiProfileT   = ApiProfile<Profile>;

    using MyType        = IRWStoreBase<Profile>;

    using BlockType     = ProfileBlockType<Profile>;
    using ID            = ProfileBlockID<Profile>;
    using BlockID       = ProfileBlockID<Profile>;
    using BlockGUID     = ProfileBlockGUID<Profile>;
    using SnapshotID    = ProfileSnapshotID<Profile>;
    using CtrID         = ProfileCtrID<Profile>;

    using SharedBlockPtr        = typename ProfileTraits<Profile>::SharedBlockPtr;
    using SharedBlockConstPtr   = typename ProfileTraits<Profile>::SharedBlockConstPtr;


    virtual void setRoot(const CtrID& ctr_id, const BlockID& root) = 0;


    virtual CtrID createCtrName() = 0;

    virtual SharedBlockConstPtr getBlock(const BlockID& id) = 0;

    virtual void removeBlock(const BlockID& id) = 0;
    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID& ctr_id) = 0;
    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID& ctr_id) = 0;

    virtual BlockID newId() = 0;

    virtual bool isActive() const = 0;

    virtual void flush_open_containers() = 0;

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id) = 0;
    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> from_root_id(const BlockID& root_block_id) = 0;

    virtual bool drop_ctr(const CtrID& ctr_id) = 0;

    */
};



template <typename Profile>
struct IROStore: IROStoreBase<Profile> {
    using Base = IROStoreBase<Profile>;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockType;
    using typename Base::CtrID;

    using Shared = typename SharedBlockPtr::Shared;

    virtual SnpSharedPtr<IROStore> self_ptr() noexcept = 0;
    //virtual SnpSharedPtr<IROStore> rw_self_ptr() noexcept = 0;

    virtual void updateBlock(Shared* block) = 0;
    virtual void resizeBlock(Shared* block, int32_t new_size) = 0;
    virtual void releaseBlock(Shared* block) noexcept = 0;

    virtual ObjectPools& object_pools() const noexcept = 0;
};


template <typename Profile>
struct IRWStore: IRWStoreBase<Profile> {
    /*
    using Base = IRWStoreBase<Profile>;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockType;
    using typename Base::CtrID;

    using Shared = typename SharedBlockPtr::Shared;

    virtual SnpSharedPtr<IRWStore> rw_self_ptr() noexcept = 0;

    virtual void updateBlock(Shared* block) = 0;
    virtual void resizeBlock(Shared* block, int32_t new_size) = 0;
    virtual void releaseBlock(Shared* block) noexcept = 0;
    */
};


template <typename Profile>
struct IROCowStore: IROStoreBase<Profile> {
    using Base = IROStoreBase<Profile>;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockID;
    using typename Base::CtrID;

    using Shared = typename SharedBlockPtr::Shared;

    virtual SnpSharedPtr<IROCowStore> self_ptr() noexcept = 0;
    //virtual SnpSharedPtr<IROCowStore> rw_self_ptr() noexcept = 0;

    virtual void ref_block(const BlockID& block_id) = 0;
    virtual void unref_block(const BlockID& block_id, std::function<void ()> on_zero) = 0;
    virtual void unref_ctr_root(const BlockID& root_block_id) = 0;

    virtual void releaseBlock(Shared* block) noexcept = 0;
    virtual void updateBlock(Shared* block) = 0;

    virtual void traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    ) = 0;

    virtual void check_updates_allowed() = 0;
};


template <typename Profile>
struct IRWCowStore: IRWStoreBase<Profile> {
    /*
    using Base = IRWStoreBase<Profile>;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockID;
    using typename Base::CtrID;

    using Shared = typename SharedBlockPtr::Shared;

    virtual SnpSharedPtr<IRWCowStore> rw_self_ptr() noexcept = 0;

    virtual void ref_block(const BlockID& block_id) = 0;
    virtual void unref_block(const BlockID& block_id, std::function<void ()> on_zero) = 0;
    virtual void unref_ctr_root(const BlockID& root_block_id) = 0;

    virtual void releaseBlock(Shared* block) noexcept = 0;
    virtual void updateBlock(Shared* block) = 0;

    virtual void check_updates_allowed() = 0;
    */
};


}
