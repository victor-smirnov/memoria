
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

	typedef IAllocator<PageType>												Allocator;
	typedef PageType                                                    		Page;
	typedef typename Page::ID                                           		ID;
	typedef PageGuard<Page, Allocator>                                     		PageG;
	typedef typename Base::Shared                                      			Shared;

	typedef typename Base::CtrShared                                         	CtrShared;

	typedef typename CtrTF<Profile, DblMrkMap<BigInt, ID, 2>>::Type				UpdateLog;

	typedef std::unordered_map<BigInt, CtrShared*>                              CtrSharedMap;

	typedef typename TxnMgr::CtrDirectory										CtrDirectory;

	typedef typename UpdateLog::Types::Value									UpdateLogValue;
	typedef typename CtrDirectory::Types::Value									CtrDirectoryValue;



private:

	Allocator*		allocator_;

	TxnMgr* 		txn_mgr_;

	BigInt 			txn_id_;

	ID				ctr_directory_root_id_;

	UpdateLog		update_log_;
	CtrDirectory	ctr_directory_;

public:

	MVCCTxn(TxnMgr* txn_mgr, BigInt txn_id):
		Base(txn_mgr->allocator()),
		allocator_(txn_mgr->allocator()),
		txn_mgr_(txn_mgr),
		txn_id_(txn_id),
		ctr_directory_root_id_(0),
		update_log_(txn_mgr->allocator(), CTR_CREATE, txn_id_),
		ctr_directory_(this, CTR_FIND, TxnMgr::CtrDirectoryName)
	{

	}

	virtual ~MVCCTxn() {}

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

	virtual PageG getPage(const ID& id, Int flags, BigInt name)
	{
		auto iter = update_log_.find(name);

		bool found;

		if (iter.found())
		{
			iter.find2ndLE(id);

			found = (!iter.isEof()) && iter.key2() == id;
		}
		else {
			found = false;
		}

		if (!found)
		{
			PageG old_page = txn_mgr_->getPage(txn_id_, id, name);

			if (flags == Allocator::READ)
			{
				old_page.shared()->set_allocator(this);
				return old_page;
			}
			else {
				PageG new_page 	= allocator_->createPage(old_page->page_size(), name);
				ID new_gid 		= new_page->gid();
				new_page.shared()->set_allocator(this);

				CopyByteBuffer(old_page.page(), new_page.page(), old_page->page_size());

				new_page->gid() = new_gid;
				new_page->id() 	= id;

				if (!iter.found())
				{
					iter = update_log_.create(name);
				}

				iter.insert2nd(id, UpdateLogValue(toInt(EntryStatus::UPDATED), new_gid));

				return new_page;
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

			PageG page = allocator_->getPage(gid, flags, name);
			page.shared()->set_allocator(this);

			return page;
		}
	}

	virtual PageG updatePage(Shared* shared, BigInt name)
	{
		MEMORIA_ASSERT(shared->id(), ==, shared->get()->gid());

		ID id = shared->get()->id();

		auto iter = findGIDInHistory(id, name);

		if (h_is_not_found(iter, id))
		{
			Int page_size = shared->get()->page_size();

			PageG new_page 	= allocator_->createPage(page_size, name);
			ID new_gid 		= new_page->gid();
			new_page.shared()->set_allocator(this);

			CopyByteBuffer(shared->get(), new_page.page(), page_size);

			new_page->gid() = new_gid;
			new_page->id() 	= id;

			if (!iter.found())
			{
				iter = update_log_.create(name);
			}

			iter.insert2nd(id, UpdateLogValue(toInt(EntryStatus::UPDATED), new_gid));

			return new_page;
		}
		else {
			return allocator_->updatePage(shared, name);
		}
	}

	virtual void removePage(const ID& id, BigInt name)
	{
		auto iter = findGIDInHistory(id, name);

		if (h_is_not_found(iter, id))
		{
			if (!iter.found())
			{
				iter = update_log_.create(name);
			}

			iter.insert2nd(id, UpdateLogValue(toInt(EntryStatus::DELETED), ID(0))); // mark the page deleted
		}
		else {
			UpdateLogValue value 	= iter.value();
			ID gid 					= value.second;

			if (value.first != toInt(EntryStatus::DELETED))
			{
				allocator_->removePage(gid, name);

				iter.value() = UpdateLogValue(toInt(EntryStatus::DELETED), ID(0));
			}
			else {
				throw vapi::Exception(MA_SRC, SBuf()<<"Page id="<<id<<" gid="<<gid<<" has been already deleted.");
			}
		}
	}

	virtual PageG createPage(Int initial_size, BigInt name)
	{
		PageG new_page 	= allocator_->createPage(initial_size, name);
		ID new_gid 		= new_page->gid();
		ID new_id		= allocator_->newId();
		new_page.shared()->set_allocator(this);

		new_page->id()	= new_id;

		auto iter = update_log_.find(name);

		if (!iter.found())
		{
			iter = update_log_.create(name);
		}

		iter.insert2nd(new_id, UpdateLogValue(toInt(EntryStatus::CREATED), new_gid));

		return new_page;
	}

	virtual void resizePage(Shared* page, Int new_size)
	{
		allocator_->resizePage(page, new_size);
	}

	virtual void releasePage(Shared* shared)
	{
		allocator_->releasePage(shared);
	}

	// Ctr Directory

	virtual ID getRootID(BigInt name)
	{
		if (name != TxnMgr::CtrDirectoryName)
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
		else if (ctr_directory_root_id_.isSet())
		{
			return ctr_directory_root_id_;
		}
		else {
			return txn_mgr_->getCtrDirectoryRootID(txn_id_);
		}
	}

	virtual void setRoot(BigInt name, const ID& root)
	{
		if (name != TxnMgr::CtrDirectoryName)
		{
			auto iter = ctr_directory_.findKey(name);

			if (is_found(iter, name))
			{
				if (root.isSet())
				{
					EntryStatus status = static_cast<EntryStatus>(iter.mark());

					iter.value() = CtrDirectoryValue(txn_id_, root);

					if (!(status == EntryStatus::CREATED || status == EntryStatus::UPDATED))
					{
						iter.setMark(toInt(EntryStatus::UPDATED));
					}
				}
				else {
					iter.value() = CtrDirectoryValue(txn_id_, root);
					iter.setMark(toInt(EntryStatus::DELETED));
				}
			}
			else if (root.isSet())
			{
				iter.insert(name, CtrDirectoryValue(txn_id_, root), toInt(EntryStatus::CREATED));
			}
			else {
				throw vapi::Exception(MA_SRC, "Try to remove nonexistent root ID form root directory");
			}
		}
		else {
			ctr_directory_root_id_ = root;
		}
	}

	virtual bool hasRoot(BigInt name)
	{
		auto iter = ctr_directory_.findKey(name);
		return is_found(iter, name);
	}

	virtual void markUpdated(BigInt name)
	{
		auto iter = ctr_directory_.findKey(name);
		if (is_found(iter, name))
		{
			EntryStatus status = static_cast<EntryStatus>(iter.mark());

			if (!(status == EntryStatus::CREATED || status == EntryStatus::UPDATED))
			{
				iter.setMark(toInt(EntryStatus::UPDATED));
			}
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"CtrDirectory entry for name "<<name<<" is not found");
		}
	}

	virtual BigInt currentTxnId() const	{
		return txn_id_;
	}



	// ITransation

	virtual void commit()
	{
		txn_mgr_->commit(*this);
	}

	virtual void rollback()
	{
		auto iter = update_log_.Begin();

		while (!iter.isEnd())
		{
			BigInt name = iter.key();

			iter.findData();

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

private:
	template <typename Iterator, typename Key>
	static bool is_found(Iterator& iter, const Key& key)
	{
		return (!iter.isEnd()) && iter.key() == key;
	}

	template <typename Iterator, typename Key>
	static bool is_not_found(Iterator& iter, const Key& key)
	{
		return iter.isEnd() || iter.key() != key;
	}

	bool h_is_not_found(const typename UpdateLog::Iterator& iter, const ID& id) const
	{
		return (!iter.found()) || iter.isEof() || iter.key2() != id;
	}

	bool h_is_found(const typename UpdateLog::Iterator& iter, const ID& id) const
	{
		return iter.found() && (!iter.isEof()) && iter.key2() == id;
	}

	typename UpdateLog::Iterator findGIDInHistory(const ID& id, BigInt name)
	{
		auto iter = update_log_.find(name);

		if (iter.found())
		{
			iter.find2ndLE(id);
		}

		return iter;
	}
};

}


#endif
