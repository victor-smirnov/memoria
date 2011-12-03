
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_ALLOCATOR_HPP
#define	_MEMORIA_CORE_CONTAINER_ALLOCATOR_HPP

#include <memoria/core/container/names.hpp>

#include <memoria/vapi/models/allocator.hpp>



namespace memoria    {

template <typename PageType, int MaxPageSize = 4096>
struct IAbstractAllocator {

	enum {NONE, READ, UPDATE};

	typedef IAbstractAllocator<PageType, MaxPageSize>					MyType;

	typedef PageType													Page;
	typedef typename Page::ID											ID;
	typedef EmptyType													Transaction;

	typedef PageGuard<Page, MyType>										PageG;

	typedef IAbstractAllocator<PageType, MaxPageSize>					AbstractAllocator;

	static const Int PAGE_SIZE											= MaxPageSize;

	virtual PageG GetPage(const ID& id, Int flags)						= 0;
	virtual PageG UpdatePage(Page* page)								= 0;
	virtual void  RemovePage(const ID& id)								= 0;
	virtual PageG CreatePage(Int initial_size = MaxPageSize)			= 0;
	virtual PageG ReallocPage(Page* page, Int new_size)					= 0;
	virtual void  ReleasePage(Page* page)								= 0;

	// Allocator directory interface part
	virtual PageG GetRoot(BigInt name, Int flags)						= 0;
	virtual ID 	  GetRootID(BigInt name)								= 0;
	virtual void  SetRoot(BigInt name, const ID& root) 					= 0;

	virtual Logger& logger()											= 0;
};


template <typename Profile, typename PageType, int MaxPageSize>
class AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType, MaxPageSize> > {
public:
	typedef IAbstractAllocator<PageType, MaxPageSize>					Type;
};


template <typename Profile, typename PageType, int MaxPageSize = 4096>
class ProxyAllocator : public AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType, MaxPageSize> >::Type {

	typedef typename AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType, MaxPageSize> >::Type Base;

	typedef typename Base::ID 										ID;

	Base* target_;

public:

	ProxyAllocator(): target_(NULL) {}

	Base* target() {
		return target_;
	}

	const Base* target() const {
		return target_;
	}

	virtual Page* GetPage(const ID& id) {
		checkTarget();
		return target_->GetPage(id);
	}

	virtual void  RemovePage(const ID& id) {
		checkTarget();
		target_->RemovePage(id);
	}

	virtual Page* CreatePage(Int initial_size = MaxPageSize) {
		checkTarget();
		return target_->CreatePage(initial_size);
	}

	virtual Page* ReallocPage(Page* page, Int new_size) {
		checkTarget();
		return target_->ReallocPage(page, new_size);
	}

	virtual Page* GetRoot(BigInt name) {
		checkTarget();
		return target_->GetRoot(name);
	}

	virtual void  SetRoot(BigInt name, const ID& root) {
		checkTarget();
		target_->SetRoot(name, root);
	}

	virtual Logger& logger() {
		checkTarget();
		return target_->logger();
	}

private:
	void checkTarget(){}
};

}


#endif
