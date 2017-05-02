
// Copyright 2017 Victor Smirnov
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

#include <iostream>
#include <memoria/v1/prototypes/bt_cow/btcow_names.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

namespace memoria {
namespace v1 {


MEMORIA_V1_CONTAINER_PART_BEGIN(v1::btcow::AllocatorName)
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;
    typedef typename Allocator::PageG                                           PageG;
    typedef typename Allocator::Shared                                          Shared;


    bool isNew() const {
        return self().root().is_null();
    }

    virtual void markUpdated(const UUID& name)
    {
        return self().allocator().markUpdated(name);
    }

    virtual UUID currentTxnId() const {
        return self().allocator().currentTxnId();
    }



    virtual PageG getPage(const ID& id, const UUID& name)
    {
        return self().allocator().getPage(id, name);
    }

    virtual PageG getPageForUpdate(const ID& id, const UUID& name)
    {
        return self().allocator().getPageForUpdate(id, name);
    }

    virtual PageG updatePage(Shared* shared, const UUID& name);

    virtual void  removePage(const ID& id, const UUID& name);

    virtual PageG createPage(int32_t initial_size, const UUID& name);


    virtual PageG getPageG(Page* page);

    virtual void  resizePage(Shared* page, int32_t new_size);

    virtual void  releasePage(Shared* shared) noexcept;

    virtual Logger& logger();

    virtual void* allocateMemory(size_t size);

    virtual void freeMemory(void* ptr);

    virtual bool isActive() {
        return self().allocator().isActive();
    }

    virtual UUID createCtrName()
    {
        return self().allocator().createCtrName();
    }

    virtual IAllocatorProperties& properties()
    {
        return self().allocator().properties();
    }

    virtual ID newId()
    {
        return self().allocator().newId();
    }

    virtual void registerCtr(const type_info&) {}
    virtual void unregisterCtr(const type_info&) {}

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::btcow::AllocatorName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::PageG M_TYPE::getPageG(Page* page) {
    return self().allocator().getPageG(page);
}

M_PARAMS
typename M_TYPE::PageG M_TYPE::updatePage(Shared* shared, const UUID& name) {
    return self().allocator().updatePage(shared, name);
}

M_PARAMS
void M_TYPE::removePage(const ID& id, const UUID& name) {
    self().allocator().removePage(id, name);
}

M_PARAMS
typename M_TYPE::PageG M_TYPE::createPage(int32_t initial_size, const UUID& name) {
    return self().allocator().createPage(initial_size, name);
}

M_PARAMS
void M_TYPE::resizePage(Shared* page, int32_t new_size) {
    self().allocator().resizePage(page, new_size);
}

M_PARAMS
void M_TYPE::releasePage(Shared* shared) noexcept {
    self().allocator().releasePage(shared);
}


M_PARAMS
Logger& M_TYPE::logger() {
    return self().allocator().logger();
}

M_PARAMS
void* M_TYPE::allocateMemory(size_t size)
{
    return self().allocator().allocateMemory(size);
}

M_PARAMS
void M_TYPE::freeMemory(void* ptr)
{
    self().allocator().freeMemory(ptr);
}


#undef M_TYPE
#undef M_PARAMS

}}
