
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

#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/core/container/page.hpp>

#include <memoria/v1/metadata/container.hpp>

#include <memory>

namespace memoria {
namespace v1 {


// Ctr directory interface
template <typename ID>
struct ICtrDirectory {
    virtual ID   getRootID(const UUID& name)                                    = 0;
    virtual void setRoot(const UUID& name, const ID& root)                      = 0;
    virtual void markUpdated(const UUID& name)                                  = 0;

    virtual bool hasRoot(const UUID& name)                                      = 0;
    virtual UUID createCtrName()                                                = 0;

    virtual ~ICtrDirectory() {}
};

struct IAllocatorProperties {
    virtual Int defaultPageSize() const                                         = 0;

    virtual BigInt lastCommitId() const                                         = 0;
    virtual void setLastCommitId(BigInt txn_id)                                 = 0;
    virtual BigInt newTxnId()                                                   = 0;

    virtual ~IAllocatorProperties() {}
};


template <typename PageType>
struct IAllocator: ICtrDirectory<typename PageType::ID> {

    enum {UNDEFINED, READ, UPDATE};

    typedef IAllocator<PageType>                                                MyType;

    typedef PageType                                                            Page;
    typedef typename Page::ID                                                   ID;
    typedef EmptyType                                                           Transaction;

    typedef PageGuard<Page, MyType>                                             PageG;
    typedef typename PageG::Shared                                              Shared;


    virtual PageG getPage(const ID& id, const UUID& name)                       = 0;
    virtual PageG getPageForUpdate(const ID& id, const UUID& name)              = 0;

    virtual PageG updatePage(Shared* shared, const UUID& name)                  = 0;
    virtual void  removePage(const ID& id, const UUID&name)                     = 0;
    virtual PageG createPage(Int initial_size, const UUID& name)                = 0;

    virtual void  resizePage(Shared* page, Int new_size)                        = 0;
    virtual void  releasePage(Shared* shared) noexcept                          = 0;
    virtual PageG getPageG(Page* page)                                          = 0;

    virtual ID newId()                                                          = 0;
    virtual UUID currentTxnId() const                                           = 0;

    // memory pool allocator

    virtual void* allocateMemory(size_t size)                                   = 0;
    virtual void  freeMemory(void* ptr)                                         = 0;

    virtual Logger& logger()                                                    = 0;
    virtual IAllocatorProperties& properties()                                  = 0;

    virtual bool isActive()                                                     = 0;

    virtual ~IAllocator() {}
};


template <typename PageType>
struct IWalkableAllocator: IAllocator<PageType> {
    virtual bool check()                                                        = 0;

    virtual void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr) = 0;
};



}}