
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_ALLOCATOR_HPP
#define	_MEMORIA_CORE_CONTAINER_ALLOCATOR_HPP

#include <memoria/core/container/names.hpp>
#include <memoria/core/container/ctr_shared.hpp>




namespace memoria    {



template <typename PageType, int MaxPageSize = MAX_BLOCK_SIZE>
struct IAbstractAllocator {

	enum {READ, UPDATE};

	typedef IAbstractAllocator<PageType, MaxPageSize>					MyType;

	typedef PageType													Page;
	typedef typename Page::ID											ID;
	typedef EmptyType													Transaction;

	typedef PageGuard<Page, MyType>										PageG;
	typedef typename PageG::Shared										Shared;

	typedef IAbstractAllocator<PageType, MaxPageSize>					AbstractAllocator;

	typedef ContainerShared<ID>											CtrShared;

	static const Int PAGE_SIZE											= MaxPageSize;

	virtual PageG getPage(const ID& id, Int flags)						= 0;
	virtual PageG getPageG(Page* page)									= 0;
	virtual void  updatePage(Shared* shared)							= 0;
	virtual void  removePage(const ID& id)								= 0;
	virtual PageG createPage(Int initial_size = MaxPageSize)			= 0;
	virtual void  resizePage(Shared* page, Int new_size)				= 0;
	virtual void  releasePage(Shared* shared)							= 0;

	virtual CtrShared* getCtrShared(BigInt name)						= 0;
	virtual bool isCtrSharedRegistered(BigInt name)						= 0;

	virtual void unregisterCtrShared(CtrShared* shared)					= 0;
	virtual void registerCtrShared(CtrShared* shared)					= 0;

	// Allocator directory interface part
	virtual PageG getRoot(BigInt name, Int flags)						= 0;
	virtual ID 	  getRootID(BigInt name)								= 0;
	virtual void  setRoot(BigInt name, const ID& root) 					= 0;

	// memory pool allocator

	virtual void* allocateMemory(size_t size)							= 0;
	virtual void  freeMemory(void* ptr)									= 0;

	virtual BigInt createCtrName()										= 0;

	virtual Logger& logger()											= 0;


};


template <typename Profile, typename PageType, int MaxPageSize>
class AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType, MaxPageSize> > {
public:
	typedef IAbstractAllocator<PageType, MaxPageSize>					Type;
};


}


#endif
