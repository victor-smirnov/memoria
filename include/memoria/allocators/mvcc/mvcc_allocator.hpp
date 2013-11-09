
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

	typedef IWalkableAllocator<PageType>										Allocator;

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
		auto iter = commit_history_.find(id);

		if (iter.found())
		{
			if (iter.find2ndLE(txn_id))
			{
				ID gid = iter.value().second;

				PageG page = allocator_->getPage(gid, Allocator::READ, name);

				page.shared()->set_allocator(this);

				return page;
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
			auto iter 	= ctr_directory_->findKeyGE(name);

			if (!iter.is_found_eq(name))
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
			auto iter 	= ctr_directory_->findKeyGE(name);

			if (iter.is_found_eq(name))
			{
				txn.forceRollback(MA_SRC, SBuf()<<"Create/exists conflict for container "<<name);
			}

			txn_iter++;
			txn_iter.selectFw(toInt(EntryStatus::UPDATED), 1);
		}


		txn_iter = txn_ctr_directory.select(toInt(EntryStatus::DELETED), 1);

		while (!txn_iter.isEnd())
		{
			BigInt name = txn_iter.key();
			auto iter 	= ctr_directory_->findKeyGE(name);

			if (iter.is_found_eq(name))
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

			auto iter = ctr_directory_->findKeyGE(name);
			MEMORIA_ASSERT_TRUE(iter.is_found_eq(name));

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

			auto iter = ctr_directory_->findKeyGE(name);
			MEMORIA_ASSERT_TRUE(!iter.is_found_eq(name));

			iter.insert(name, CtrDirectoryValue(last_commited_txn_id_, id), toInt(EntryStatus::CLEAN));

			importPages(txn.update_log(), name);

			txn_iter++;
			txn_iter.selectFw(toInt(EntryStatus::CREATED), 1);
		}



		txn_iter = txn_ctr_directory.select(toInt(EntryStatus::DELETED), 1);

		while (!txn_iter.isEnd())
		{
			BigInt name = txn_iter.key();

			auto iter = ctr_directory_->findKeyGE(name);

			if (iter.is_found_eq(name))
			{
				iter.remove();
			}
			else {
				txn_iter++;
			}

			importPages(txn.update_log(), name);

			txn_iter.selectFw(toInt(EntryStatus::DELETED), 1);
		}

		allocator_->properties().setLastCommitId(last_commited_txn_id_);

		allocator_->commit();
	}

	BigInt newTxnId()
	{
		return allocator_->properties().newTxnId();
	}


	// IAllocator

	virtual PageG getPage(const ID& id, Int flags, BigInt name)
	{
		auto iter = commit_history_.find(id);

		if (iter.found())
		{
			if (iter.find2ndLE(last_commited_txn_id_))
			{
				ID gid = iter.value().second;

				PageG old_page = allocator_->getPage(gid, Allocator::READ, name);
				old_page.shared()->set_allocator(this);

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


	virtual PageG updatePage(Shared* shared, BigInt name)
	{
		MEMORIA_ASSERT(shared->id(), ==, shared->get()->gid());

		ID id = shared->get()->id();

		auto iter = commit_history_.find(id);

		if (iter.found())
		{
			if (iter.find2ndLE(last_commited_txn_id_))
			{
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

		if (iter.found())
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
		PageG new_page 	= allocator_->createPage(initial_size, name);
		ID new_gid 		= new_page->gid();
		ID new_id		= allocator_->newId();
		new_page->id()	= new_id;

		auto iter = commit_history_.create(new_id);

		iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::CREATED), new_gid));

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
			auto iter = roots_.findKeyLE(last_commited_txn_id_);

			if (iter.is_found_eq(last_commited_txn_id_))
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
			auto iter = ctr_directory_->findKeyGE(name);

			if (root.isSet())
			{
				CtrDirectoryValue root_value(last_commited_txn_id_, root);

				if (iter.is_found_eq(name))
				{
					iter.value() = root_value;
				}
				else {
					iter.insert(name, root_value, toInt(EntryStatus::CLEAN));
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

    	walker->beginSnapshot("trunk");

    	ctr_directory_->walkTree(walker);

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

private:

	void dumpHistory(ContainerWalker* walker)
	{
		auto iter = commit_history_.Begin();

		walker->beginSection("CommitHistory");

		while (!iter.isEnd())
		{
			ID id = iter.key();

			walker->beginSection((SBuf()<<id).str().c_str());

			iter.findData();

			while (!iter.isEof())
			{
				BigInt txn_id 				= iter.key2();
				CommitHistoryValue value	= iter.value();

				Int mark 	= value.first;
				ID gid 		= value.second;

				PageG page = allocator_->getPage(gid, Allocator::READ, -1); // fixme: provide Ctr name here

				walker->singleNode((SBuf()<<txn_id<<"__"<<mark<<"__"<<gid).str().c_str(), page.page());

				iter.skipFw(1);
			}

			walker->endSection();

			iter++;
		}

		walker->endSection();
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
};

}


#endif
