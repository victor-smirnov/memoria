
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

#include <memoria/profiles/common/common.hpp>
#include <memoria/core/container/store.hpp>

#include <memory>

namespace memoria {

template <typename Profile>
class SWMRStoreCommitIDProxy:
        public ProfileStoreType<Profile>,
        public EnableSharedFromThis<SWMRStoreCommitIDProxy<Profile>>
{
    using Base          = ProfileStoreType<Profile>;
    using ApiProfileT   = ApiProfile<Profile>;

    using MyType        = IStoreBase<Profile>;

    using BlockType     = ProfileBlockType<Profile>;
    using ID            = ProfileBlockID<Profile>;
    using BlockID       = ProfileBlockID<Profile>;
    using BlockGUID     = ProfileBlockGUID<Profile>;
    using SnapshotID    = ProfileSnapshotID<Profile>;
    using CtrID         = ProfileCtrID<Profile>;
    using CommitID      = ProfileSnapshotID<Profile>;
    using StoreT        = ProfileStoreType<Profile>;

    using SharedBlockPtr        = typename ProfileTraits<Profile>::SharedBlockPtr;
    using SharedBlockConstPtr   = typename ProfileTraits<Profile>::SharedBlockConstPtr;

    using Shared        = typename SharedBlockPtr::Shared;
    using Delegate      = ProfileStoreType<Profile>;

    Delegate* delegate_;
    CommitID commit_id_;
public:
    SWMRStoreCommitIDProxy(Delegate* delegate, const CommitID& commit_id) noexcept :
        delegate_(delegate), commit_id_(commit_id)
    {}

    // Overloaded methods

    virtual SnpSharedPtr<StoreT> self_ptr() noexcept {
        return this->shared_from_this();
    }

    virtual SnapshotID currentTxnId() const {
        return commit_id_;
    }

    // Delegated methods

    virtual BlockID getRootID(const CtrID& ctr_id) {
        return delegate_->getRootID(ctr_id);
    }

    virtual void setRoot(const CtrID& ctr_id, const BlockID& root) {
        return delegate_->setRoot(ctr_id, root);
    }

    virtual bool hasRoot(const CtrID& ctr_id) {
        return delegate_->hasRoot(ctr_id);
    }

    virtual CtrID createCtrName() {
        return delegate_->createCtrName();
    }

    virtual SharedBlockConstPtr getBlock(const BlockID& id) {
        return delegate_->getBlock(id);
    }

    virtual void removeBlock(const BlockID& id) {
        return delegate_->removeBlock(id);
    }

    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID& ctr_id) {
        return delegate_->createBlock(initial_size, ctr_id);
    }

    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID& ctr_id) {
        return delegate_->cloneBlock(block, ctr_id);
    }

    virtual BlockID newId() {
        return delegate_->newId();
    }



    // memory pool allocator
    virtual void* allocateMemory(size_t size) {
        return delegate_->allocateMemory(size);
    }

    virtual void freeMemory(void* ptr) noexcept {
        return delegate_->freeMemory(ptr);
    }

    virtual bool isActive() const {
        return delegate_->isActive();
    }

    virtual void registerCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>* instance) {
        return delegate_->registerCtr(ctr_id, instance);
    }

    virtual void unregisterCtr(const CtrID& ctr_id, CtrReferenceable<ApiProfileT>* instance) {
        return delegate_->unregisterCtr(ctr_id, instance);
    }

    virtual void flush_open_containers() {
        return delegate_->flush_open_containers();
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id) {
        return delegate_->find(ctr_id);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> from_root_id(const BlockID& root_block_id) {
        return delegate_->from_root_id(root_block_id);
    }

    virtual bool check() {
        return delegate_->check();
    }

    virtual void walkContainers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) {
        return delegate_->walkContainers(walker, allocator_descr);
    }

    virtual bool drop_ctr(const CtrID& ctr_id) {
        return delegate_->drop_ctr(ctr_id);
    }

    virtual U8String ctr_type_name(const CtrID& ctr_id) {
        return delegate_->ctr_type_name(ctr_id);
    }




    virtual void updateBlock(Shared* block) {
        return delegate_->updateBlock(block);
    }

//    virtual void resizeBlock(Shared* block, int32_t new_size) {
//        return delegate_->resizeBlock(block, new_size);
//    }

    virtual void releaseBlock(Shared* block) noexcept {
        return delegate_->releaseBlock(block);
    }

//    virtual ObjectPools& object_pools() const noexcept {
//        return delegate_->object_pools();
//    }

    virtual void ref_block(const BlockID& block_id) {
        return delegate_->ref_block(block_id);
    }

    virtual void unref_block(const BlockID& block_id, std::function<void ()> on_zero) {
        return delegate_->unref_block(block_id, on_zero);
    }
    virtual void unref_ctr_root(const BlockID& root_block_id) {
        return delegate_->unref_ctr_root(root_block_id);
    }

    virtual void traverse_ctr(
            const BlockID& root_block,
            BTreeTraverseNodeHandler<Profile>& node_handler
    ) {
        return delegate_->traverse_ctr(root_block, node_handler);
    }

    virtual void check_updates_allowed() {
        return delegate_->check_updates_allowed();
    }

};


}
