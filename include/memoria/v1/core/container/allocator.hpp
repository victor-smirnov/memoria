
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

#include <memoria/v1/profiles/common/common.hpp>

#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/profiles/common/page.hpp>
#include <memoria/v1/core/container/ctr_referenceable.hpp>

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif


#include <memoria/v1/core/graph/graph.hpp>

#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/metadata/container.hpp>

#include <memoria/v1/core/types.hpp>

#include <memory>
#include <typeinfo>

namespace memoria {
namespace v1 {

struct IAllocatorProperties {
    virtual int32_t defaultPageSize() const                                      = 0;

    virtual int64_t lastCommitId() const                                         = 0;
    virtual void setLastCommitId(int64_t txn_id)                                 = 0;
    virtual int64_t newTxnId()                                                   = 0;

    virtual ~IAllocatorProperties() {}
};


struct AllocatorBase {
    virtual ~AllocatorBase() noexcept {}
};


template <typename Profile>
struct IAllocator: AllocatorBase {

    enum {UNDEFINED, READ, UPDATE};

    using MyType    = IAllocator<Profile>;

    using Page          = ProfileBlockType<Profile>;
    using BlockType     = ProfileBlockType<Profile>;
    using ID            = ProfileBlockID<Profile>;
    using BlockID       = ProfileBlockID<Profile>;
    using SnapshotID    = ProfileSnapshotID<Profile>;
    using CtrID         = ProfileCtrID<Profile>;


    using PageG     = PageGuard<Page, MyType>;
    using Shared    = typename PageG::Shared;
    
    virtual BlockID getRootID(const CtrID& ctr_id)                              = 0;
    virtual void setRoot(const CtrID& ctr_id, const BlockID& root)              = 0;
    virtual void markUpdated(const CtrID& ctr_id)                               = 0;

    virtual bool hasRoot(const CtrID& ctr_id)                                   = 0;
    virtual CtrID createCtrName()                                               = 0;


    virtual PageG getPage(const BlockID& id, const CtrID& ctr_if)               = 0;
    virtual PageG getPageForUpdate(const BlockID& id, const CtrID& ctr_id)      = 0;

    virtual PageG updatePage(Shared* shared, const CtrID& ctr_id)               = 0;
    virtual void  removePage(const ID& id, const CtrID& ctr_id)                 = 0;
    virtual PageG createPage(int32_t initial_size, const CtrID& ctr_id)         = 0;

    virtual PageG clonePage(const Shared* shared, const BlockID& new_id, const CtrID& name) = 0;

    virtual void  resizePage(Shared* page, int32_t new_size)                    = 0;
    virtual void  releasePage(Shared* shared) noexcept                          = 0;
    virtual PageG getPageG(BlockType* page)                                     = 0;

    virtual ID newId()                                                          = 0;
    virtual SnapshotID currentTxnId() const                                     = 0;

    // memory pool allocator

    virtual void* allocateMemory(size_t size)                                   = 0;
    virtual void  freeMemory(void* ptr)                                         = 0;

    virtual Logger& logger()                                                    = 0;
    virtual IAllocatorProperties& allocator_properties()                        = 0;

    virtual bool isActive() const                                               = 0;

    virtual void registerCtr(const std::type_info&)                             = 0;
    virtual void unregisterCtr(const std::type_info&)                           = 0;

    virtual SnpSharedPtr<MyType> self_ptr()                                     = 0;
    virtual CtrSharedPtr<CtrReferenceable> get(const CtrID& ctr_id)             = 0;
    virtual CtrSharedPtr<CtrReferenceable> from_root_id(const BlockID& root_page_id, const CtrID& name) = 0;

    virtual bool check()                                                        = 0;
    virtual void walkContainers(ContainerWalker* walker, const char16_t* allocator_descr = nullptr) = 0;

    virtual U16String ctr_type_name(const CtrID& name)                          = 0;

    virtual Vertex allocator_vertex() {
        return Vertex();
    }
    
    virtual ~IAllocator() noexcept {}
};



}}
