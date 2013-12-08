
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_ALLOCATOR_MVCC_TXN_RO_HPP_
#define MEMORIA_ALLOCATOR_MVCC_TXN_RO_HPP_

#include <memoria/core/tools/pool.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/file.hpp>
#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/types/types.hpp>

#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/page.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/containers/map/map_factory.hpp>
#include <memoria/containers/smrk_map/smrkmap_factory.hpp>

#include <memoria/allocators/mvcc/mvcc_tools.hpp>

#include <unordered_map>

namespace memoria {

template <typename Profile, typename TxnMgr, typename PageType>
class MVCCReadOnlyTxn: public MVCCAllocatorBase<ITxn<PageType>> {
	typedef MVCCAllocatorBase<ITxn<PageType>>									Base;
public:

	typedef IJournaledAllocator<PageType>										Allocator;
	typedef PageType                                                    		Page;
	typedef typename Page::ID                                           		ID;
	typedef typename Base::PageG                                     			PageG;
	typedef typename Base::Shared                                      			Shared;

	typedef typename Base::CtrShared                                         	CtrShared;

	typedef typename CtrTF<Profile, DblMrkMap2<BigInt, ID, 2>>::Type			UpdateLog;

	typedef std::unordered_map<BigInt, CtrShared*>                              CtrSharedMap;

	typedef typename TxnMgr::CtrDirectory										CtrDirectory;

	typedef typename UpdateLog::Types::Value									UpdateLogValue;
	typedef typename CtrDirectory::Types::Value									CtrDirectoryValue;

	typedef typename TxnMgr::MVCCPageShared										MVCCPageShared;
	typedef typename TxnMgr::MVCCPageSharedPtr									MVCCPageSharedPtr;
	typedef typename TxnMgr::PageSharedMap										PageSharedMap;
	typedef typename TxnMgr::SharedPool											SharedPool;

	template <typename, typename> friend class MVCCAllocator;

private:

	// memory pool for PageShared objects
	SharedPool		shared_pool_;
	PageSharedMap	allocated_shared_objects_;

	Allocator*		allocator_;

	TxnMgr* 		txn_mgr_;

	BigInt 			txn_id_;

	TxnStatus		status_;

	CtrDirectory	ctr_directory_;

public:

	MVCCReadOnlyTxn(TxnMgr* txn_mgr, BigInt txn_id, TxnStatus status, Int ctr_cmd = CTR_CREATE):
		Base(txn_mgr->allocator()),
		shared_pool_(128, 128),
		allocator_(txn_mgr->allocator()),
		txn_mgr_(txn_mgr),
		txn_id_(txn_id),
		status_(status),
		ctr_directory_(this, CTR_FIND, TxnMgr::CtrDirectoryName)
	{}

	virtual ~MVCCReadOnlyTxn()
	{
		txn_mgr_->unregisterTxn(txn_id_);
	}

	virtual TxnStatus status() const {
		return status_;
	}

	virtual void setSnapshot(bool snapshot)
	{
		fail();
	}

	virtual bool is_snapshot() const {
		return status_ == TxnStatus::SNAPSHOT;
	}

	CtrDirectory& ctr_directory()
	{
		return ctr_directory_;
	}

	virtual BigInt txn_id() const
	{
		return txn_id_;
	}


    virtual PageG getPage(const ID& id, BigInt name)
    {
    	PageG old_page = txn_mgr_->getPage(txn_id_, id, name);
    	return wrapPageG(old_page);
    }

    virtual PageG getPageForUpdate(const ID& id, BigInt name)
    {
    	return failG();
    }


	virtual PageG updatePage(Shared* _shared, BigInt name)
	{
		return failG();
	}

	virtual void removePage(const ID& id, BigInt name)
	{
		fail();
	}

	virtual PageG createPage(Int initial_size, BigInt name)
	{
		return failG();
	}

	virtual void resizePage(Shared* _shared, Int new_size)
	{
		fail();
	}

	virtual void releasePage(Shared* _shared) noexcept
	{
		MVCCPageShared* shared = static_cast<MVCCPageShared*>(_shared);

		allocated_shared_objects_.erase(shared->id());

		freePageShared(shared);
	}

	// Ctr Directory

	virtual ID getRootID(BigInt name)
	{
		if (name == TxnMgr::CtrDirectoryName)
		{
			return txn_mgr_->getCtrDirectoryRootID(txn_id_);
		}
		else
		{
			auto iter = ctr_directory_.find(name);

			if (!(iter.isEnd() || iter.key() != name))
			{
				return iter.value().value().value();
			}
			else {
				return ID(0);
			}
		}
	}



	virtual void setRoot(BigInt name, const ID& root)
	{
		fail();
	}

	virtual bool hasRoot(BigInt name)
	{
		if (name == TxnMgr::CtrDirectoryName)
		{
			return txn_mgr_->hasRoot(TxnMgr::CtrDirectoryName);
		}
		else {
			auto iter = ctr_directory_.findKeyGE(name);
			return iter.is_found_eq(name);
		}
	}

	virtual void markUpdated(BigInt name)
	{
		fail();
	}

	virtual BigInt currentTxnId() const	{
		return txn_id_;
	}

	// ITransation

	virtual BigInt commit()
	{
		if (status_ == TxnStatus::SNAPSHOT)
		{
			throw vapi::Exception(MA_SRC, "Can't commit snapshots");
		}
		else if (status_ == TxnStatus::COMMITED)
		{
			throw vapi::Exception(MA_SRC, "Can't commit already committed transactions");
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"Unknown txn status: "<<toInt(status_));
		}
	}

	virtual void rollback()
	{

	}


	virtual bool check()
	{
		bool result = checkDictionaries();

		auto metadata = MetadataRepository<Profile>::getMetadata();

		for (auto iter = ctr_directory_.Begin(); !iter.isEnd(); )
		{
			BigInt ctr_name = iter.key();

			PageG page = this->getPage(iter.value().value().value(), ctr_name);

			ContainerMetadata* ctr_meta = metadata->getContainerMetadata(page->ctr_type_hash());

			result = ctr_meta->getCtrInterface()->check(&page->id(), ctr_name, this) || result;

			iter++;
		}

		return result;
	}

	bool checkDictionaries()
	{
		bool result = false;

		result = ctr_directory_.checkTree();

		return result;
	}

    virtual void flush(bool force_sync = false)
    {
    	return allocator_->flush(force_sync);
    }

    virtual void walkContainers(vapi::ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
    	if (status_ == TxnStatus::COMMITED)
    	{
    		walker->beginAllocator("CommittedTxn", allocator_descr);
    	}
    	else if (status_ == TxnStatus::SNAPSHOT)
    	{
    		walker->beginAllocator("Snapshot", allocator_descr);
    	}
    	else {
    		walker->beginAllocator("ReadOnlyTxn", allocator_descr);
    	}

    	ctr_directory_.walkTree(walker);

    	auto metadata = MetadataRepository<Profile>::getMetadata();

    	auto iter = ctr_directory_.Begin();

    	while (!iter.isEnd())
    	{
    		BigInt ctr_name = iter.key();
    		ID root_id		= iter.value().value().value();

    		PageG page 		= this->getPage(root_id, ctr_name);

    		ContainerMetadata* ctr_meta = metadata->getContainerMetadata(page->ctr_type_hash());

    		ctr_meta->getCtrInterface()->walk(&page->id(), ctr_name, this, walker);

    		iter++;
    	}

    	walker->endAllocator();
    }


private:

    void fail()
    {
    	if (status_ == TxnStatus::COMMITED)
    	{
    		throw vapi::Exception(MA_SRC, "This committed transaction is read only");
    	}
    	else if (status_ == TxnStatus::SNAPSHOT)
    	{
    		throw vapi::Exception(MA_SRC, "This snapshot is read only");
    	}
    	else {
    		throw vapi::Exception(MA_SRC, "This transaction with unknown status is read only");
    	}
    }

    PageG failG()
    {
    	if (status_ == TxnStatus::COMMITED)
    	{
    		throw vapi::Exception(MA_SRC, "This committed transaction is read only");
    	}
    	else if (status_ == TxnStatus::SNAPSHOT)
    	{
    		throw vapi::Exception(MA_SRC, "This snapshot is read only");
    	}
    	else {
    		throw vapi::Exception(MA_SRC, "This transaction with unknown status is read only");
    	}
    }

	bool find2ndEQ(typename UpdateLog::Iterator& iter, const ID& id)
	{
		return iter.findKeyGE(id) && iter.key2() == id;
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

    			shared->state()	= src.shared()->state();
    			shared->set_page(src.page());
    		}

    		return PageG(shared);
    	}
    	else {
    		MVCCPageSharedPtr shared = allocatePageShared();

    		shared->id()    = id;
    		shared->state()	= src.shared()->state();

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

    	shared->state()	= src.shared()->state();

    	return PageG(shared);
    }
};

}


#endif
