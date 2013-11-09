
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
#include <memoria/allocators/mvcc/mvcc_tools.hpp>

#include <memoria/core/exceptions/memoria.hpp>
#include <memoria/metadata/tools.hpp>



#include <unordered_set>
#include <memory>

namespace memoria {

template <typename Profile, typename PageType>
class MVCCAllocator: public MVCCAllocatorBase<IMVCCAllocator<PageType>> {
	typedef MVCCAllocatorBase<IMVCCAllocator<PageType>>							Base;

public:
	typedef MVCCAllocator<Profile, PageType>									MyType;

	typedef IAllocator<PageType>												Allocator;

	typedef typename Base::ID													ID;
	typedef typename Base::PageG												PageG;
	typedef typename Base::Txn													Txn;
	typedef typename Base::TxnPtr												TxnPtr;
	typedef typename Base::CtrShared											CtrShared;
	typedef typename Base::Shared												Shared;

	typedef MVCCTxn<Profile, MyType, PageType>									TxnImpl;

	typedef typename CtrTF<Profile, DblMrkMap<BigInt, ID, 2>>::Type				CommitHistory;
	typedef typename CtrTF<Profile, Map<BigInt, ID>>::Type						Roots;

	typedef typename CommitHistory::Types::Value								CommitHistoryValue;
	typedef std::pair<BigInt, CommitHistoryValue>								CommitHistoryEntry;

	typedef std::unordered_map<BigInt, CtrShared*>                              CtrSharedMap;

	typedef TxnValue<ID>														CtrDirectoryTxnValue;

	typedef typename CtrTF<Profile, SMrkMap<BigInt, CtrDirectoryTxnValue, 2>>::Type	CtrDirectory;
	typedef typename CtrDirectory::Types::Value									CtrDirectoryValue;
	typedef std::unique_ptr<CtrDirectory>										CtrDirectoryPtr;

	static const Int CtrDirectoryName												= 1;

private:

	ContainerMetadataRepository* metadata_;

	Allocator* 		allocator_;

	CommitHistory 	commit_history_;
	Roots			roots_;

	std::unordered_set<TxnPtr> transactions_;

	CtrDirectoryPtr ctr_directory_;

	BigInt 			last_commited_txn_id_;

public:

	MVCCAllocator(Allocator* allocator):
		Base(allocator),
		metadata_(MetadataRepository<Profile>::getMetadata()),
		allocator_(allocator),
		commit_history_(allocator, CTR_CREATE | CTR_FIND, 10),
		roots_(allocator, CTR_CREATE | CTR_FIND, 11)
	{
		initMetadata();

		if (commit_history_.size() == 0)
		{
			// create initial CtrDirectory

			last_commited_txn_id_ = allocator_->properties().newTxnId();

			ctr_directory_ = CtrDirectoryPtr(new CtrDirectory(this, CTR_CREATE, CtrDirectoryName));

			allocator_->properties().setLastCommitId(last_commited_txn_id_);

			allocator_->properties().setMVCC(true);

			allocator_->commit();
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

	static void initMetadata()
	{
		CommitHistory::initMetadata();
		Roots::initMetadata();
		CtrDirectory::initMetadata();
		TxnImpl::UpdateLog::initMetadata();
	}

	ContainerMetadataRepository* getMetadata() const {
		return metadata_;
	}


	virtual PageG getPage(BigInt txn_id, const ID& id, BigInt name)
	{
		auto iter = findGIDInHistory(txn_id, id);

		MEMORIA_ASSERT_FALSE(iter.isEof());

		ID gid = iter.value().second;

		PageG page = allocator_->getPage(gid, Allocator::READ, name);

		page.shared()->set_allocator(this);

		return page;
	}

	virtual ID getCtrDirectoryRootID(BigInt txn_id)
	{
		auto iter = findLE(roots_, txn_id);

		if (!iter.isEnd())
		{
			return iter.value();
		}
		else {
			iter.dumpPath();

			throw Exception(MA_SRC, SBuf()<<"No container directory root ID for txn: "<<txn_id);
		}
	}



	virtual BigInt commited_txn_id() {
		return last_commited_txn_id_;
	}

	virtual TxnPtr begin()
	{
		return std::make_shared<TxnImpl>(this, newTxnId());
	}

	void commit(TxnImpl& txn)
	{
		auto& txn_ctr_directory = txn.ctr_directory();

		// Check containers for conflicts

		auto txn_iter = txn_ctr_directory.select(toInt(EntryStatus::UPDATED), 1);

		while (!txn_iter.isEnd())
		{
			BigInt name = txn_iter.key();
			auto iter 	= ctr_directory_->findKey(name);

			if (is_not_found(iter, name))
			{
				txn.forceRollback(MA_SRC, SBuf()<<"Update non-existent/removed container "<<name);
			}
			else {
				CtrDirectoryValue txn_entry = txn_iter.value();
				CtrDirectoryValue entry 	= iter.value();

				BigInt src_txn_id = txn_entry.txn_id();
				BigInt tgt_txn_id = entry.txn_id();

				if (tgt_txn_id > src_txn_id)
				{
					txn.forceRollback(MA_SRC, SBuf()<<"Update/update conflict for container "<<name);
				}
			}

			txn_iter++;
			txn_iter.selectFw(toInt(EntryStatus::UPDATED), 1);
		}



		txn_iter = txn_ctr_directory.select(toInt(EntryStatus::CREATED), 1);

		while (!txn_iter.isEnd())
		{
			BigInt name = txn_iter.key();
			auto iter 	= ctr_directory_->findKey(name);

			if (is_found(iter, name))
			{
				iter.dumpPath();
				txn_ctr_directory.Begin().dumpPath();

				txn.forceRollback(MA_SRC, SBuf()<<"Create/exists conflict for container "<<name);
			}

			txn_iter++;
			txn_iter.selectFw(toInt(EntryStatus::UPDATED), 1);
		}


		txn_iter = txn_ctr_directory.select(toInt(EntryStatus::DELETED), 1);

		while (!txn_iter.isEnd())
		{
			BigInt name = txn_iter.key();
			auto iter 	= ctr_directory_->findKey(name);

			if (is_found(iter, name))
			{
				CtrDirectoryValue txn_entry = txn_iter.value();
				CtrDirectoryValue entry 	= iter.value();

				BigInt src_txn_id = txn_entry.txn_id();
				BigInt tgt_txn_id = entry.txn_id();

				if (tgt_txn_id > src_txn_id)
				{
					txn.forceRollback(MA_SRC, SBuf()<<"Delete/update conflict for container "<<name);
				}
			}

			txn_iter++;
			txn_iter.selectFw(toInt(EntryStatus::UPDATED), 1);
		}


		// Commit changes to commit_history_

		last_commited_txn_id_ = newTxnId();

		txn_iter = txn_ctr_directory.select(toInt(EntryStatus::UPDATED), 1);

		while (!txn_iter.isEnd())
		{
			BigInt name = txn_iter.key();
			ID id = txn_iter.value().value().value();

			auto iter = ctr_directory_->findKey(name);
			MEMORIA_ASSERT_TRUE(is_found(iter, name));

			iter.value() = CtrDirectoryValue(last_commited_txn_id_, id);

			importPages(txn.update_log(), name);

			txn_iter++;
			txn_iter.selectFw(toInt(EntryStatus::UPDATED), 1);
		}


		txn_iter = txn_ctr_directory.select(toInt(EntryStatus::CREATED), 1);

		while (!txn_iter.isEnd())
		{
			BigInt name = txn_iter.key();
			ID id = txn_iter.value().value().value();

			auto iter = ctr_directory_->findKey(name);
			MEMORIA_ASSERT_TRUE(is_not_found(iter, name));

			iter.insert(name, CtrDirectoryValue(last_commited_txn_id_, id), toInt(EntryStatus::CLEAN));

			importPages(txn.update_log(), name);

			txn_iter++;
			txn_iter.selectFw(toInt(EntryStatus::CREATED), 1);
		}



		txn_iter = txn_ctr_directory.select(toInt(EntryStatus::DELETED), 1);

		while (!txn_iter.isEnd())
		{
			BigInt name = txn_iter.key();

			auto iter = ctr_directory_->findKey(name);

			if (is_found(iter, name))
			{
				iter.remove();
			}
			else {
				txn_iter++;
			}

			importPages(txn.update_log(), name);

			txn_iter.selectFw(toInt(EntryStatus::DELETED), 1);
		}

		allocator_->commit();


//		cout<<"Commit History"<<endl;
//		commit_history_.Begin().dumpPath();
//
//		cout<<"Ctr Directory"<<endl;
//		ctr_directory_->Begin().dumpPath();
//
//		cout<<"Roots"<<endl;
//		roots_.Begin().dumpPath();
	}

	BigInt newTxnId()
	{
		return allocator_->properties().newTxnId();
	}


	// IAllocator

	virtual PageG getPage(const ID& id, Int flags, BigInt name)
	{
		auto iter = findGIDInHistory(last_commited_txn_id_, id);

		MEMORIA_ASSERT_FALSE(iter.isEof());

		ID gid = iter.value().second;

		PageG old_page = allocator_->getPage(gid, Allocator::READ, name);

		if (flags == Allocator::READ)
		{
			return old_page;
		}
		else if (iter.key2() != last_commited_txn_id_)
		{
			PageG new_page 	= allocator_->createPage(old_page->page_size(), name);
			ID new_gid 		= new_page->gid();
			new_page.shared()->set_allocator(this);

			CopyByteBuffer(old_page.page(), new_page.page(), old_page->page_size());

			new_page->gid() = new_gid;
			new_page->id() 	= id;

			iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::UPDATED), new_gid));

			return new_page;
		}
		else {
			old_page.update(name);
			return old_page;
		}
	}




	virtual PageG updatePage(Shared* shared, BigInt name)
	{
		MEMORIA_ASSERT(shared->id(), ==, shared->get()->gid());

		ID id = shared->get()->id();

		auto iter = findGIDInHistory(last_commited_txn_id_, id);

		if (iter.key2() != last_commited_txn_id_)
		{
			Int page_size = shared->get()->page_size();

			PageG new_page 	= allocator_->createPage(page_size, name);
			new_page.shared()->set_allocator(this);

			ID new_gid 		= new_page->gid();

			CopyByteBuffer(shared->get(), new_page.page(), page_size);

			new_page->gid() = new_gid;
			new_page->id() 	= id;

			iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::UPDATED), new_gid));

			return new_page;
		}
		else {
			PageG updated = allocator_->updatePage(shared, name);
			updated.shared()->set_allocator(this);

			return updated;
		}
	}

	virtual void removePage(const ID& id, BigInt name)
	{
		auto iter = findGIDInHistory(last_commited_txn_id_, id);

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
	}

	virtual PageG createPage(Int initial_size, BigInt name)
	{
		PageG new_page 	= allocator_->createPage(initial_size, name);
		ID new_gid 		= new_page->gid();
		ID new_id		= allocator_->newId();
		new_page->id()	= new_id;

		auto iter = commit_history_.create(new_id);

		iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::DELETED), new_gid));

		new_page.shared()->set_allocator(this);

		return new_page;
	}

	virtual void resizePage(Shared* page, Int new_size)
	{
		return allocator_->resizePage(page, new_size);
	}

	virtual void releasePage(Shared* shared)
	{
		allocator_->releasePage(shared);
	}

	virtual ID getRootID(BigInt name)
	{
		if (name == CtrDirectoryName)
		{
			auto iter = roots_.findKey(last_commited_txn_id_);

			if (!iter.isEnd())
			{
				return iter.value();
			}
			else {
				throw Exception(MA_SRC, "CtrDirectory root ID is not found");
			}
		}
		else
		{
			auto iter = ctr_directory_->findKey(name);

			if (is_found(iter, name))
			{
				return iter.value().value().value();
			}
			else {
				throw Exception(MA_SRC, SBuf()<<"Ctr root ID is not found for name: "<<name);
			}
		}
	}

	virtual void markUpdated(BigInt name)
	{

	}

	virtual BigInt currentTxnId() const
	{
		return last_commited_txn_id_;
	}


	virtual void setRoot(BigInt name, const ID& root)
	{
		if (name == CtrDirectoryName)
		{
			auto iter = roots_.findKey(last_commited_txn_id_);

			if (is_found(iter, last_commited_txn_id_))
			{
				if (root.isSet())
				{
					iter.value() = root;
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
			auto iter = ctr_directory_->findKey(name);

			if (root.isSet())
			{
				CtrDirectoryValue root_value(last_commited_txn_id_, root);

				if (is_found(iter, name))
				{
					iter.value() = root_value;
				}
				else {
					iter.insert(name, root_value, toInt(EntryStatus::CLEAN));
				}
			}
			else {
				if (is_found(iter, name))
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
			auto iter = roots_.findKey(last_commited_txn_id_);
			return !iter.isEnd();
		}
		else
		{
			auto iter = ctr_directory_->findKey(name);
			return is_found(iter, name);
		}
	}

	void dumpPage(const PageG& page)
	{
		auto page_metadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

		vapi::dumpPageData(page_metadata, page.page(), std::cout);
	}


	void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
		walker->beginAllocator("MVCCAllocator", allocator_descr);
    	walker->beginSnapshot("trunk");

    	auto iter = ctr_directory_->Begin();

    	while (!iter.isEnd())
    	{
    		BigInt ctr_name = iter.key();
    		ID root_id		= iter.value().value().value();

    		PageG page 		= this->getPage(root_id, Base::READ, ctr_name);

    		ContainerMetadata* ctr_meta = metadata_->getContainerMetadata(page->ctr_type_hash());

    		ctr_meta->getCtrInterface()->walk(&page->id(), ctr_name, this, walker);

    		iter++;
    	}

    	walker->endSnapshot();
    	walker->endAllocator();
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

	typename CommitHistory::Iterator findGIDInHistory(BigInt txn_id, const ID& id)
	{
		auto iter = commit_history_.find(id);

		if (!iter.found()) {
			int a = 0; a++;
		}

		MEMORIA_ASSERT_TRUE(iter.found());
		MEMORIA_ASSERT_TRUE(iter.blob_size() > 0);

		iter.find2ndLE(txn_id);

		if (iter.isEof() || iter.key2() > txn_id)
		{
			iter.skipBw(1);
		}

		if (!iter.isEof())
		{
			MEMORIA_ASSERT(iter.key2(), <=, txn_id);
		}

		return iter;
	}

	template <typename Ctr>
	void importPages(Ctr& ctr, BigInt name)
	{
		auto iter = ctr.find(name);
		MEMORIA_ASSERT_TRUE(iter.found());

		iter.findData();

		while (!iter.isEof())
		{
			typename Ctr::Types::Value value = iter.value();

			EntryStatus status	= static_cast<EntryStatus>(value.first);
			ID gid				= value.second;
			ID id 				= iter.key2();

			if (status == EntryStatus::CREATED)
			{
				auto h_iter = commit_history_.createNew(id);

				h_iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::CREATED), gid));
			}
			else if (status == EntryStatus::UPDATED)
			{
				auto h_iter = commit_history_.find(id);
				MEMORIA_ASSERT_TRUE(h_iter.found());

				h_iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::UPDATED), gid));
			}
			else if (status == EntryStatus::DELETED)
			{
				auto h_iter = commit_history_.find(id);
				MEMORIA_ASSERT_TRUE(h_iter.found());

				h_iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::DELETED), ID(0)));
			}
			else {
				throw vapi::Exception(MA_SRC, SBuf()<<"Unknown entry status value: "<<toInt(status));
			}

			iter.skipFw(1);
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


	typename Roots::Iterator findLE(Roots& ctr, BigInt key)
	{
		auto iter = ctr.findKey(key);

		if (iter.isEnd() || iter.key() > key)
		{
			iter--;
		}

		if (!iter.isEnd())
		{
			MEMORIA_ASSERT(iter.key(), <=, key);
		}

		return iter;
	}
};

}


#endif
