
// Copyright 2013-2025 Victor Smirnov
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

#include <memoria/profiles/common/block.hpp>
#include <memoria/core/container/ctr_referenceable.hpp>

#include <memoria/core/memory/memory.hpp>
#include <memoria/profiles/common/container_operations.hpp>

#include <memoria/core/tools/result.hpp>
#include <memoria/core/memory/object_pool.hpp>
#include <memoria/core/tools/checks.hpp>

#include <memory>
#include <typeinfo>

namespace memoria {

template <typename Profile>
struct IStoreBase: IStoreApiBase<ApiProfile<Profile>> {

    using ApiProfileT   = ApiProfile<Profile>;

    using MyType        = IStoreBase<Profile>;

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


    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID& ctr_id) = 0;
    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID& ctr_id) = 0;

    virtual BlockID newId() = 0;
    virtual SnapshotID snaphsot_Id() const = 0;

    // memory pool allocator
    virtual void* allocateMemory(size_t size) = 0;
    virtual void freeMemory(void* ptr) noexcept = 0;

    virtual bool isActive() const = 0;

    virtual void flush_open_containers() = 0;

    virtual void on_ctr_drop(const CtrID& ctr_id) = 0;

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id) = 0;
    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> from_root_id(const BlockID& root_block_id) = 0;

    virtual void check(const CheckResultConsumerFn& consumer) = 0;
    virtual void check_storage(SharedBlockConstPtr block, const CheckResultConsumerFn& consumer) = 0;

    virtual void walkContainers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) = 0;

    virtual bool drop_ctr(const CtrID& ctr_id) = 0;

    virtual U8String ctr_type_name(const CtrID& ctr_id) = 0;

    virtual ObjectPools& object_pools() const = 0;

    virtual void start_no_reentry(const CtrID& ctr_id) = 0;
    virtual void finish_no_reentry(const CtrID& ctr_id) noexcept = 0;

    void with_no_reentry(const CtrID& ctr_id, const std::function<void ()>& fn) {
        start_no_reentry(ctr_id);
        try {
            fn();
            finish_no_reentry(ctr_id);
        }
        catch (...) {
            finish_no_reentry(ctr_id);
            throw;
        }
    }

    template <typename T>
    T with_no_reentry(const CtrID& ctr_id, const std::function<T ()>& fn) {
        start_no_reentry(ctr_id);
        try {
            auto rtn = fn();
            finish_no_reentry(ctr_id);
            return std::move(rtn);
        }
        catch (...) {
            finish_no_reentry(ctr_id);
            throw;
        }
    }

    virtual bool is_allocated(const BlockID& block_id) {
        return true;
    }
};





template <typename Profile>
struct IStore: IStoreBase<Profile> {
    using Base = IStoreBase<Profile>;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockType;
    using typename Base::CtrID;
    using typename Base::BlockID;

    using Shared = typename SharedBlockPtr::Shared;

    virtual SnpSharedPtr<IStore> self_ptr() = 0;

    virtual void updateBlock(Shared* block) = 0;
    virtual void resizeBlock(Shared* block, int32_t new_size) = 0;
    virtual void releaseBlock(Shared* block) noexcept = 0;
    virtual void removeBlock(const BlockID& id) = 0;
};



template <typename Profile>
struct ICowStore: IStoreBase<Profile> {
    using Base = IStoreBase<Profile>;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockID;
    using typename Base::CtrID;

    using Shared = typename SharedBlockPtr::Shared;

    virtual SnpSharedPtr<ICowStore> self_ptr() = 0;

    virtual void ref_block(const BlockID& block_id) = 0;
    virtual void unref_block(const BlockID& block_id) = 0;

    virtual void releaseBlock(Shared* block) noexcept = 0;
    virtual void updateBlock(Shared* block) = 0;

    virtual void traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    ) = 0;

    virtual void check_updates_allowed() = 0;

    virtual bool cow_has_counters() const {return true;}
};


}
