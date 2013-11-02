
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_ALLOCATOR_HPP
#define _MEMORIA_CORE_CONTAINER_ALLOCATOR_HPP

#include <memoria/core/container/names.hpp>
#include <memoria/core/container/ctr_shared.hpp>
#include <memoria/core/container/page.hpp>

#include <memory>

namespace memoria    {


// Ctr directory interface
template <typename ID>
struct ICtrDirectory {
    virtual ID    getRootID(BigInt name)                                		= 0;
    virtual void  setRoot(BigInt name, const ID& root)                  		= 0;
    virtual bool  hasRoot(BigInt name)                                  		= 0;
    virtual BigInt createCtrName()                                      		= 0;

    virtual ~ICtrDirectory() {}
};

struct IAllocatorProperties {
	virtual Int defaultPageSize() const											= 0;

	virtual ~IAllocatorProperties() {}
};


template <typename PageType>
struct IAllocator: ICtrDirectory<typename PageType::ID> {

    enum {READ, UPDATE};

    typedef IAllocator<PageType>                                        		MyType;

    typedef PageType                                                    		Page;
    typedef typename Page::ID                                           		ID;
    typedef EmptyType                                                   		Transaction;

    typedef PageGuard<Page, MyType>                                     		PageG;
    typedef typename PageG::Shared                                      		Shared;

    typedef IAllocator<PageType>                                        		AbstractAllocator;

    typedef ContainerShared<ID>                                         		CtrShared;

    virtual PageG getPage(const ID& id, Int flags)                      		= 0;
    virtual PageG getPageG(Page* page)                                  		= 0;
    virtual void  updatePage(Shared* shared)                            		= 0;
    virtual void  removePage(const ID& id)                              		= 0;
    virtual PageG createPage(Int initial_size)                          		= 0;
    virtual void  resizePage(Shared* page, Int new_size)                		= 0;
    virtual void  releasePage(Shared* shared)                           		= 0;

    virtual CtrShared* getCtrShared(BigInt name)                        		= 0;
    virtual bool isCtrSharedRegistered(BigInt name)                     		= 0;

    virtual void unregisterCtrShared(CtrShared* shared)                 		= 0;
    virtual void registerCtrShared(CtrShared* shared)                   		= 0;

    // memory pool allocator

    virtual void* allocateMemory(size_t size)                           		= 0;
    virtual void  freeMemory(void* ptr)                                 		= 0;

    virtual Logger& logger()                                            		= 0;
    virtual const IAllocatorProperties& properties() const						= 0;
};


template <typename Profile, typename PageType>
class AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType> > {
public:
    typedef IAllocator<PageType>                                                Type;
};


template <typename PageType>
struct ITxn: IAllocator<PageType> {
	virtual void commit() 														= 0;
	virtual void rollback() 													= 0;
};


template <typename PageType>
struct IMVCCTxnMgr {
	typedef ITxn<PageType>														Txn;
	typedef	std::shared_ptr<Txn>												TxnPtr;

	virtual TxnPtr begin() 														= 0;

	virtual ~IMVCCTxnMgr() {}
};






}


#endif
