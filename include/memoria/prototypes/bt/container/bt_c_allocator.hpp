
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BT_CTR_ALLOCATOR_HPP
#define _MEMORIA_PROTOTYPES_BT_CTR_ALLOCATOR_HPP

#include <iostream>
#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

namespace memoria    {


MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::AllocatorName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;
    typedef typename Allocator::PageG                                           PageG;
    typedef typename Allocator::Shared                                          Shared;
    typedef typename Allocator::CtrShared                                       CtrShared;


    void set_root(const ID &root)
    {
        self().shared()->root_log()  = root;
        self().shared()->updated()   = true;

        self().allocator().setRoot(self().name(), root);
    }

    const ID &root() const
    {
        const CtrShared* shared = self().shared();

        if (shared->updated())
        {
            return shared->root_log();
        }
        else {
            return shared->root();
        }
    }

    bool isNew() const {
        return self().shared() == nullptr;
    }


    virtual CtrShared* getCtrShared(BigInt name)
    {
        return self().shared()->get(name);
    }

    virtual void unregisterCtrShared(CtrShared* shared)
    {
        self().shared()->unregisterChild(shared);
    }

    virtual void registerCtrShared(CtrShared* shared)
    {
        self().shared()->registerChild(shared);
    }

    virtual bool isCtrSharedRegistered(BigInt name)
    {
        return self().shared()->isChildRegistered(name);
    }


    // Allocator directory interface part
//    virtual bool hasRoot(BigInt name)
//    {
////        throw vapi::Exception(MA_SRC, "Allocator::hasRoot(BigInt) method must be properly implements for this container");
//    	return isCtrSharedRegistered(name); // Is it correct?
//    }

    virtual void markUpdated(BigInt name)
    {
    	return self().allocator().markUpdated(name);
    }

    virtual BigInt currentTxnId() const {
    	return self().allocator().currentTxnId();
    }



    virtual PageG getPage(const ID& id, BigInt name)
    {
    	return self().allocator().getPage(id, name);
    }

    virtual PageG getPageForUpdate(const ID& id, BigInt name)
    {
    	return self().allocator().getPageForUpdate(id, name);
    }

    virtual PageG updatePage(Shared* shared, BigInt name);

    virtual void  removePage(const ID& id, BigInt name);

    virtual PageG createPage(Int initial_size, BigInt name);


    virtual PageG getPageG(Page* page);

    virtual void  resizePage(Shared* page, Int new_size);

    virtual void  releasePage(Shared* shared) noexcept;

    virtual Logger& logger();

    virtual void* allocateMemory(size_t size);

    virtual void freeMemory(void* ptr);

    virtual BigInt createCtrName()
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
typename M_TYPE::PageG M_TYPE::updatePage(Shared* shared, BigInt name) {
    return self().allocator().updatePage(shared, name);
}

M_PARAMS
void M_TYPE::removePage(const ID& id, BigInt name) {
    self().allocator().removePage(id, name);
}

M_PARAMS
typename M_TYPE::PageG M_TYPE::createPage(Int initial_size, BigInt name) {
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



#endif
