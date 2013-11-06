
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_ALLOCATOR_MVCC_TOOLS_HPP_
#define MEMORIA_ALLOCATOR_MVCC_TOOLS_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/exceptions/memoria.hpp>

#include <memoria/core/container/allocator.hpp>

#include <unordered_map>

namespace memoria {

template <typename Base, typename Allocator = IAllocator<typename Base::Page>>
class MVCCAllocatorBase: public Base {


public:
	typedef typename Base::Page													Page;
	typedef typename Base::PageG												PageG;
	typedef typename Base::ID													ID;
	typedef typename Base::CtrShared											CtrShared;
	typedef typename Base::Shared												Shared;


	typedef std::unordered_map<BigInt, CtrShared*>                              CtrSharedMap;

protected:

	CtrSharedMap ctr_shared_;

	Allocator* allocator_;

public:
	MVCCAllocatorBase(Allocator* allocator): allocator_(allocator) {}

	virtual ~MVCCAllocatorBase() = default;

	Allocator* allocator() {
		return allocator_;
	}

	virtual PageG getPageG(Page* page) {
		return allocator_->getPageG(page);
	}

    virtual CtrShared* getCtrShared(BigInt name)
    {
    	auto i = ctr_shared_.find(name);

    	if (i != ctr_shared_.end())
    	{
    		return i->second;
    	}
    	else
    	{
    		throw Exception(MEMORIA_SOURCE, SBuf()<<"Unknown CtrShared requested for name "<<name);
    	}
    }

    virtual bool isCtrSharedRegistered(BigInt name)
    {
    	return ctr_shared_.find(name) != ctr_shared_.end();
    }

    virtual void unregisterCtrShared(CtrShared* shared)
    {
    	ctr_shared_.erase(shared->name());
    }

    virtual void registerCtrShared(CtrShared* shared)
    {
    	BigInt name = shared->name();

    	auto i = ctr_shared_.find(name);

    	if (i == ctr_shared_.end())
    	{
    		ctr_shared_[name] = shared;
    	}
    	else if (!i->second)
    	{
    		i->second = shared;
    	}
    	else
    	{
    		throw Exception(MA_SRC, SBuf()<<"CtrShared for name "<<name<<" is already registered");
    	}
    }

    virtual BigInt createCtrName()
    {
    	return allocator_->createCtrName();
    }

    virtual ID newId()
    {
    	return allocator_->newId();
    }

    // memory pool allocator

    virtual void* allocateMemory(size_t size)
    {
    	return allocator_->allocateMemory(size);
    }

    virtual void freeMemory(void* ptr) {
    	allocator_->freeMemory(ptr);
    }

    virtual Logger& logger() {
    	return allocator_->logger();
    }

    virtual IAllocatorProperties& properties()
    {
    	return allocator_->properties();
    }

    virtual void commit(bool force_sync = false)
    {
    	allocator_->commit(force_sync);
    }

    virtual void rollback(bool force_sync = false)
    {
    	allocator_->rollback(force_sync);
    }
};

}


#endif
