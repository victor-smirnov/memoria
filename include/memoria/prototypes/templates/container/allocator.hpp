
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_ALLOCATOR_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_ALLOCATOR_HPP

#include <iostream>

#include <memoria/prototypes/btree/pages/pages.hpp>
#include <memoria/vapi/models/logs.hpp>

#include <memoria/prototypes/btree/names.hpp>

#include <memoria/core/container/macros.hpp>

namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::AllocatorName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page                                        	Page;
    typedef typename Allocator::Page::ID                                        ID;
    typedef typename Allocator::PageG                                        	PageG;
    typedef typename Allocator::Shared                                        	Shared;
    typedef typename Allocator::CtrShared                                       CtrShared;


    void set_root(const ID &root)
    {
    	me()->shared()->root_log() 	= root;
    	me()->shared()->updated() 	= true;

    	me()->allocator().SetRoot(me()->name(), root);
    }

    const ID &root() const
    {
    	const CtrShared* shared = me()->shared();

    	if (shared->updated())
    	{
    		return shared->root_log();
    	}
    	else {
    		return shared->root();
    	}
    }




    virtual CtrShared* GetCtrShared(BigInt name)
    {
    	return me()->shared()->Get(name);
    }

    virtual void UnregisterCtrShared(CtrShared* shared)
    {
    	me()->shared()->UnregisterChild(shared);
    }

    virtual void RegisterCtrShared(CtrShared* shared)
    {
    	me()->shared()->RegisterChild(shared);
    }

    virtual bool IsCtrSharedRegistered(BigInt name)
    {
    	return me()->shared()->IsChildRegistered(name);
    }


    // Allocator directory interface part
    virtual PageG GetRoot(BigInt name, Int flags)
    {
    	return me()->allocator().GetPage(me()->GetRootID(name), flags);
    }






    virtual PageG GetPage(const ID& id, Int flags);

    virtual PageG GetPageG(Page* page);

    virtual void  UpdatePage(Shared* shared);

    virtual void  RemovePage(const ID& id);

    virtual PageG CreatePage(Int initial_size = Allocator::MaxPageSize);

    virtual void  ResizePage(Shared* page, Int new_size);

    virtual void  ReleasePage(Shared* shared);

    virtual Logger& logger();

    virtual void* AllocateMemory(size_t size);

    virtual void FreeMemory(void* ptr);

    virtual BigInt CreateCtrName()
    {
    	return me()->allocator().CreateCtrName();
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::AllocatorName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::PageG M_TYPE::GetPage(const ID& id, Int flags) {
	return me()->allocator().GetPage(id, flags);
}

M_PARAMS
typename M_TYPE::PageG M_TYPE::GetPageG(Page* page) {
	return me()->allocator().GetPageG(page);
}

M_PARAMS
void M_TYPE::UpdatePage(Shared* shared) {
	me()->allocator().UpdatePage(shared);
}

M_PARAMS
void M_TYPE::RemovePage(const ID& id) {
	me()->allocator().RemovePage(id);
}

M_PARAMS
typename M_TYPE::PageG M_TYPE::CreatePage(Int initial_size) {
	return me()->allocator().CreatePage(initial_size);
}

M_PARAMS
void M_TYPE::ResizePage(Shared* page, Int new_size) {
	me()->allocator().ResizePage(page, new_size);
}

M_PARAMS
void M_TYPE::ReleasePage(Shared* shared) {
	me()->allocator().ReleasePage(shared);
}


M_PARAMS
Logger& M_TYPE::logger() {
	return me()->allocator().logger();
}

M_PARAMS
void* M_TYPE::AllocateMemory(size_t size)
{
	return me()->allocator().AllocateMemory(size);
}

M_PARAMS
void M_TYPE::FreeMemory(void* ptr)
{
	me()->allocator().FreeMemory(ptr);
}


#undef M_TYPE
#undef M_PARAMS






}



#endif	/* _MEMORIA_PROTOTYPES_BTREE_MODEL_CHECKS_HPP */
