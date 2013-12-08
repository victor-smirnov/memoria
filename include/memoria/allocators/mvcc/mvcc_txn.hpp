
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_ALLOCATOR_MVCC_TXN_HPP_
#define MEMORIA_ALLOCATOR_MVCC_TXN_HPP_

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
class MVCCTxn: public MVCCAllocatorBase<ITxn<PageType>> {
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

	TxnUpdateAllocatorProxy<TxnMgr, PageType> update_log_allocator_proxy_;

	// memory pool for PageShared objects
	SharedPool		shared_pool_;
	PageSharedMap	allocated_shared_objects_;

	Allocator*		allocator_;

	TxnMgr* 		txn_mgr_;

	BigInt 			txn_id_;

	TxnStatus		status_;

	ID				ctr_directory_root_id_;

	UpdateLog		update_log_;
	CtrDirectory	ctr_directory_;

	bool snapshot_	= false;


public:

	MVCCTxn(TxnMgr* txn_mgr, BigInt txn_id, TxnStatus status, Int ctr_cmd = CTR_CREATE):
		Base(txn_mgr->allocator()),
		update_log_allocator_proxy_(txn_mgr),
		shared_pool_(128, 128),
		allocator_(txn_mgr->allocator()),
		txn_mgr_(txn_mgr),
		txn_id_(txn_id),
		status_(status),
		ctr_directory_root_id_(0),
		update_log_(&update_log_allocator_proxy_, ctr_cmd, txn_id_),
		ctr_directory_(this, CTR_FIND, TxnMgr::CtrDirectoryName)
	{}

	virtual ~MVCCTxn()
	{
		txn_mgr_->unregisterTxn(txn_id_);
	}

	virtual TxnStatus status() const {
		return status_;
	}

	virtual void setSnapshot(bool snapshot)
	{
		snapshot_ = snapshot;
	}

	virtual bool is_snapshot() const {
		return snapshot_;
	}

	UpdateLog& update_log()
	{
		return update_log_;
	}

	CtrDirectory& ctr_directory()
	{
		return ctr_directory_;
	}

	ID ctr_directory_root_id() const
	{
		return ctr_directory_root_id_;
	}

	virtual BigInt txn_id() const
	{
		return txn_id_;
	}


    virtual PageG getPage(const ID& id, BigInt name) {
    	return getPage(id, Allocator::READ, name);
    }

    virtual PageG getPageForUpdate(const ID& id, BigInt name)
    {
    	return getPage(id, Allocator::UPDATE, name);
    }

	PageG getPage(const ID& id, Int flags, BigInt name)
	{
		auto iter = update_log_.find(name);

		if (!(iter.is_found_eq(name) && find2ndEQ(iter, id)))
		{
			PageG old_page = txn_mgr_->getPage(txn_id_, id, name);

			if (flags == Allocator::READ)
			{
				return wrapPageG(old_page);
			}
			else {
				PageG new_page 	= allocator_->createPage(old_page->page_size(), name);
				ID new_gid 		= new_page->gid();

				CopyByteBuffer(old_page.page(), new_page.page(), old_page->page_size());

				new_page->gid() = new_gid;
				new_page->id() 	= id;

				if (!iter.is_found_eq(name))
				{
					iter = update_log_.create(name);
				}

				iter.insert2nd(id, UpdateLogValue(toInt(EntryStatus::UPDATED), new_gid));

				return wrapPageG(new_page);
			}
		}
		else
		{
			UpdateLogValue log_entry = iter.value();
			MEMORIA_ASSERT_TRUE(
					log_entry.first == toInt(EntryStatus::UPDATED) || // Check the Mark
					log_entry.first == toInt(EntryStatus::CREATED)
			);

			ID gid = log_entry.second;

			PageG page;

			if (flags == Allocator::READ)
			{
				page = allocator_->getPage(gid, name);
			}
			else {
				page = allocator_->getPageForUpdate(gid, name);
			}

			MEMORIA_ASSERT(page->id(), ==, id);

			return wrapPageG(page);
		}
	}

	virtual PageG updatePage(Shared* _shared, BigInt name)
	{
		MVCCPageShared* shared = static_cast<MVCCPageShared*>(_shared);

		ID id = shared->id();

		auto iter = update_log_.find(name);

		if (iter.is_found_eq(name) && find2ndEQ(iter, id))
		{
			return wrapPageG(shared, allocator_->updatePage(shared->delegate(), name));
		}
		else {
			Int page_size = shared->get()->page_size();

			PageG new_page 	= allocator_->createPage(page_size, name);
			ID new_gid 		= new_page->gid();

			CopyByteBuffer(shared->get(), new_page.page(), page_size);

			new_page->gid() = new_gid;
			new_page->id() 	= id;

			if (!iter.is_found_eq(name))
			{
				iter = update_log_.createNew(name);
			}

			iter.insert2nd(id, UpdateLogValue(toInt(EntryStatus::UPDATED), new_gid));

			return wrapPageG(new_page);
		}
	}

	virtual void removePage(const ID& id, BigInt name)
	{
		auto iter = update_log_.find(name);

		if (iter.is_found_eq(name) && find2ndEQ(iter, id))
		{
			UpdateLogValue value 	= iter.value();
			ID gid 					= value.second;

			if (value.first != toInt(EntryStatus::DELETED))
			{
				allocator_->removePage(gid, name);

				iter.setValue(UpdateLogValue(toInt(EntryStatus::DELETED), ID(0)));
			}
			else {
				//throw vapi::Exception(MA_SRC, SBuf()<<"Page id="<<id<<" gid="<<gid<<" has been already deleted.");
				cout<<"Page id="<<id<<" gid="<<gid<<" has been already deleted."<<endl;
			}
		}
		else {
			if (!iter.is_found_eq(name))
			{
				iter = update_log_.create(name);
			}

			iter.insert2nd(id, UpdateLogValue(toInt(EntryStatus::DELETED), ID(0))); // mark the page deleted
		}

		// Is it necessary to mark existing page guards DELETEd?
	}

	virtual PageG createPage(Int initial_size, BigInt name)
	{
		PageG new_page 	= allocator_->createPage(initial_size, name);
		ID new_gid 		= new_page->gid();
		ID new_id		= allocator_->newId();

		new_page->id()	= new_id;

		auto iter = update_log_.find(name);

		if (!iter.is_found_eq(name))
		{
			iter = update_log_.createNew(name);
		}

		iter.insert2nd(new_id, UpdateLogValue(toInt(EntryStatus::CREATED), new_gid));

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

	// Ctr Directory

	virtual ID getRootID(BigInt name)
	{
		if (name == TxnMgr::CtrDirectoryName)
		{
			if (ctr_directory_root_id_.isSet())
			{
				return ctr_directory_root_id_;
			}
			else {
				return txn_mgr_->getCtrDirectoryRootID(txn_id_);
			}
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
		if (name == TxnMgr::CtrDirectoryName)
		{
			ctr_directory_root_id_ = root;
		}
		else {
			auto iter = ctr_directory_.findKeyGE(name);

			if (iter.is_found_eq(name))
			{
				if (root.isSet())
				{
					EntryStatus status = static_cast<EntryStatus>(iter.label(0));

					iter.svalue() = CtrDirectoryValue(txn_id_, root);

					if (!(status == EntryStatus::CREATED || status == EntryStatus::UPDATED))
					{
						iter.set_label(0, toInt(EntryStatus::UPDATED));
					}
				}
				else {
					iter.svalue() = CtrDirectoryValue(txn_id_, root);
					iter.set_label(0, toInt(EntryStatus::DELETED));
				}
			}
			else if (root.isSet())
			{
				using Entry = typename CtrDirectory::Types::Entry;

				Entry entry;
				entry.key() 	= name;
				entry.value()	= CtrDirectoryValue(txn_id_, root);
				std::get<0>(entry.labels()) = toInt(EntryStatus::CREATED);

				//iter.insert(name, CtrDirectoryValue(txn_id_, root), toInt(EntryStatus::CREATED));
				iter.insert(entry);
			}
			else {
				throw vapi::Exception(MA_SRC, "Try to remove nonexistent root ID form root directory");
			}
		}
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
		if (name == TxnMgr::CtrDirectoryName)
		{
			// do nothing
		}
		else
		{
			auto iter = ctr_directory_.findKeyGE(name);
			if (iter.is_found_eq(name))
			{
				EntryStatus status = static_cast<EntryStatus>(iter.label(0));

				if (!(status == EntryStatus::CREATED || status == EntryStatus::UPDATED))
				{
					iter.set_label(0, toInt(EntryStatus::UPDATED));
				}
			}
			else {
				throw vapi::Exception(MA_SRC, SBuf()<<"CtrDirectory entry for name "<<name<<" is not found");
			}
		}
	}

	virtual BigInt currentTxnId() const	{
		return txn_id_;
	}



	// ITransation

	virtual BigInt commit()
	{
		if (status_ == TxnStatus::ACTIVE)
		{
			return txn_mgr_->commit(*this);
		}
		else if (status_ == TxnStatus::SNAPSHOT)
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
		auto iter = update_log_.Begin();

		while (!iter.isEnd())
		{
			BigInt name = iter.key();

			while(!iter.isEof())
			{
				UpdateLogValue entry = iter.value();

				EntryStatus status = static_cast<EntryStatus>(entry.first);

				if (status != EntryStatus::DELETED)
				{
					allocator_->removePage(entry.second, name);
				}

				iter.skipFw(1);
			}

			iter++;
		}

		update_log_.drop();
	}

	void forceRollback(const char* src, const SBuf& msg)
	{
		forceRollback(src, msg.str());
	}

	void forceRollback(const char* src, StringRef msg)
	{
		rollback();
		throw vapi::RollbackException(src, msg);
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

		result = ctr_directory_.checkTree() || result;
		result = update_log_.checkTree() 	|| result;

		return result;
	}

    virtual void flush(bool force_sync = false) {
    	return allocator_->flush(force_sync);
    }

    virtual void walkContainers(vapi::ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
    	walker->beginAllocator("MVCCTxn", allocator_descr);

    	dumpUpdateLog(walker);

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

    void dumpUpdateLog(vapi::ContainerWalker* walker)
    {
    	auto iter = update_log_.Begin();

    	walker->beginSection("UpdateLog");

    	while (!iter.isEnd())
    	{
    		BigInt name = iter.key();

    		walker->beginSection((SBuf()<<name).str().c_str());

    		while (!iter.isEof())
    		{
    			ID id 					= iter.key2();
    			UpdateLogValue value	= iter.value();

    			Int mark 	= value.first;
    			ID gid 		= value.second;

    			if (mark != toInt(EntryStatus::DELETED))
    			{
    				PageG page = allocator_->getPage(gid, -1); // fixme: provide valid Ctr name here

    				walker->singleNode((SBuf()<<id<<"__"<<mark<<"__"<<gid).str().c_str(), page.page());
    			}
    			else {
    				walker->content((SBuf()<<id<<"__"<<mark<<"__"<<gid).str().c_str(), "DELETED");
    			}

    			iter.skipFw(1);
    		}

    		walker->endSection();

    		iter++;
    	}

    	walker->endSection();
    }


	void clean() {
		update_log_.drop();
		allocator_->flush();
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
