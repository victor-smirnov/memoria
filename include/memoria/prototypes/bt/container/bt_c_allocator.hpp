
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <iostream>
#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

namespace memoria    {


MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::AllocatorName)
    
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

    virtual PageG createPage(Int initial_size, const UUID& name);


    virtual PageG getPageG(Page* page);

    virtual void  resizePage(Shared* page, Int new_size);

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

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::AllocatorName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

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
typename M_TYPE::PageG M_TYPE::createPage(Int initial_size, const UUID& name) {
    return self().allocator().createPage(initial_size, name);
}

M_PARAMS
void M_TYPE::resizePage(Shared* page, Int new_size) {
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

}
