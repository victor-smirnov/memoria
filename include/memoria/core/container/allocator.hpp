
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
    virtual ID   getRootID(const UUID& name)                                    = 0;
    virtual void setRoot(const UUID& name, const ID& root)                      = 0;
    virtual void markUpdated(const UUID& name)                                  = 0;

    virtual bool hasRoot(const UUID& name)                                      = 0;
    virtual UUID createCtrName()                                                = 0;

    virtual ~ICtrDirectory() {}
};

struct IAllocatorProperties {
    virtual Int defaultPageSize() const                                         = 0;

    virtual BigInt lastCommitId() const                                         = 0;
    virtual void setLastCommitId(BigInt txn_id)                                 = 0;
    virtual BigInt newTxnId()                                                   = 0;

    virtual ~IAllocatorProperties() {}
};


template <typename PageType>
struct IAllocator: ICtrDirectory<typename PageType::ID> {

    enum {UNDEFINED, READ, UPDATE};

    typedef IAllocator<PageType>                                                MyType;

    typedef PageType                                                            Page;
    typedef typename Page::ID                                                   ID;
    typedef EmptyType                                                           Transaction;

    typedef PageGuard<Page, MyType>                                             PageG;
    typedef typename PageG::Shared                                              Shared;

    typedef IAllocator<PageType>                                                AbstractAllocator;

    typedef ContainerShared<ID>                                                 CtrShared;


    virtual PageG getPage(const ID& id, const UUID& name)                       = 0;
    virtual PageG getPageForUpdate(const ID& id, const UUID& name)              = 0;


    virtual PageG updatePage(Shared* shared, const UUID& name)                  = 0;
    virtual void  removePage(const ID& id, const UUID&name)                     = 0;
    virtual PageG createPage(Int initial_size, const UUID& name)                = 0;


    virtual void  resizePage(Shared* page, Int new_size)                        = 0;
    virtual void  releasePage(Shared* shared) noexcept                          = 0;
    virtual PageG getPageG(Page* page)                                          = 0;

    virtual CtrShared* getCtrShared(const UUID& name)                           = 0;
    virtual bool isCtrSharedRegistered(const UUID& name)                        = 0;

    virtual void unregisterCtrShared(CtrShared* shared)                         = 0;
    virtual void registerCtrShared(CtrShared* shared)                           = 0;

    virtual ID newId()                                                          = 0;
    virtual BigInt currentTxnId() const                                         = 0;

    // memory pool allocator

    virtual void* allocateMemory(size_t size)                                   = 0;
    virtual void  freeMemory(void* ptr)                                         = 0;

    virtual Logger& logger()                                                    = 0;
    virtual IAllocatorProperties& properties()                                  = 0;

    virtual ~IAllocator() {}

};

template <typename PageType>
struct IJournaledAllocator: IAllocator<PageType> {
    virtual void flush(bool force_sync = false)                                 = 0;
    virtual void rollback(bool force_sync = false)                              = 0;
    virtual bool check()                                                        = 0;
};


template <typename PageType>
struct IWalkableAllocator: IJournaledAllocator<PageType> {
    virtual void walkContainers(vapi::ContainerWalker* walker, const char* allocator_descr = nullptr) = 0;
};


template <typename Profile, typename PageType>
class AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType> > {
public:
    typedef IWalkableAllocator<PageType>                                        Type;
};


enum class EntryStatus: Int {
    CLEAN, UPDATED, DELETED, CREATED // 2 bits per value
};

enum class TxnStatus: Int {
    ACTIVE, COMMITED, SNAPSHOT
};


//static inline Int toInt(EntryStatus es) {
//    return static_cast<Int>(es);
//}
//
//static inline Int toInt(TxnStatus es) {
//    return static_cast<Int>(es);
//}

//template <typename PageType>
//struct ITxn: IWalkableAllocator<PageType> {
//
//    virtual ~ITxn() = default;
//
//    virtual BigInt txn_id() const                                               = 0;
//    virtual TxnStatus status() const                                            = 0;
//
//    virtual BigInt commit()                                                     = 0;
//    virtual void rollback()                                                     = 0;
//    virtual void flush(bool force_sync = false)                                 = 0;
//    virtual bool check()                                                        = 0;
//
//    virtual void setSnapshot(bool snapshot)                                     = 0;
//    virtual bool is_snapshot() const                                            = 0;
//};


//template <typename Owner>
//struct ITxnIterator {
//    virtual ~ITxnIterator() = default;
//
//    virtual BigInt txn_id() const                                               = 0;
//    virtual TxnStatus status() const                                            = 0;
//    virtual void remove()                                                       = 0;
//    virtual bool has_next() const                                               = 0;
//    virtual void next()                                                         = 0;
//    virtual typename Owner::TxnPtr txn()                                        = 0;
//};
//
//template <typename PageType>
//struct IMVCCAllocator: public IWalkableAllocator<PageType> {
//
//    typedef IMVCCAllocator<PageType>                                            MyType;
//
//    typedef IAllocator<PageType>                                                Base;
//
//    typedef ITxn<PageType>                                                      Txn;
//    typedef ITxnIterator<MyType>                                                TxnIterator;
//
//    typedef std::shared_ptr<Txn>                                                TxnPtr;
//    typedef std::weak_ptr<Txn>                                                  WeakTxnPtr;
//    typedef std::shared_ptr<TxnIterator>                                        TxnIteratorPtr;
//
//    typedef typename Base::PageG                                                PageG;
//    typedef typename Base::Page                                                 Page;
//    typedef typename Base::Shared                                               Shared;
//    typedef typename Base::CtrShared                                            CtrShared;
//    typedef typename Base::ID                                                   ID;
//
//    virtual ~IMVCCAllocator() {}
//
//    virtual PageG getPage(BigInt txn_id, const ID& id, BigInt name)             = 0;
//
//    virtual ID getCtrDirectoryRootID(BigInt txn_id)                             = 0;
//
//    virtual ID getTxnUpdateHistoryRootID(BigInt txn_id)                         = 0;
//    virtual void setTxnUpdateHistoryRootID(BigInt txn_id, const ID& id)         = 0;
//    virtual bool hasTxnUpdateHistoryRootID(BigInt txn_id)                       = 0;
//
//
//    virtual BigInt commited_txn_id()                                            = 0;
//
//    virtual TxnPtr begin()                                                      = 0;
//    virtual TxnPtr findTxn(BigInt txn_id)                                       = 0;
//
//    virtual void flush(bool force_sync = false)                                 = 0;
//
//    virtual TxnIteratorPtr transactions(TxnStatus status)                       = 0;
//    virtual BigInt total_transactions(TxnStatus status)                         = 0;
//};


//
//template <typename Allocator>
//class JournaledAllocatorProxy: public Allocator {
//
//
//public:
//    typedef Allocator                                                           Base;
//
//    typedef typename Base::Page                                                 Page;
//    typedef typename Base::PageG                                                PageG;
//    typedef typename Base::ID                                                   ID;
//    typedef typename Base::CtrShared                                            CtrShared;
//    typedef typename Base::Shared                                               Shared;
//
//protected:
//
//    Allocator* allocator_;
//
//public:
//    JournaledAllocatorProxy(Allocator* allocator): allocator_(allocator) {}
//
//    virtual ~JournaledAllocatorProxy() = default;
//
//    Allocator* allocator() {
//        return allocator_;
//    }
//
//    virtual PageG getPageG(Page* page) {
//        return allocator_->getPageG(page);
//    }
//
//    virtual CtrShared* getCtrShared(BigInt name)
//    {
//        return allocator_->getCtrShared(name);
//    }
//
//    virtual bool isCtrSharedRegistered(BigInt name)
//    {
//        return allocator_->isCtrSharedRegistered(name);
//    }
//
//    virtual void unregisterCtrShared(CtrShared* shared)
//    {
//        allocator_->unregisterCtrShared(shared);
//    }
//
//    virtual void registerCtrShared(CtrShared* shared)
//    {
//        allocator_->registerCtrShared(shared);
//    }
//
//    virtual BigInt createCtrName()
//    {
//        return allocator_->createCtrName();
//    }
//
//    virtual BigInt currentTxnId() const {
//        return allocator_->currentTxnId();
//    }
//
//    virtual ID newId()
//    {
//        return allocator_->newId();
//    }
//
//    // memory pool allocator
//
//    virtual void* allocateMemory(size_t size)
//    {
//        return allocator_->allocateMemory(size);
//    }
//
//    virtual void freeMemory(void* ptr) {
//        allocator_->freeMemory(ptr);
//    }
//
//    virtual Logger& logger() {
//        return allocator_->logger();
//    }
//
//    virtual IAllocatorProperties& properties()
//    {
//        return allocator_->properties();
//    }
//
//    virtual void flush(bool force_sync = false)
//    {
//        allocator_->flush(force_sync);
//    }
//
//    virtual void rollback(bool force_sync = false)
//    {
//        allocator_->rollback(force_sync);
//    }
//
//    virtual ID getRootID(BigInt name) {
//        return allocator_->getRootID(name);
//    }
//
//    virtual void setRoot(BigInt name, const ID& root)
//    {
//        allocator_->setRoot(name, root);
//    }
//
//    virtual bool hasRoot(BigInt name)
//    {
//        return allocator_->hasRoot(name);
//    }
//
//    virtual void markUpdated(BigInt name)
//    {
//        allocator_->markUpdated(name);
//    }
//
//    virtual PageG getPage(const ID& id, BigInt name)
//    {
//        return allocator_->getPage(id, name);
//    }
//
//    virtual PageG getPageForUpdate(const ID& id, BigInt name)
//    {
//        return allocator_->getPageForUpdate(id, name);
//    }
//
//
//    virtual PageG updatePage(Shared* shared, BigInt name) {
//        return allocator_->updatePage(shared, name);
//    }
//
//    virtual void removePage(const ID& id, BigInt name) {
//        allocator_->removePage(id, name);
//    }
//
//    virtual PageG createPage(Int initial_size, BigInt name)
//    {
//        return allocator_->createPage(initial_size, name);
//    }
//
//
//    virtual void resizePage(Shared* page, Int new_size) {
//        allocator_->resizePage(page, new_size);
//    }
//
//    virtual void releasePage(Shared* shared) noexcept
//    {
//        allocator_->releasePage(shared);
//    }
//
//    virtual bool check() {
//        return allocator_->check();
//    }
//};
//
//
//
//template <typename Allocator>
//class WalkableAllocatorProxy: public JournaledAllocatorProxy<Allocator> {
//    typedef JournaledAllocatorProxy<Allocator>                              Base;
//
//
//public:
//    WalkableAllocatorProxy(Allocator* allocator): Base(allocator) {}
//
//    virtual ~WalkableAllocatorProxy() = default;
//
//
//    virtual void walkContainers(vapi::ContainerWalker* walker, const char* allocator_descr = nullptr)
//    {
//        Base::allocator()->walkContainers(walker, allocator_descr);
//    }
//};


}


#endif
