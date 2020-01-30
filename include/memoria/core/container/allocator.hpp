
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
struct IAllocator {

    enum {UNDEFINED, READ, UPDATE};

    using MyType        = IAllocator<Profile>;

    using BlockType     = ProfileBlockType<Profile>;
    using ID            = ProfileBlockID<Profile>;
    using BlockID       = ProfileBlockID<Profile>;
    using SnapshotID    = ProfileSnapshotID<Profile>;
    using CtrID         = ProfileCtrID<Profile>;


    using BlockG        = BlockGuard<BlockType, MyType>;
    using Shared        = typename BlockG::Shared;

    virtual ~IAllocator() noexcept {}
    
    virtual Result<BlockID> getRootID(const CtrID& ctr_id) noexcept = 0;
    virtual Result<void> setRoot(const CtrID& ctr_id, const BlockID& root) noexcept = 0;

    virtual Result<bool> hasRoot(const CtrID& ctr_id) noexcept = 0;
    virtual Result<CtrID> createCtrName() noexcept = 0;


    virtual Result<BlockG> getBlock(const BlockID& id) noexcept = 0;
    virtual Result<BlockG> getBlockForUpdate(const BlockID& id) noexcept = 0;

    virtual Result<BlockG> updateBlock(Shared* shared) noexcept = 0;
    virtual Result<void>  removeBlock(const ID& id) noexcept = 0;
    virtual Result<BlockG> createBlock(int32_t initial_size) noexcept = 0;

    virtual Result<BlockG> cloneBlock(const Shared* shared, const BlockID& new_id) noexcept = 0;

    virtual Result<void>  resizeBlock(Shared* block, int32_t new_size) noexcept = 0;
    virtual Result<void>  releaseBlock(Shared* shared) noexcept = 0;
    virtual Result<BlockG> getBlockG(BlockType* block) noexcept = 0;

    virtual Result<ID> newId() noexcept = 0;
    virtual SnapshotID currentTxnId() const noexcept = 0;

    // memory pool allocator

    virtual Result<void*> allocateMemory(size_t size) noexcept = 0;
    virtual void  freeMemory(void* ptr) noexcept = 0;

    virtual Logger& logger() noexcept = 0;

    virtual bool isActive() const noexcept = 0;

    virtual Result<void> registerCtr(const CtrID& ctr_id, CtrReferenceable<Profile>* instance) noexcept = 0;
    virtual Result<void> unregisterCtr(const CtrID& ctr_id, CtrReferenceable<Profile>* instance) noexcept = 0;
    virtual Result<void> flush_open_containers() noexcept = 0;

    virtual SnpSharedPtr<MyType> self_ptr() noexcept = 0;
    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> find(const CtrID& ctr_id) noexcept = 0;
    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> from_root_id(const BlockID& root_block_id, const CtrID& name) noexcept = 0;

    virtual Result<bool> check() noexcept = 0;
    virtual Result<void> walkContainers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) noexcept = 0;

    virtual Result<bool> drop_ctr(const CtrID& ctr_id) noexcept = 0;

    virtual Result<U8String> ctr_type_name(const CtrID& ctr_id) noexcept = 0;

    virtual Result<Vertex> allocator_vertex() noexcept {
        return Result<Vertex>::of();
    }
};

}
