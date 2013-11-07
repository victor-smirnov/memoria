
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

	typedef typename CtrTF<Profile, MrkMap<BigInt, ID>>::Type					UpdateLog;

	typedef std::unordered_map<BigInt, CtrShared*>                              CtrSharedMap;

	typedef typename TxnMgr::CtrDirectory										CtrDirectory;

	typedef typename UpdateLog::Types::Value									UpdateLogValue;
	typedef typename CtrDirectory::Types::Value									CtrDirectoryValue;



private:

	Allocator*		allocator_;

	TxnMgr* 		txn_mgr_;

	BigInt 			txn_id_;

	UpdateLog		update_log_;
	CtrDirectory	ctr_directory_;

public:

	MVCCTxn(TxnMgr* txn_mgr, BigInt txn_id):
		Base(txn_mgr->allocator()),
		allocator_(txn_mgr->allocator()),
		txn_mgr_(txn_mgr),
		txn_id_(txn_id),
		update_log_(txn_mgr->allocator(), CTR_CREATE, txn_id_),
		ctr_directory_(this, CTR_FIND, TxnMgr::CtrDirectoryName)
	{
		UpdateLog::initMetadata();
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

	virtual BigInt txn_id() const
	{
		return txn_id_;
	}

	virtual PageG getPage(const ID& id, Int flags, BigInt name)
	{
		auto iter = update_log_.find(id);

		if (is_not_found(iter, id))
		{
			PageG old_page = txn_mgr_->getPage(txn_id_, id, name);

			if (flags == Allocator::READ)
			{
				return old_page;
			}
			else {
				PageG new_page 	= allocator_->createPage(old_page->page_size(), name);
				ID new_gid 		= new_page->gid();

				CopyByteBuffer(old_page.page(), new_page.page(), old_page->page_size());

				new_page->gid() = new_gid;
				new_page->id() 	= id;

				iter.insert(id, UpdateLogValue(0, new_gid));

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

			return allocator_->getPage(gid, flags, name);
		}
	}

	virtual PageG updatePage(Shared* shared, BigInt name)
	{
		MEMORIA_ASSERT(shared->id(), ==, shared->get()->gid());

		ID id = shared->get()->id();

		auto iter = update_log_.find(id);

		if (is_not_found(iter, id))
		{
			UpdateLogValue entry = iter.value();

			MEMORIA_ASSERT(entry.second, ==, shared->id());

			Int page_size = shared->get()->page_size();

			PageG new_page 	= allocator_->createPage(page_size, name);
			ID new_gid 		= new_page->gid();

			CopyByteBuffer(shared->get(), new_page.page(), page_size);

			new_page->gid() = new_gid;
			new_page->id() 	= id;

			iter.insert(id, UpdateLogValue(toInt(EntryStatus::UPDATED), new_gid));

			return new_page;
		}
		else {
			return allocator_->updatePage(shared, name);
		}
	}

	virtual void removePage(const ID& id, BigInt name)
	{
		auto iter = update_log_.find(id);

		if (is_not_found(iter, id))
		{
			PageG page = txn_mgr_->getPage(txn_id_, id, name);

			iter.insert(id, UpdateLogValue(toInt(EntryStatus::DELETED), page->gid())); // mark the page deleted
		}
		else {
			ID gid = iter.value();
			iter.value() = UpdateLogValue(toInt(EntryStatus::DELETED), gid);
		}
	}

	virtual PageG createPage(Int initial_size, BigInt name)
	{
		PageG new_page 	= allocator_->createPage(initial_size, name);
		ID new_gid 		= new_page->gid();
		ID new_id		= allocator_->newId();

		new_page->id()	= new_id;

		update_log_.insertIFNotExists(new_id) = UpdateLogValue(toInt(EntryStatus::CREATED), new_gid);

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
		if (name > 0)
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
		else {
			return txn_mgr_->getRootID(txn_id_);
		}
	}

	virtual void setRoot(BigInt name, const ID& root)
	{
		if (name > 0)
		{
			auto iter = ctr_directory_.find(name);

			if (is_found(iter, name))
			{
				if (root.isSet())
				{
					//iter.value() = RootMapValue(toInt(EntryStatus::UPDATED), root);
					iter.value() = root;
					iter.setMark(toInt(EntryStatus::UPDATED));
				}
				else {
					//iter.value() = RootMapValue(toInt(EntryStatus::DELETED), root);

					iter.value() = root;
					iter.setMark(toInt(EntryStatus::DELETED));
				}
			}
			else if (root.isSet())
			{
				//iter.insert(name, RootMapValue(toInt(EntryStatus::CREATED), root));
				iter.insert(name, root, toInt(EntryStatus::CREATED));
			}
			else {
				throw vapi::Exception(MA_SRC, "Try to remove nonexistent root ID form root directory");
			}
		}
		else {
			return txn_mgr_->setCtrDirectoryRootID(txn_id_, root);
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
			Int mark = iter.mark();

			if (mark == toInt(EntryStatus::CLEAN))
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
			UpdateLogValue entry = iter.value();

			auto mark = entry.first;
			if (mark == 0 || mark == 1)
			{
				allocator_->removePage(entry.second, -1);
			}

			iter++;
		}

		update_log_.drop();
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
};

}


#endif
