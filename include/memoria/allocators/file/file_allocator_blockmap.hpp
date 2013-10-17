
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_ALLOCATORS_FILE_ALLOCATOR_BLOCKMAP_HPP
#define _MEMORIA_ALLOCATORS_FILE_ALLOCATOR_BLOCKMAP_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/page.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/allocators/file/superblock_ctr.hpp>
#include <memoria/core/tools/lru_cache.hpp>


#include <malloc.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>

namespace memoria {

using namespace std;


using namespace memoria::vapi;

template <typename BaseAllocator, typename PageType>
class BlockMapFileAllocator: public IAllocator<PageType> {

    typedef IAllocator<PageType>                                                Base;

public:
    typedef typename Base::Page                                                 Page;
    typedef typename Base::PageG                                                PageG;
    typedef typename Base::Shared                                               Shared;
    typedef typename Base::CtrShared                                            CtrShared;
    typedef typename Page::ID                                                   ID;

    typedef SuperblockCtr<ID>													SuperblockCtrType;


private:
    typedef BlockMapFileAllocator<BaseAllocator, PageType>                      MyType;

    BaseAllocator& allocator_;

public:
    BlockMapFileAllocator(BaseAllocator& base_allocator): allocator_(base_allocator) {}

    virtual ~BlockMapFileAllocator()
    {}

    virtual PageG getPage(const ID& id, Int flags)
    {
    	return allocator_.getPage(id, flags);
    }

    virtual PageG getPageG(Page* page)
    {
    	allocator_.updatePage(page);
    }

    virtual void  updatePage(Shared* shared) {
    	allocator_.updatePage(shared);
    }

    virtual void  removePage(const ID& id) {
    	allocator_.removePage(id);
    }

    virtual PageG createPage(Int initial_size) {
    	return allocator_.createPage(initial_size);
    }

    virtual void  resizePage(Shared* page, Int new_size)
    {
    	allocator_.resizePage(page, new_size);
    }

    virtual void  releasePage(Shared* shared) {
    	allocator_.releasePage(shared);
    }

    virtual CtrShared* getCtrShared(BigInt name)
    {
    	return allocator_.getCtrShared(name);
    }

    virtual bool isCtrSharedRegistered(BigInt name)
    {
    	allocator_.isCtrSharedRegistered(name);
    }

    virtual void unregisterCtrShared(CtrShared* shared)
    {
    	allocator_.unregisterCtrShared(shared);
    }

    virtual void registerCtrShared(CtrShared* shared) {
    	allocator_.registerCtrShared(shared);
    }

    // memory pool allocator

    virtual void* allocateMemory(size_t size) {
    	return allocator_.allocateMemory();
    }

    virtual void  freeMemory(void* ptr) {
    	return allocator_.freeMemory();
    }

    virtual Logger& logger() {return allocator_.logger();}

    virtual const IAllocatorProperties& properties() const {
    	return allocator_.properties();
    }

};

}



#endif
