
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

	typedef typename CtrTF<Profile, SMrkMap<BigInt, ID, 2>>::Type				CtrDirectory;
	typedef typename CtrDirectory::Types::Value									CtrDirectoryValue;
	typedef std::unique_ptr<CtrDirectory>										CtrDirectoryPtr;

	static const Int RootMapName												= 1;

private:

	ContainerMetadataRepository* metadata_;

	Allocator* 		allocator_;

	CommitHistory 	commit_history_;
	Roots			roots_;

	std::unordered_set<TxnPtr> transactions_;

	CtrDirectoryPtr root_map_;

	BigInt last_commited_txn_id_;

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
			// create initial RootMap

			last_commited_txn_id_ = allocator_->properties().newTxnId();

			root_map_ = CtrDirectoryPtr(new CtrDirectory(this, CTR_CREATE, RootMapName));

			allocator_->properties().setLastCommitId(last_commited_txn_id_);

			allocator_->properties().setMVCC(true);

			allocator_->commit();
		}
		else {
			last_commited_txn_id_ = allocator_->properties().lastCommitId();

			root_map_ = CtrDirectoryPtr(new CtrDirectory(this, CTR_FIND, RootMapName));
		}
	}

	virtual ~MVCCAllocator() {}

	CtrDirectory* roots()
	{
		return root_map_.get();
	}

	static void initMetadata()
	{
		CommitHistory::initMetadata();
		Roots::initMetadata();
		CtrDirectory::initMetadata();
	}

	ContainerMetadataRepository* getMetadata() const {
		return metadata_;
	}


	virtual PageG getPage(BigInt txn_id, const ID& id)
	{
		auto iter = findGIDInHistory(txn_id, id);

		MEMORIA_ASSERT_FALSE(iter.isEof());

		ID gid = iter.value().second;

		return allocator_->getPage(gid, Allocator::READ);
	}

	virtual ID getCtrDirectoryRootID(BigInt txn_id)
	{
		auto iter = roots_.findKey(txn_id);

		if (is_found(iter, txn_id))
		{
			return iter.value();
		}
		else {
			iter.dumpPath();

			throw Exception(MA_SRC, SBuf()<<"No container directory root ID for txn: "<<txn_id);
		}
	}

	virtual void setCtrDirectoryRootID(BigInt txn_id, const ID& root_id)
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

	virtual BigInt commited_txn_id() {
		return last_commited_txn_id_;
	}

	virtual TxnPtr begin()
	{
		return std::make_shared<TxnImpl>(this, newTxnId());
	}

	void commit(TxnImpl* txn)
	{
		//typename TxnImpl::CtrDirectory root_map(txn, CTR_FIND, 0);
	}

	BigInt newTxnId()
	{
		return allocator_->properties().newTxnId();
	}


	// IAllocator

	virtual PageG getPage(const ID& id, Int flags)
	{
		PageG page = getPage(last_commited_txn_id_, id);

		if (flags == Allocator::UPDATE)
		{
			page.update();
		}

		return page;
	}

	virtual PageG updatePage(Shared* shared)
	{
		MEMORIA_ASSERT(shared->id(), ==, shared->get()->gid());

		ID id = shared->get()->id();

		auto iter = findGIDInHistory(last_commited_txn_id_, id);

		if (iter.key2() != last_commited_txn_id_)
		{
			Int page_size = shared->get()->page_size();

			PageG new_page 	= allocator_->createPage(page_size);
			ID new_gid 		= new_page->gid();

			CopyByteBuffer(shared->get(), new_page.page(), page_size);

			new_page->gid() = new_gid;
			new_page->id() 	= id;

			iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::UPDATED), new_gid));

			return new_page;
		}
		else {
			return allocator_->updatePage(shared);
		}
	}

	virtual void removePage(const ID& id)
	{
		auto iter = findGIDInHistory(last_commited_txn_id_, id);

		if (iter.key2() != last_commited_txn_id_)
		{
			iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::DELETED), 0));
		}
		else {
			CommitHistoryValue value = iter.value();
			value.first = toInt(EntryStatus::DELETED);
			iter.setValue(value);
		}
	}

	virtual PageG createPage(Int initial_size)
	{
		PageG new_page 	= allocator_->createPage(initial_size);
		ID new_gid 		= new_page->gid();
		ID new_id		= allocator_->newId();
		new_page->id()	= new_id;

		auto iter = commit_history_.create(new_id);

		iter.insert2nd(last_commited_txn_id_, CommitHistoryValue(toInt(EntryStatus::DELETED), new_gid));

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
		if (name == RootMapName)
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
			auto iter = root_map_->findKey(name);

			if (is_found(iter, name))
			{
				return iter.value();
			}
			else {
				throw Exception(MA_SRC, SBuf()<<"Ctr root ID is not found for name: "<<name);
			}
		}
	}


	virtual void setRoot(BigInt name, const ID& root)
	{
		if (name == RootMapName)
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
			auto iter = root_map_->findKey(name);

			if (root.isSet())
			{
				if (is_found(iter, name))
				{
					iter.value() = root;
				}
				else {
					iter.insert(name, root, toInt(EntryStatus::CLEAN));
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
		if (name == RootMapName)
		{
			auto iter = roots_.findKey(last_commited_txn_id_);
			return !iter.isEnd();
		}
		else
		{
			auto iter = root_map_->findKey(name);
			return is_found(iter, name);
		}
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

		if (!iter.found())
		{
			cout<<txn_id<<" "<<id<<endl;
			iter.dumpPath();
		}

		MEMORIA_ASSERT_TRUE(iter.found());
		MEMORIA_ASSERT_TRUE(iter.blob_size() > 0);

		iter.find2ndLE(txn_id);

		return iter;
	}
};

}


#endif
