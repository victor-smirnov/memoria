
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_ALLOCATOR_MVCC_TXNMGR_HPP_
#define MEMORIA_ALLOCATOR_MVCC_TXNMGR_HPP_

#include <memoria/core/tools/pool.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/file.hpp>
#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/types/types.hpp>

#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/page.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/allocators/mvcc/mvcc_txn.hpp>
#include <memoria/allocators/mvcc/mvcc_txn_ro.hpp>
#include <memoria/allocators/mvcc/mvcc_tools.hpp>

#include <memoria/core/exceptions/memoria.hpp>
#include <memoria/metadata/tools.hpp>



#include <map>
#include <memory>

namespace memoria {

template <typename Profile, typename PageType>
class MVCCAllocator: public MVCCAllocatorBase<IMVCCAllocator<PageType>> {
    typedef MVCCAllocatorBase<IMVCCAllocator<PageType>>                         Base;

public:
    typedef MVCCAllocator<Profile, PageType>                                    MyType;

    typedef IMVCCAllocator<PageType>                                            BaseAllocator;
    typedef IWalkableAllocator<PageType>                                        Allocator;

    typedef typename Base::ID                                                   ID;
    typedef typename Base::PageG                                                PageG;
    typedef typename Base::Txn                                                  Txn;
    typedef typename Base::TxnPtr                                               TxnPtr;
    typedef typename Base::WeakTxnPtr                                           WeakTxnPtr;
    typedef typename Base::TxnIterator                                          TxnIterator;
    typedef typename Base::TxnIteratorPtr                                       TxnIteratorPtr;


    typedef typename Base::CtrShared                                            CtrShared;
    typedef typename Base::Shared                                               Shared;
    typedef typename Base::Shared*                                              PageSharedPtr;

    typedef MVCCTxn<Profile, MyType, PageType>                                  TxnImpl;
    typedef MVCCReadOnlyTxn<Profile, MyType, PageType>                          ReadOnlyTxnImpl;

    typedef typename CtrTF<Profile, DblMrkMap2<BigInt, ID, 2>>::Type            CommitHistory;
    typedef typename CtrTF<Profile, Map<BigInt, ID>>::Type                      Roots;
    typedef typename CtrTF<Profile, SMrkMap<BigInt, ID, 2>>::Type               TxnHistory;
    typedef typename CtrTF<Profile, Map<BigInt, UByte>>::Type                   TxnLog;

    typedef std::unique_ptr<TxnLog>                                             TxnLogPtr;

    typedef typename TxnHistory::Types::Value                                   TxnHistoryValue;
    typedef typename TxnLog::Types::Value                                       TxnLogValue;



    typedef typename CommitHistory::Types::Value                                CommitHistoryValue;
    typedef std::pair<BigInt, CommitHistoryValue>                               CommitHistoryEntry;

    typedef std::unordered_map<BigInt, CtrShared*>                              CtrSharedMap;

    typedef TxnValue<ID>                                                        CtrDirectoryTxnValue;

    typedef typename CtrTF<Profile, SMrkMap<BigInt, CtrDirectoryTxnValue, 2>>::Type CtrDirectory;
    typedef typename CtrDirectory::Types::Value                                 CtrDirectoryValue;
    typedef std::unique_ptr<CtrDirectory>                                       CtrDirectoryPtr;

    static const Int NameBase                                                   = 1023;

    static const Int CtrDirectoryName                                           = NameBase + 1;
    static const Int CommitHistoryName                                          = NameBase + 2;
    static const Int RootsName                                                  = NameBase + 3;
    static const Int TxnHistoryName                                             = NameBase + 4;

    template <typename, typename, typename> friend class MVCCTxn;
    template <typename, typename, typename> friend class MVCCReadOnlyTxn;

    class MVCCPageShared;
    typedef MVCCPageShared*                                                     MVCCPageSharedPtr;

    typedef std::unordered_map<ID, MVCCPageSharedPtr, IDKeyHash, IDKeyEq>       PageSharedMap;


    class MVCCPageShared: public Shared {

        MVCCPageSharedPtr next_;
        MVCCPageSharedPtr prev_;

    public:

        MVCCPageShared()
        {
            init();
        }

        void init()
        {
            Shared::init();

            next_ = prev_ = nullptr;
        }

        MVCCPageSharedPtr& next() {
            return next_;
        }

        const MVCCPageSharedPtr& next() const {
            return next_;
        }

        MVCCPageSharedPtr& prev() {
            return prev_;
        }

        const MVCCPageSharedPtr& prev() const {
            return prev_;
        }
    };



    class SharedPool: public IntrusiveList<MVCCPageShared> {
        typedef IntrusiveList<MVCCPageShared>   Base;
        typedef typename Base::size_type        size_type;

        size_type max_size_;

    public:
        SharedPool() = default;

        SharedPool(size_type size, size_type max_size): max_size_(max_size)
        {
            for (Int c = 0; c < size; c++)
            {
                this->insert(this->begin(), new MVCCPageShared());
            }
        }

        size_type max_size() const
        {
            return max_size_;
        }

        void put(MVCCPageShared* shared)
        {
            this->insert(this->begin(), shared);
        }
    };

private:

    class TxnLogAllocator: public JournaledAllocatorProxy<IJournaledAllocator<PageType>> {
        typedef JournaledAllocatorProxy<IJournaledAllocator<PageType>>          Base;

        TxnLog& txn_log_;

    public:
        TxnLogAllocator(Allocator* allocator, TxnLog& txn_log):
            Base(allocator),
            txn_log_(txn_log)
        {}

        virtual PageG getPageForUpdate(const ID& id, BigInt name)
        {
            auto iter = txn_log_.findKeyGE(id);

            if (!iter.is_found_eq(id))
            {
                iter.insert(id, 0);
            }

            return this->allocator()->getPageForUpdate(id, name);
        }


        virtual PageG updatePage(Shared* shared, BigInt name)
        {
            PageG page = this->allocator()->updatePage(shared, name);

            ID id = page->id();

            auto iter = txn_log_.findKeyGE(id);

            if (!iter.is_found_eq(id))
            {
                iter.insert(id, 0);
            }

            return page;
        }

        virtual void removePage(const ID& id, BigInt name)
        {
            this->allocator()->removePage(id, name);
        }

        virtual PageG createPage(Int initial_size, BigInt name)
        {
            PageG page = this->allocator()->createPage(initial_size, name);

            ID id = page->id();

            auto iter = txn_log_.findKeyGE(id);

            MEMORIA_ASSERT_FALSE(iter.is_found_eq(id));

            iter.insert(id, 0);

            return page;
        }
    };

    bool            compactify_history_ = true;
    std::map<BigInt, WeakTxnPtr> transactions_;

    TxnUpdateAllocatorProxy<MyType, PageType> update_allocator_proxy_;

    Int             commit_history_cnt_ = 0;

    // memory pool for PageShared objects
    SharedPool      shared_pool_;
    PageSharedMap   allocated_shared_objects_;

    ContainerMetadataRepository* metadata_;

    Allocator*      allocator_;

    CommitHistory   commit_history_;
    Roots           roots_;
    TxnHistory      txn_history_;

    CtrDirectoryPtr ctr_directory_;

    BigInt          last_commited_txn_id_;

public:

    MVCCAllocator(Allocator* allocator):
        Base(allocator),
        update_allocator_proxy_(this),
        shared_pool_(256, 256),
        metadata_(MetadataRepository<Profile>::getMetadata()),
        allocator_(allocator),
        commit_history_(allocator, CTR_CREATE | CTR_FIND, CommitHistoryName),
        roots_(allocator, CTR_CREATE | CTR_FIND, RootsName),
        txn_history_(allocator, CTR_CREATE | CTR_FIND, TxnHistoryName)
    {
        initMetadata();

        if (commit_history_.size() == 0)
        {
            // create initial CtrDirectory
            last_commited_txn_id_ = allocator_->properties().newTxnId();

            ctr_directory_ = CtrDirectoryPtr(new CtrDirectory(this, CTR_CREATE, CtrDirectoryName));

            allocator_->properties().setLastCommitId(last_commited_txn_id_);

            allocator_->properties().setMVCC(true);
        }
        else {
            last_commited_txn_id_ = allocator_->properties().lastCommitId();

            ctr_directory_ = CtrDirectoryPtr(new CtrDirectory(this, CTR_FIND, CtrDirectoryName));
        }
    }

    virtual ~MVCCAllocator() {}

    CtrDirectory* roots()
    {
        return ctr_directory_.get();
    }

    TxnHistory& txn_history()
    {
        return txn_history_;
    }

    static void initMetadata()
    {
        CommitHistory::initMetadata();
        Roots::initMetadata();
        CtrDirectory::initMetadata();
        TxnImpl::UpdateLog::initMetadata();
        TxnHistory::initMetadata();
        TxnLog::initMetadata();
    }

    ContainerMetadataRepository* getMetadata() const {
        return metadata_;
    }

    bool is_compactify_history() const
    {
        return compactify_history_;
    }

    void setCompactifyHistory(bool compactify)
    {
        compactify_history_ = compactify;
    }


    virtual PageG getPage(BigInt txn_id, const ID& id, BigInt name)
    {
        auto iter = commit_history_.find(id);

        if (iter.is_found_eq(id))
        {
            if (iter.find2ndLE(txn_id))
            {
                ID gid = iter.value().second;

                PageG page = allocator_->getPage(gid, name);

                MEMORIA_WARNING(page->id(), !=, id);

                return wrapPageG(page);
            }
            else {
                throw vapi::Exception(
                        MA_SRC,
                        SBuf()<<"Page with id="<<id<<" is not found in commit history for txn_id="<<txn_id
                );
            }
        }
        else {
            throw vapi::Exception(MA_SRC, SBuf()<<"Page with id="<<id<<" is not found in commit history");
        }
    }

    virtual ID getCtrDirectoryRootID(BigInt txn_id)
    {
        auto iter = roots_.findKeyLE(txn_id);

        if (iter.is_found_le(txn_id))
        {
            return iter.value();
        }
        else {
            throw Exception(MA_SRC, SBuf()<<"No container directory root ID for txn: "<<txn_id);
        }
    }


    virtual ID getTxnUpdateHistoryRootID(BigInt txn_id)
    {
        auto iter = txn_history_.findKeyGE(txn_id);

        if (iter.is_found_eq(txn_id))
        {
            return iter.value();
        }
        else {
            throw Exception(MA_SRC, SBuf()<<"No UpdateLog root ID for txn: "<<txn_id);
        }
    }

    virtual void setTxnUpdateHistoryRootID(BigInt txn_id, const ID& id)
    {
        auto iter = txn_history_.findKeyGE(txn_id);

        if (iter.is_found_eq(txn_id))
        {
            if (id.isSet())
            {
                iter.svalue() = id;
            }
            else {
                iter.remove();
            }
        }
        else if (id.isSet())
        {
            using Entry = typename TxnHistory::Types::Entry;

            Entry entry;

            entry.key()     = txn_id;
            entry.value()   = id;
            std::get<0>(entry.labels()) = toInt(TxnStatus::ACTIVE);

            iter.insert(entry);

//          iter.insert(txn_id, id, toInt(TxnStatus::ACTIVE));
        }
    }

    virtual bool hasTxnUpdateHistoryRootID(BigInt txn_id)
    {
        auto iter = txn_history_.findKeyGE(txn_id);
        return iter.is_found_eq(txn_id);
    }

    virtual BigInt commited_txn_id() {
        return last_commited_txn_id_;
    }

    virtual TxnPtr begin()
    {
        TxnPtr txn = std::make_shared<TxnImpl>(this, newTxnId(), TxnStatus::ACTIVE, CTR_CREATE);

        transactions_[txn->currentTxnId()] = txn;

        return txn;
    }

    virtual TxnPtr findTxn(BigInt txn_id)
    {
        auto iter = transactions_.find(txn_id);

        if (iter != transactions_.end())
        {
            if (iter->second.expired())
            {
                TxnStatus status    = getTxnStatus(txn_id);

                if (status == TxnStatus::ACTIVE)
                {
                    TxnPtr txn      = std::make_shared<TxnImpl>(this, txn_id, status, CTR_FIND);
                    iter->second    = txn;

                    return txn;
                }
                else {
                    TxnPtr txn      = std::make_shared<ReadOnlyTxnImpl>(this, txn_id, status, CTR_FIND);
                    iter->second    = txn;

                    return txn;
                }
            }
            else {
                return iter->second.lock();
            }
        }
        else {
            TxnStatus status        = getTxnStatus(txn_id);

            if (status == TxnStatus::ACTIVE)
            {
                TxnPtr txn              = std::make_shared<TxnImpl>(this, txn_id, status, CTR_FIND);
                transactions_[txn_id]   = txn;

                return txn;
            }
            else {
                TxnPtr txn              = std::make_shared<ReadOnlyTxnImpl>(this, txn_id, status, CTR_FIND);
                transactions_[txn_id]   = txn;

                return txn;
            }
        }
    }

    virtual void flush(bool force_sync = false)
    {
        allocator_->flush(force_sync);
    }

    BigInt commit(TxnImpl& txn)
    {
        auto& txn_ctr_directory = txn.ctr_directory();

        // Check containers for conflicts

        auto txn_iter = txn_ctr_directory.selectLabel(0, toInt(EntryStatus::UPDATED), 1);

        while (!txn_iter.isEnd())
        {
            BigInt name = txn_iter.key();
            auto iter   = ctr_directory_->findKeyGE(name);

            if (!iter.is_found_eq(name))
            {
                txn.forceRollback(MA_SRC, SBuf()<<"Update non-existent/removed container "<<name);
            }
            else {
                CtrDirectoryValue txn_entry = txn_iter.value();
                CtrDirectoryValue entry     = iter.value();

                BigInt src_txn_id = txn_entry.txn_id();
                BigInt tgt_txn_id = entry.txn_id();

                if (tgt_txn_id > src_txn_id)
                {
                    txn.forceRollback(MA_SRC, SBuf()<<"Update/update conflict for container "<<name);
                }
            }

            txn_iter++;
            txn_iter.selectLabelFw(0, toInt(EntryStatus::UPDATED), 1);
        }



        txn_iter = txn_ctr_directory.selectLabel(0, toInt(EntryStatus::CREATED), 1);

        while (!txn_iter.isEnd())
        {
            BigInt name = txn_iter.key();
            auto iter   = ctr_directory_->findKeyGE(name);

            if (iter.is_found_eq(name))
            {
                txn.forceRollback(MA_SRC, SBuf()<<"Create/exists conflict for container "<<name);
            }

            txn_iter++;
            txn_iter.selectLabelFw(0, toInt(EntryStatus::UPDATED), 1);
        }


        txn_iter = txn_ctr_directory.selectLabel(0, toInt(EntryStatus::DELETED), 1);

        while (!txn_iter.isEnd())
        {
            BigInt name = txn_iter.key();
            auto iter   = ctr_directory_->findKeyGE(name);

            if (iter.is_found_eq(name))
            {
                CtrDirectoryValue txn_entry = txn_iter.value();
                CtrDirectoryValue entry     = iter.value();

                BigInt src_txn_id = txn_entry.txn_id();
                BigInt tgt_txn_id = entry.txn_id();

                if (tgt_txn_id > src_txn_id)
                {
                    txn.forceRollback(MA_SRC, SBuf()<<"Delete/update conflict for container "<<name);
                }
            }

            txn_iter++;
            txn_iter.selectLabelFw(0, toInt(EntryStatus::UPDATED), 1);
        }


        // Commit changes to commit_history_

        last_commited_txn_id_ = newTxnId();

        // FIXME: move this line down to the next iteration
        txn_iter = txn_ctr_directory.selectLabel(0, toInt(EntryStatus::UPDATED), 1);

        TxnLog txn_log(&update_allocator_proxy_, CTR_CREATE, last_commited_txn_id_);

        {
            auto iter = txn_history_.findKeyGE(last_commited_txn_id_);
            MEMORIA_ASSERT_TRUE(iter.is_found_eq(last_commited_txn_id_));

            TxnStatus status = txn.is_snapshot() ? TxnStatus::SNAPSHOT : TxnStatus::COMMITED;

            iter.set_label(0, toInt(status));
        }

        TxnLogAllocator txn_log_allocator(this, txn_log);

        CtrDirectory ctr_directory(&txn_log_allocator, CTR_FIND, CtrDirectoryName);

        while (!txn_iter.isEnd())
        {
            BigInt name = txn_iter.key();
            ID id = txn_iter.value().value().value();

            auto iter = ctr_directory.findKeyGE(name);
            MEMORIA_ASSERT_TRUE(iter.is_found_eq(name));

            iter.svalue() = CtrDirectoryValue(last_commited_txn_id_, id);

            importPages(txn.update_log(), txn_log,  name);

            txn_iter++;
            txn_iter.selectLabelFw(0, toInt(EntryStatus::UPDATED), 1);
        }


        txn_iter = txn_ctr_directory.selectLabel(0, toInt(EntryStatus::CREATED), 1);

        while (!txn_iter.isEnd())
        {
            BigInt name = txn_iter.key();
            ID id = txn_iter.value().value().value();

            auto iter = ctr_directory.findKeyGE(name);
            MEMORIA_ASSERT_TRUE(!iter.is_found_eq(name));

            using Entry = typename CtrDirectory::Types::Entry;

            Entry entry;
            entry.key()     = name;
            entry.value()   = CtrDirectoryValue(last_commited_txn_id_, id);
            std::get<0>(entry.labels()) = toInt(EntryStatus::CLEAN);

            //iter.insert(name, CtrDirectoryValue(last_commited_txn_id_, id), toInt(EntryStatus::CLEAN));
            iter.insert(entry);

            importPages(txn.update_log(), txn_log, name);

            txn_iter++;
            txn_iter.selectLabelFw(0, toInt(EntryStatus::CREATED), 1);
        }



        txn_iter = txn_ctr_directory.selectLabel(0, toInt(EntryStatus::DELETED), 1);

        while (!txn_iter.isEnd())
        {
            BigInt name = txn_iter.key();

            auto iter = ctr_directory.findKeyGE(name);

            if (iter.is_found_eq(name))
            {
                iter.remove();
            }
            else {
                txn_iter++;
            }

            importPages(txn.update_log(), txn_log, name);

            txn_iter.selectLabelFw(0, toInt(EntryStatus::DELETED), 1);
        }

        cleanupCtrDirectoryBlocks(txn.update_log());

        allocator_->properties().setLastCommitId(last_commited_txn_id_);

        if (compactify_history_ && (commit_history_cnt_ % 5 == 0))
        {
            compactifyCommitHistory();
        }

        commit_history_cnt_++;

        allocator_->flush();

        txn.clean();

        return last_commited_txn_id_;
    }

    BigInt newTxnId()
    {
        return allocator_->properties().newTxnId();
    }


    // IAllocator

    virtual PageG getPage(const ID& id, BigInt name)
    {
        auto iter = commit_history_.find(id);

        if (iter.is_found_eq(id))
        {
            if (iter.find2ndLE(last_commited_txn_id_))
            {
                ID gid = iter.value().second;

                PageG page = allocator_->getPage(gid, name);

                MEMORIA_WARNING(page->id(), !=, id);

                return wrapPageG(page);
            }
            else {
                throw vapi::Exception(
                        MA_SRC,
                        SBuf()<<"Page with id="<<id<<" is not found in commit history for txn_id="<<last_commited_txn_id_
                );
            }
        }
        else {
            throw vapi::Exception(MA_SRC, SBuf()<<"Page with id="<<id<<" is not found in commit history");
        }
    }



    virtual PageG getPageForUpdate(const ID& id, BigInt name)
    {
        auto iter = commit_history_.find(id);

        if (iter.is_found_eq(id))
        {
            if (iter.find2ndLE(last_commited_txn_id_))
            {
                ID gid = iter.value().second;

                PageG old_page = allocator_->getPage(gid, name);

                MEMORIA_ASSERT(old_page->id(), ==, id);

                if (iter.key2() == last_commited_txn_id_)
                {
                    return wrapPageG(old_page);
                }
                else
                {
                    PageG new_page  = allocator_->createPage(old_page->page_size(), name);
                    ID new_gid      = new_page->gid();

                    CopyByteBuffer(old_page.page(), new_page.page(), old_page->page_size());

                    new_page->gid() = new_gid;
                    new_page->id()  = id;

                    iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::UPDATED), new_gid));

                    return wrapPageG(new_page);
                }
            }
            else {
                throw vapi::Exception(
                        MA_SRC,
                        SBuf()<<"Page with id="<<id<<" is not found in commit history for txn_id="<<last_commited_txn_id_
                );
            }
        }
        else {
            throw vapi::Exception(MA_SRC, SBuf()<<"Page with id="<<id<<" is not found in commit history");
        }
    }




    virtual PageG updatePage(Shared* _shared, BigInt name)
    {
        MVCCPageShared* shared = static_cast<MVCCPageShared*>(_shared);

        ID id = shared->id();

        auto iter = commit_history_.find(id);

        if (iter.is_found_eq(id))
        {
            if (iter.find2ndLE(last_commited_txn_id_))
            {
                if (iter.key2() < last_commited_txn_id_)
                {
                    Int page_size = shared->get()->page_size();

                    PageG new_page  = allocator_->createPage(page_size, name);
                    ID new_gid      = new_page->gid();

                    CopyByteBuffer(shared->get(), new_page.page(), page_size);

                    new_page->gid() = new_gid;
                    new_page->id()  = id;

                    iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::UPDATED), new_gid));

                    return wrapPageG(new_page);
                }
                else {
                    return wrapPageG(shared, allocator_->updatePage(shared->delegate(), name));
                }
            }
            else {
                throw vapi::Exception(
                        MA_SRC,
                        SBuf()<<"Page with id="<<id<<" is not found in commit history for txn_id="<<last_commited_txn_id_
                );
            }
        }
        else {
            throw vapi::Exception(MA_SRC, SBuf()<<"Page with id="<<id<<" is not found in commit history");
        }
    }




    virtual void removePage(const ID& id, BigInt name)
    {
        auto iter = commit_history_.find(id);

        if (iter.is_found_eq(id))
        {
            if (iter.find2ndLE(last_commited_txn_id_))
            {
                if (iter.key2() != last_commited_txn_id_)
                {
                    iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::DELETED), 0));
                }
                else {
                    CommitHistoryValue value = iter.value();

                    if (value.first != toInt(EntryStatus::DELETED))
                    {
                        value.first = toInt(EntryStatus::DELETED);

                        if (value.second.isSet())
                        {
                            allocator_->removePage(value.second, name);
                        }

                        iter.setValue(value);
                    }
                }

                // Is it necessary to mark existing page guards DELETEd?
            }
            else {
                throw vapi::Exception(
                        MA_SRC,
                        SBuf()<<"Page with id="<<id<<" is not found in commit history for txn_id="<<last_commited_txn_id_
                );
            }
        }
        else {
            throw vapi::Exception(MA_SRC, SBuf()<<"Page with id="<<id<<" is not found in commit history");
        }
    }


    virtual PageG createPage(Int initial_size, BigInt name)
    {
        PageG new_page  = allocator_->createPage(initial_size, name);
        ID new_gid      = new_page->gid();
        ID new_id       = allocator_->newId();
        new_page->id()  = new_id;

        auto iter = commit_history_.createNew(new_id);

        iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::CREATED), new_gid));

        return wrapPageG(new_page);
    }

    virtual void resizePage(Shared* _shared, Int new_size)
    {
        MVCCPageShared* shared = static_cast<MVCCPageShared*>(_shared);

        return allocator_->resizePage(shared->delegate(), new_size);
    }

    virtual void releasePage(Shared* _shared) noexcept
    {
        MVCCPageShared* shared = static_cast<MVCCPageShared*>(_shared);

        allocated_shared_objects_.erase(shared->id());

        freePageShared(shared);
    }

    virtual ID getRootID(BigInt name)
    {
        if (name == CtrDirectoryName)
        {
            auto iter = roots_.findKeyLE(last_commited_txn_id_);

            if (iter.is_found_le(last_commited_txn_id_))
            {
                return iter.value();
            }
            else {
                throw Exception(MA_SRC, "CtrDirectory root ID is not found");
            }
        }
        else
        {
            auto iter = ctr_directory_->findKeyGE(name);

            if (iter.is_found_eq(name))
            {
                return iter.value().value().value();
            }
            else {
                throw Exception(MA_SRC, SBuf()<<"Ctr root ID is not found for name: "<<name);
            }
        }
    }

    virtual void markUpdated(BigInt name) {}

    virtual BigInt currentTxnId() const
    {
        return last_commited_txn_id_;
    }


    virtual void setRoot(BigInt name, const ID& root)
    {
        if (name == CtrDirectoryName)
        {
            auto iter = roots_.findKeyLE(last_commited_txn_id_);

            if (iter.is_found_eq(last_commited_txn_id_))
            {
                if (root.isSet())
                {
                    iter.svalue() = root;
                }
                else {
                    iter.remove();
                }
            }
            else if (root.isSet())
            {
                iter.insert(last_commited_txn_id_, root);
            }
            else {
                throw Exception(MA_SRC, "Attempt to remove deleted basic root");
            }
        }
        else
        {
            auto iter = ctr_directory_->findKeyGE(name);

            if (root.isSet())
            {
                CtrDirectoryValue root_value(last_commited_txn_id_, root);

                if (iter.is_found_eq(name))
                {
                    iter.svalue() = root_value;
                }
                else {
                    using Entry = typename CtrDirectory::Types::Entry;

                    Entry entry;
                    entry.key()     = name;
                    entry.value()   = root_value;
                    std::get<0>(entry.labels()) = toInt(EntryStatus::CLEAN);


//                  iter.insert(name, root_value, toInt(EntryStatus::CLEAN));
                    iter.insert(entry);
                }
            }
            else {
                if (iter.is_found_eq(name))
                {
                    iter.remove();
                }
                else {
                    throw Exception(MA_SRC, "Attempt to remove deleted root");
                }
            }
        }
    }

    virtual bool hasRoot(BigInt name)
    {
        if (name == CtrDirectoryName)
        {
            auto iter = roots_.findKeyLE(last_commited_txn_id_);
            return iter.is_found_le(last_commited_txn_id_);
        }
        else
        {
            auto iter = ctr_directory_->findKeyGE(name);
            return iter.is_found_eq(name);
        }
    }

    void dumpPage(const PageG& page)
    {
        auto page_metadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        vapi::dumpPageData(page_metadata, page.page(), std::cout);
    }


    virtual void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
        allocator_->walkContainers(walker, allocator_descr);

        walker->beginAllocator("MVCCAllocator", allocator_descr);

        dumpHistory(walker);
        dumpTransactions(walker);

        walker->beginSnapshot("trunk");

        ctr_directory_->walkTree(walker);

        auto iter = ctr_directory_->Begin();

        while (!iter.isEnd())
        {
            BigInt ctr_name = iter.key();
            ID root_id      = iter.value().value().value();

            PageG page      = this->getPage(root_id, ctr_name);

            Int master_hash = page->master_ctr_type_hash();
            Int ctr_hash    = page->ctr_type_hash();

            ContainerMetadata* ctr_meta = metadata_->getContainerMetadata(master_hash != 0 ? master_hash : ctr_hash);

            ctr_meta->getCtrInterface()->walk(&page->id(), ctr_name, this, walker);

            iter++;
        }

        walker->endSnapshot();
        walker->endAllocator();
    }

    void dumpAllocator(StringRef path, const char* descr = nullptr)
    {
        typedef FSDumpContainerWalker<typename Allocator::Page> Walker;
        Walker walker(metadata_, path);

        walkContainers(&walker, descr);
    }

    void dumpHistory(StringRef path)
    {
        typedef FSDumpContainerWalker<typename Allocator::Page> Walker;
        Walker walker(metadata_, path);

        dumpHistory(&walker);
    }

    virtual bool check()
    {
        bool result = checkDictionaries();

        for (auto iter = ctr_directory_->Begin(); !iter.isEnd(); )
        {
            BigInt ctr_name = iter.key();

            PageG page = this->getPage(iter.value().value().value(), ctr_name);

            ContainerMetadata* ctr_meta = metadata_->getContainerMetadata(page->ctr_type_hash());

            result = ctr_meta->getCtrInterface()->check(&page->id(), ctr_name, this) || result;

            iter++;
        }

        return result;
    }

    bool checkDictionaries()
    {
        bool result = ctr_directory_->checkTree();

        result = commit_history_.checkTree() || result;
        result = roots_.checkTree() || result;

        checkCommitHistory();

        return result;
    }

    void compactifyCommitHistory()
    {
        auto start = txn_history_.selectLabel(0, toInt(TxnStatus::COMMITED), 1);

        while(!start.isEnd())
        {
            BigInt limit = findMergeTxnHistoryLimit(start);

            cleanupTransactions(start.key(), limit);

            while (start.key() < limit)
            {
                MEMORIA_ASSERT(start.label(0), ==, toInt(TxnStatus::COMMITED));

                auto current_start_key = start.key();

                // Txn log is removed in different iterator that affects current txn history iterators
                // So they must be refreshed
                TxnLog txn_log(&update_allocator_proxy_, CTR_FIND, start.key());
                txn_log.drop();

                // refresh iterators
                start = txn_history_.findKeyGE(current_start_key);
            }

            start.selectNextLabel(0, toInt(TxnStatus::COMMITED));
        }
    }

    virtual void removeTxn(BigInt txn_id)
    {
        TxnStatus status = getTxnStatus(txn_id);
        if (status == TxnStatus::SNAPSHOT)
        {
            // Mark transaction as COMMITED. Next history compaction will
            // free the resources.
            setTxnStatus(txn_id, TxnStatus::COMMITED);
        }
    }

    virtual TxnIteratorPtr transactions(TxnStatus status)
    {
        auto iter = txn_history_.selectLabel(0, toInt(status), 1);
        return std::make_shared<TxnIteratorImpl<MyType, typename TxnHistory::Iterator>>(this, status, iter);
    }


    virtual BigInt total_transactions(TxnStatus status)
    {
        auto iter = txn_history_.End();
        return iter.label_rank(0, toInt(status));
    }


    /**
     * Invoked manually at startup to fix unclean shutdown
     */
    void cleanupActiveTransactions()
    {
        for (auto iter = this->transactions(TxnStatus::ACTIVE); iter->has_next(); iter->next())
        {
            iter->txn()->rollback();
        }
    }


private:

    // *********************************** Private Methods **************************************** //

    void unregisterTxn(BigInt txn_id)
    {
        transactions_.erase(txn_id);
    }

    TxnStatus getTxnStatus(BigInt txn_id)
    {
        auto iter = txn_history_.findKeyGE(txn_id);
        MEMORIA_ASSERT_TRUE(iter.is_found_eq(txn_id));

        return static_cast<TxnStatus>(iter.label(0));
    }

    void setTxnStatus(BigInt txn_id, TxnStatus status)
    {
        auto iter = txn_history_.findKeyGE(txn_id);
        MEMORIA_ASSERT_TRUE(iter.is_found_eq(txn_id));

        iter.set_label(0, toInt(status));
    }

    void checkCommitHistory()
    {
        auto iter = commit_history_.Begin();

        while (!iter.isEnd())
        {
            ID id = iter.key();

            while (!iter.isEof())
            {
                BigInt txn_id               = iter.key2();
                CommitHistoryValue value    = iter.value();

                Int mark    = value.first;
                ID gid      = value.second;

                if (mark != toInt(EntryStatus::DELETED))
                {
                    // fixme: provide valid Ctr name here
                    PageG page = allocator_->getPage(gid, -1);

                    if (page->id() != id)
                    {
                        throw vapi::Exception(MA_SRC, SBuf()<<"Invalid IDs for page "<<gid
                                                            <<" txn_id="<<txn_id
                                                            <<": expected ID="<<id
                                                            <<" actual ID="<<page->id()
                                              );
                    }
                }

                iter.skipFw(1);
            }

            iter++;
        }
    }

    void dumpHistory(ContainerWalker* walker)
    {
        auto iter = commit_history_.Begin();

        walker->beginSection("CommitHistory");

        while (!iter.isEnd())
        {
            ID id = iter.key();

            walker->beginSection((SBuf()<<id).str().c_str());

            while (!iter.isEof())
            {
                BigInt txn_id               = iter.key2();
                CommitHistoryValue value    = iter.value();

                Int mark    = value.first;
                ID gid      = value.second;

                if (mark != toInt(EntryStatus::DELETED))
                {
                    PageG page = allocator_->getPage(gid, -1); // fixme: provide valid Ctr name here

                    walker->singleNode((SBuf()<<txn_id<<"__"<<mark<<"__"<<gid).str().c_str(), page.page());
                }
                else {
                    walker->content((SBuf()<<txn_id<<"__"<<mark<<"__"<<gid).str().c_str(), "DELETED");
                }

                iter.skipFw(1);
            }

            walker->endSection();

            iter++;
        }

        walker->endSection();
    }

    void dumpTransactions(ContainerWalker* walker)
    {
        walker->beginSection("Transactions");

        TxnHistory txn_history(allocator_, CTR_FIND, TxnHistoryName);

        auto iter = txn_history.Begin();

        while (!iter.isEnd())
        {
            BigInt txn_id = iter.key();
            TxnStatus status = static_cast<TxnStatus>(iter.label(0));

            if (status == TxnStatus::ACTIVE)
            {
                auto txn = this->findTxn(txn_id);

                txn->walkContainers(walker);
            }
            else
            {
                TxnLog txn_log(&update_allocator_proxy_, CTR_FIND, txn_id);
                txn_log.walkTree(walker);
            }

            iter++;
        }

        walker->endSection();
    }


    template <typename Ctr>
    void importPages(Ctr& ctr, TxnLog& txn_log, BigInt name)
    {
        auto iter = ctr.find(name);
        MEMORIA_ASSERT_TRUE(iter.is_found_eq(name));

        while (!iter.isEof())
        {
            typename Ctr::Types::Value value = iter.value();

            EntryStatus status  = static_cast<EntryStatus>(value.first);
            ID gid              = value.second;
            ID id               = iter.key2();

            if (status == EntryStatus::CREATED)
            {
                auto h_iter = commit_history_.createNew(id);

                h_iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::CREATED), gid));

                txn_log[id].svalue() = toInt(EntryStatus::CREATED);
            }
            else if (status == EntryStatus::UPDATED)
            {
                auto h_iter = commit_history_.find(id);
                MEMORIA_ASSERT_TRUE(h_iter.is_found_eq(id));

                h_iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::UPDATED), gid));

                txn_log[id].svalue() = toInt(EntryStatus::UPDATED);
            }
            else if (status == EntryStatus::DELETED)
            {
                auto h_iter = commit_history_.find(id);
                MEMORIA_ASSERT_TRUE(h_iter.is_found_eq(id));

                h_iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::DELETED), ID(0)));

                txn_log[id].svalue() = toInt(EntryStatus::DELETED);
            }
            else {
                throw vapi::Exception(MA_SRC, SBuf()<<"Unknown entry status value: "<<toInt(status));
            }

//          MEMORIA_ASSERT_FALSE(checkDictionaries());

            iter.skipFw(1);
        }
    }





    BigInt findMergeTxnHistoryLimit(typename TxnHistory::Iterator& start)
    {
        BigInt start_txn_id = start.key();
        BigInt limit = findMergeTxnHistoryOnlyLimit(start);

        auto lower_range = transactions_.lower_bound(start_txn_id);

        if (lower_range == transactions_.end())
        {
            return limit;
        }
        else if (lower_range->first >= limit)
        {
            return limit;
        }
        else {
            start.dump();
            return lower_range->first;
        }
    }

    BigInt findMergeTxnHistoryOnlyLimit(typename TxnHistory::Iterator& start)
    {
        auto iter1 = start;
        auto iter2 = start;

        iter1.selectLabelFw(0, toInt(TxnStatus::ACTIVE), 1);
        iter1--;

        iter2.selectLabelFw(0, toInt(TxnStatus::SNAPSHOT), 1);

        if (iter2.isEnd())
        {
            return iter1.key();
        }
        else if (iter1.key() < iter2.key())
        {
            return iter1.key();
        }
        else {
            return iter2.key();
        }
    }

    template <typename Ctr>
    void cleanupCtrDirectoryBlocks(Ctr& txn_log)
    {
        auto iter = txn_log.find(CtrDirectoryName);

        if (iter.is_found_eq(CtrDirectoryName))
        {
            while (!iter.isEof())
            {
                typename Ctr::Types::Value value = iter.value();

                EntryStatus status  = static_cast<EntryStatus>(value.first);
                ID gid              = value.second;

                if (status != EntryStatus::DELETED)
                {
                    allocator_->removePage(gid, -1);
                }

                value.first             = toInt(EntryStatus::DELETED);
                value.second.value()    = ID(0);

                iter.setValue(value);

                iter.skipFw(1);
            }
        }
    }

    typename TxnHistory::Iterator findTxnHistoryStart()
    {
        return txn_history_.select(toInt(TxnStatus::COMMITED), 1);
    }

    typename TxnHistory::Iterator findMergeTxnHistoryTop()
    {
        auto iter = txn_history_.select(toInt(TxnStatus::ACTIVE), 1);

        iter--;

        return iter;
    }


    TxnLog combineTxnLogs(BigInt base_txn_id, BigInt max_txn_id)
    {
        auto iter = txn_history_.findKeyGE(base_txn_id);

        TxnLog multi_log(allocator_, CTR_CREATE);

        while ((!iter.isEnd()) && iter.key() < max_txn_id)
        {
            BigInt txn_id = iter.key();
            TxnLog txn_log(&update_allocator_proxy_, CTR_FIND, txn_id);

            auto txn_log_iter = txn_log.Begin();

            while (!txn_log_iter.isEnd())
            {
                ID id = txn_log_iter.key();

                multi_log[id] = 0;

                txn_log_iter++;
            }

            iter++;
        }

        return multi_log;
    }


    void cleanupTransactions(BigInt base_txn_id, BigInt max_txn_id)
    {
        if (max_txn_id > base_txn_id)
        {
            auto multi_log  = combineTxnLogs(base_txn_id, max_txn_id);
            auto iter       = multi_log.Begin();

            while (!iter.isEnd())
            {
                ID id = iter.key();

                auto history_iter = commit_history_.find(id);

                if (history_iter.is_found_eq(id))
                {
                    cleanupHistoryBlocks(history_iter, base_txn_id, max_txn_id);

                    if (history_iter.size() == 0)
                    {
                        history_iter.removeEntry();
                    }
                }

                iter++;
            }

            multi_log.drop();

            cleanupCtrDirectoryRoots(base_txn_id, max_txn_id);
        }
    }



    void dumpIDs(TxnLog& txn_log, BigInt base, BigInt max)
    {
        auto iter = txn_log.Begin();

        if (!iter.isEnd())
        {
            cout<<"=============== Combined TxnLog =============== "<<base<<" "<<max<<endl;

            while (!iter.isEnd())
            {
                cout<<ID(iter.key())<<endl;

                iter++;
            }

            cout<<endl;
        }
    }

    void dumpTxns(typename CommitHistory::Iterator& iter)
    {
        auto i1 = iter;

        if (i1.pos() > 0) {
            i1.skipBw(i1.pos());
        }

        cout<<"  Txns: ";

        while (!i1.isEof())
        {
            CommitHistoryValue entry = i1.value();

            cout<<i1.key2()<<":"<<entry.second<<", ";
            i1.skipFw(1);
        }

        cout<<endl;
    }

    void dumpAvailableTxns(BigInt base, BigInt max)
    {
        auto h_iter = commit_history_.Begin();

        while (!h_iter.isEnd())
        {
            dumpAvailableTxns(h_iter, base, max);
            h_iter++;
        }
    }

    void dumpAvailableTxns(typename CommitHistory::Iterator& iter, BigInt base, BigInt max)
    {
        auto i1 = iter;

        i1.findKeyGE(base);

        stringstream ss;

        ss<<"  Txns: "<<ID(iter.key())<<" ";

        Int cnt = 0;

        while ((!i1.isEof()) && i1.key2() < max)
        {
            ss<<i1.key2()<<", ";
            i1.skipFw(1);

            cnt++;
        }

        ss<<" Total="<<cnt<<endl;

        if (cnt > 0) {
            cout<<ss.str();
        }
    }

    void cleanupHistoryBlocks(typename CommitHistory::Iterator& iter, BigInt base_txn_id, BigInt max_txn_id)
    {
        auto top = iter;

        if (iter.findKeyGE(base_txn_id))
        {
            if (iter.key2() < max_txn_id)
            {
                BigInt pos = iter.pos();

                if (top.findKeyLE(max_txn_id))
                {
                    BigInt top_txn_id = top.key2();

                    while(iter.key2() < top_txn_id)
                    {
                        CommitHistoryValue entry = iter.value();

                        EntryStatus status  = static_cast<EntryStatus>(entry.first);
                        ID gid              = entry.second;

                        if (status != EntryStatus::DELETED)
                        {
                            MEMORIA_ASSERT_TRUE(gid.isSet());

                            allocator_->removePage(gid, -1);
                        }

                        iter.remove();
                    }

                    if (top_txn_id < max_txn_id)
                    {
                        iter.updateKey2(max_txn_id - top_txn_id);
                        MEMORIA_ASSERT(iter.key2(), ==, max_txn_id);

                        TxnLog txn_log(&update_allocator_proxy_, CTR_FIND, max_txn_id);

                        ID id = iter.key();

                        auto i2 = txn_log.findKeyGE(id);

                        if (!i2.is_found_eq(id))
                        {
                            i2.insert(id, 0);
                        }
                    }

                    if (pos == 0)
                    {
                        CommitHistoryValue entry = iter.value();
                        EntryStatus status = static_cast<EntryStatus>(entry.first);

                        if (status == EntryStatus::DELETED)
                        {
                            iter.remove();
                        }
                        else if (status == EntryStatus::UPDATED)
                        {
                            entry.first = toInt(EntryStatus::CREATED);
                            iter.setValue(entry);
                        }
                    }
                }
            }
        }
    }


    void cleanupCtrDirectoryRoots(BigInt base_txn_id, BigInt max_txn_id)
    {
        auto base = roots_.findKeyGE(base_txn_id);

        if (base.is_found_lt(max_txn_id))
        {
            auto top = roots_.findKeyLE(max_txn_id);

            if (top.is_found_ge(base_txn_id))
            {
                BigInt top_txn_id = top.key();

                while (base.key() < top_txn_id)
                {
                    base.remove();
                }

                if (top_txn_id < max_txn_id)
                {
                    // FIXME: adjust next key?
                    base.adjustKey(max_txn_id - top_txn_id);
                    MEMORIA_ASSERT(base.key(), ==, max_txn_id);
                }
            }
        }
    }


    void setCtrDirectoryRootID(BigInt txn_id, const ID& root_id)
    {
        auto iter = roots_.findKey(txn_id);

        if (is_found(iter, txn_id))
        {
            iter.value() = root_id;
        }
        else {
            iter.insert(txn_id, root_id);
        }
    }


    MVCCPageSharedPtr allocatePageShared()
    {
        MEMORIA_ASSERT_TRUE(shared_pool_.size() > 0);

        MVCCPageSharedPtr shared = shared_pool_.takeTop();

        shared->init();

        return shared;
    }

    void freePageShared(MVCCPageSharedPtr shared)
    {
        MEMORIA_ASSERT_FALSE(shared->next());
        MEMORIA_ASSERT_FALSE(shared->prev());

        MEMORIA_ASSERT(shared_pool_.size(), <, shared_pool_.max_size());

        shared_pool_.put(shared);
    }



    template <typename PageGuard>
    PageG wrapPageG(PageGuard&& src)
    {
        ID id = src->id();

        auto iter = allocated_shared_objects_.find(id);

        if (iter != allocated_shared_objects_.end())
        {
            MVCCPageSharedPtr shared = iter->second;

            if (shared->delegate() != src.shared())
            {
                shared->setDelegate(src.shared());

                shared->state() = src.shared()->state();
                shared->set_page(src.page());
            }

            return PageG(shared);
        }
        else {
            MVCCPageSharedPtr shared = allocatePageShared();

            shared->id()    = id;
            shared->state() = src.shared()->state();

            shared->set_page(src.page());
            shared->set_allocator(this);

            shared->setDelegate(src.shared());

            allocated_shared_objects_[id] = shared;

            return PageG(shared);
        }
    }


    PageG wrapPageG(MVCCPageShared* shared, PageG src)
    {
        shared->setDelegate(src.shared());
        shared->set_page(src.page());

        shared->state() = src.shared()->state();

        return PageG(shared);
    }
};

}


#endif
