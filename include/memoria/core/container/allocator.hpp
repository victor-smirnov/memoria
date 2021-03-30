
// Copyright 2013 Victor Smirnov
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

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <memory>
#include <typeinfo>

namespace memoria {

template <typename Profile>
struct IAllocatorBase: AllocatorApiBase<ApiProfile<Profile>> {

    using ApiProfileT = ApiProfile<Profile>;

    enum {UNDEFINED, READ, UPDATE};

    using MyType        = IAllocator<Profile>;

    using BlockType     = ProfileBlockType<Profile>;
    using ID            = ProfileBlockID<Profile>;
    using BlockID       = ProfileBlockID<Profile>;
    using SnapshotID    = ProfileSnapshotID<Profile>;
    using CtrID         = ProfileCtrID<Profile>;


    using BlockG        = typename ProfileTraits<Profile>::BlockGuardT;


    virtual ~IAllocatorBase() noexcept {}
    
    virtual Result<BlockID> getRootID(const CtrID& ctr_id) noexcept = 0;
    virtual VoidResult setRoot(const CtrID& ctr_id, const BlockID& root) noexcept = 0;

    virtual BoolResult hasRoot(const CtrID& ctr_id) noexcept = 0;
    virtual Result<CtrID> createCtrName() noexcept = 0;

    virtual Result<BlockG> getBlock(const BlockID& id) noexcept = 0;

    virtual VoidResult removeBlock(const BlockID& id) noexcept = 0;
    virtual Result<BlockG> createBlock(int32_t initial_size) noexcept = 0;

    virtual Result<BlockG> cloneBlock(const BlockG& block) noexcept = 0;



    virtual Result<BlockID> newId() noexcept = 0;
    virtual SnapshotID currentTxnId() const noexcept = 0;

    // memory pool allocator
    virtual Result<void*> allocateMemory(size_t size) noexcept = 0;
    virtual void freeMemory(void* ptr) noexcept = 0;

    virtual Logger& logger() noexcept = 0;

    virtual bool isActive() const noexcept = 0;

    virtual VoidResult registerCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>* instance) noexcept = 0;
    virtual VoidResult unregisterCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>* instance) noexcept = 0;
    virtual VoidResult flush_open_containers() noexcept = 0;


    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> find(const CtrID& ctr_id) noexcept = 0;
    virtual Result<CtrSharedPtr<CtrReferenceable<ApiProfileT>>> from_root_id(const BlockID& root_block_id, const CtrID& name) noexcept = 0;

    virtual BoolResult check() noexcept = 0;
    virtual VoidResult walkContainers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) noexcept = 0;

    virtual BoolResult drop_ctr(const CtrID& ctr_id) noexcept = 0;

    virtual Result<U8String> ctr_type_name(const CtrID& ctr_id) noexcept = 0;
};


template <typename Profile>
struct IAllocator: IAllocatorBase<Profile> {
    using Base = IAllocatorBase<Profile>;

    using typename Base::BlockG;
    using typename Base::BlockType;

    using Shared = typename BlockG::Shared;


    virtual SnpSharedPtr<IAllocator> self_ptr() noexcept = 0;

    virtual Result<BlockG> updateBlock(Shared* block) noexcept = 0;
    virtual VoidResult resizeBlock(Shared* block, int32_t new_size) noexcept = 0;
    virtual VoidResult releaseBlock(Shared* block) noexcept = 0;
};


template <typename Profile>
struct ICoWAllocator: IAllocatorBase<Profile> {
    using Base = IAllocatorBase<Profile>;

    using typename Base::BlockG;
    using typename Base::BlockID;

    virtual SnpSharedPtr<ICoWAllocator> self_ptr() noexcept = 0;

    virtual VoidResult ref_block(const BlockID& block_id) noexcept = 0;
    virtual VoidResult unref_block(const BlockID& block_id, std::function<VoidResult()> on_zero) noexcept = 0;
    virtual VoidResult unref_ctr_root(const BlockID& root_block_id) noexcept = 0;

    virtual VoidResult traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    ) noexcept = 0;

    virtual VoidResult check_updates_allowed() noexcept = 0;
};

}
