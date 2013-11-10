
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
	typedef typename Base::Shared*												PageSharedPtr;

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

	static const Int CtrDirectoryName											= 0;


	class MVCCPageShared;
	typedef MVCCPageShared*														MVCCPageSharedPtr;

	typedef std::unordered_map<ID, MVCCPageSharedPtr, IDKeyHash, IDKeyEq>     	PageSharedMap;


	class MVCCPageShared: public Shared {

		PageSharedPtr delegate_;

		MVCCPageSharedPtr next_;
		MVCCPageSharedPtr prev_;

	public:

		MVCCPageShared() {
			next_ = prev_ = nullptr;
			init();
		}

		void init()
		{
			Shared::init();

			delegate_ = nullptr;
		}

		Int unref()
		{
			Int references = Shared::unref();

			if (references == 0)
			{
				unrefDelegate();
			}

			return references;
		}


		void setDelegate(Shared* delegate)
		{
			unrefDelegate();

			delegate_ = delegate;

			delegate->ref();
		}

		PageSharedPtr delegate() {
			return delegate_;
		}

		const PageSharedPtr delegate() const {
			return delegate_;
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

	private:
		void unrefDelegate()
		{
			if (delegate_ && delegate_->unref() == 0)
			{
				delegate_->allocator()->releasePage(delegate_);
				delegate_ = nullptr;
			}
		}
	};



	class SharedPool: public IntrusiveList<MVCCPageShared> {
		typedef IntrusiveList<MVCCPageShared> 	Base;
		typedef typename Base::size_type		size_type;

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

	// memory pool for PageShared objects

	SharedPool		shared_pool_;
	PageSharedMap	allocated_shared_objects_;

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
		shared_pool_(256, 256),
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
				return wrapPageG(allocator_->getPage(gid, name));
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

    virtual PageG getPage(const ID& id, BigInt name)
    {
    	auto iter = commit_history_.find(id);

    	if (iter.found())
    	{
    		if (iter.find2ndLE(last_commited_txn_id_))
    		{
    			ID gid = iter.value().second;

    			return wrapPageG(allocator_->getPage(gid, name));
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

    	if (iter.found())
    	{
    		if (iter.find2ndLE(last_commited_txn_id_))
    		{
    			ID gid = iter.value().second;

    			PageG old_page = allocator_->getPage(gid, name);

    			if (iter.key2() == last_commited_txn_id_)
    			{
    				return wrapPageG(old_page);
    			}
    			else
    			{
    				PageG new_page 	= allocator_->createPage(old_page->page_size(), name);
    				ID new_gid 		= new_page->gid();

    				CopyByteBuffer(old_page.page(), new_page.page(), old_page->page_size());

    				new_page->gid() = new_gid;
    				new_page->id() 	= id;

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

		if (iter.found())
		{
			if (iter.find2ndLE(last_commited_txn_id_))
			{
				if (iter.key2() < last_commited_txn_id_)
				{
					Int page_size = shared->get()->page_size();

					PageG new_page 	= allocator_->createPage(page_size, name);
					ID new_gid 		= new_page->gid();

					CopyByteBuffer(shared->get(), new_page.page(), page_size);

					new_page->gid() = new_gid;
					new_page->id() 	= id;

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
		PageG new_page 	= allocator_->createPage(initial_size, name);
		ID new_gid 		= new_page->gid();
		ID new_id		= allocator_->newId();
		new_page->id()	= new_id;

		auto iter = commit_history_.create(new_id);

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

				PageG page = allocator_->getPage(gid, -1); // fixme: provide valid Ctr name here

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
