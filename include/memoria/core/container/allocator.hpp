
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_ALLOCATOR_HPP
#define _MEMORIA_CORE_CONTAINER_ALLOCATOR_HPP

#include <memoria/core/container/names.hpp>
#include <memoria/core/container/ctr_shared.hpp>
#include <memoria/core/container/page.hpp>

#include <memoria/metadata/container.hpp>

#include <memory>

namespace memoria    {


// Ctr directory interface
template <typename ID>
struct ICtrDirectory {
    virtual ID    getRootID(BigInt name)                                		= 0;
    virtual void  setRoot(BigInt name, const ID& root)                  		= 0;
    virtual void markUpdated(BigInt name)										= 0;

    virtual bool  hasRoot(BigInt name)                                  		= 0;
    virtual BigInt createCtrName()                                      		= 0;

    virtual ~ICtrDirectory() {}
};

struct IAllocatorProperties {
	virtual Int defaultPageSize() const											= 0;

	virtual BigInt lastCommitId() const											= 0;
	virtual void setLastCommitId(BigInt txn_id)									= 0;
	virtual BigInt newTxnId()													= 0;

	virtual bool isMVCC() const													= 0;
	virtual void setMVCC(bool mvcc)												= 0;

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



    virtual PageG getPage(const ID& id, Int flags, BigInt name)            		= 0;
    virtual PageG updatePage(Shared* shared, BigInt name)                  		= 0;
    virtual void  removePage(const ID& id, BigInt name)                    		= 0;
    virtual PageG createPage(Int initial_size, BigInt name)                		= 0;


    virtual void  resizePage(Shared* page, Int new_size)                		= 0;
    virtual void  releasePage(Shared* shared)                           		= 0;
    virtual PageG getPageG(Page* page)                                  		= 0;

    virtual CtrShared* getCtrShared(BigInt name)                        		= 0;
    virtual bool isCtrSharedRegistered(BigInt name)                     		= 0;

    virtual void unregisterCtrShared(CtrShared* shared)                 		= 0;
    virtual void registerCtrShared(CtrShared* shared)                   		= 0;

    virtual ID newId()															= 0;
    virtual BigInt currentTxnId() const											= 0;

    // memory pool allocator

    virtual void* allocateMemory(size_t size)                           		= 0;
    virtual void  freeMemory(void* ptr)                                 		= 0;

    virtual Logger& logger()                                            		= 0;
    virtual IAllocatorProperties& properties()									= 0;
};

template <typename PageType>
struct IJournaledAllocator: IAllocator<PageType> {
	virtual void commit(bool force_sync = false)								= 0;
	virtual void rollback(bool force_sync = false)								= 0;
};


template <typename PageType>
struct IWalkableAllocator: IJournaledAllocator<PageType> {
	virtual void walkContainers(vapi::ContainerWalker* walker, const char* allocator_descr = nullptr) = 0;
};


template <typename Profile, typename PageType>
class AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType> > {
public:
    typedef IWalkableAllocator<PageType>                                       	Type;
};


enum class EntryStatus: Int {
	CLEAN, UPDATED, DELETED, CREATED // 2 bits per value
};

static inline Int toInt(EntryStatus es) {
	return static_cast<Int>(es);
}

template <typename PageType>
struct ITxn: IAllocator<PageType> {

	virtual BigInt txn_id()	const												= 0;

	virtual void commit() 														= 0;
	virtual void rollback() 													= 0;
};


template <typename PageType>
struct IMVCCAllocator: public IWalkableAllocator<PageType> {
	typedef IAllocator<PageType>												Base;

	typedef ITxn<PageType>														Txn;
	typedef	std::shared_ptr<Txn>												TxnPtr;

	typedef typename Base::PageG                                     			PageG;
	typedef typename Base::Page                                     			Page;
	typedef typename Base::Shared                                     			Shared;
	typedef typename Base::CtrShared                                     		CtrShared;
	typedef typename Base::ID                                     				ID;

	virtual ~IMVCCAllocator() {}

	virtual PageG getPage(BigInt txn_id, const ID& id, BigInt name)    			= 0;
	virtual ID getCtrDirectoryRootID(BigInt txn_id)								= 0;

	virtual BigInt commited_txn_id()											= 0;

	virtual TxnPtr begin() 														= 0;
};






}


#endif
