
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_ALLOCATORS_FILE_ALLOCATOR_HPP
#define _MEMORIA_ALLOCATORS_FILE_ALLOCATOR_HPP

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/containers/map/map_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>

#include <memoria/core/tools/pool.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/file.hpp>
#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/types/types.hpp>

#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/page.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/allocators/file/superblock_ctr.hpp>

#include <memoria/core/tools/lru_cache.hpp>



#include <malloc.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>

namespace memoria {




template <typename Profile, typename PageType, typename TxnType = EmptyType>
class FileAllocator: public AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType> >::Type {

    typedef typename AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType> >::Type Base;

public:
    typedef typename Base::Page                                                 Page;

    typedef typename Base::PageG                                                PageG;
    typedef typename Base::Shared                                               Shared;
    typedef typename Base::CtrShared                                            CtrShared;
    typedef typename Page::ID                                                   ID;

    typedef SuperblockCtr<ID>													SuperblockCtrType;

    typedef Ctr<typename CtrTF<Profile, Sequence<1, true>>::CtrTypes>           BlockMapType;
    typedef Ctr<typename CtrTF<Profile, Map<BigInt, UBigInt>>::CtrTypes>       	IDMapType;
    typedef Ctr<typename CtrTF<Profile, Root>::CtrTypes>       					RootMapType;

    typedef IJournaledAllocator<Page>											JournaledAllocator;
    typedef IWalkableAllocator<Page>											WalkableAllocator;

private:
    typedef FileAllocator<Profile, PageType, TxnType>                           MyType;
    typedef typename Base::Page*                                                PagePtr;



    template <typename Key, typename Value>
    using IDHashMap = std::unordered_map<Key, Value, IDKeyHash, IDKeyEq>;

    template <typename Entry>
    struct PageCacheEvictionPredicate {
    	bool operator()(const Entry& entry)
    	{
    		return !(entry.is_updated() || entry.shared());
    	}
    };

    enum class PageOp {
    	UPDATE = Base::Shared::UPDATE, DELETE = Base::Shared::DELETE, NONE
    };

    class MyShared;
    typedef MyShared* MySharedPtr;

    template <typename Node>
    struct LRUNode: Node {

    	PagePtr page_;
    	PagePtr front_page_;

    	PageOp  page_op_;

    	UBigInt position_;

    	MySharedPtr shared_;

    	LRUNode()
    	{
    		this->reset();
    	}

    	LRUNode(const ID& gid)
    	{
    		this->reset();

    		this->key() = gid;
    	}

    	LRUNode(const ID& gid, UBigInt position)
    	{
    		this->reset();

    		this->key() 		= gid;
    		this->position() 	= position;
    	}

    	void reset()
    	{
    		Node::reset();

    		page_ = front_page_ = nullptr;

    		page_op_ 	= PageOp::NONE;
    		position_	= 0;
    		shared_		= nullptr;
    	}

    	PagePtr& page() {return page_;}
    	const PagePtr& page() const {return page_;}

    	PagePtr& front_page() {return front_page_;}
    	const PagePtr& front_page() const {return front_page_;}

    	PagePtr current_page() const {
    		return front_page_ != nullptr ? front_page_ : page_;
    	}

    	PageOp& page_op() {
    		return page_op_;
    	}

    	const PageOp& page_op() const {
    		return page_op_;
    	}

    	UBigInt& position() {
    		return position_;
    	}

    	const UBigInt& position() const {
    		return position_;
    	}

    	bool is_updated() const
    	{
    		return page_op_ != PageOp::NONE;
    	}

    	MySharedPtr& shared() {return shared_;}
    	const MySharedPtr& shared() const {return shared_;}
    };



    typedef LRUCache<
    		ID,
    		PageCacheEvictionPredicate,
    		LRUNode,
    		IDHashMap
    > 																			PageCacheType;

    typedef typename PageCacheType::Entry										PageCacheEntryType;
    typedef typename PageCacheType::Entry*										PageCacheEntryPtrType;

    class MyShared: public Base::Shared {

    	typedef PageCacheEntryType* PageCacheEntryPtr;
    	typedef MyShared*			MySharedPtr;

    	PageCacheEntryPtr entry_;

    	MySharedPtr next_;
    	MySharedPtr prev_;

    public:
    	MyShared() {
    		prev_ = next_ = nullptr;

    		init();
    	}

    	PageCacheEntryPtr& entry() {return entry_;}
    	const PageCacheEntryPtr& entry() const {return entry_;}

    	MySharedPtr& next() {return next_;}
    	const MySharedPtr& next() const {return next_;}

    	MySharedPtr& prev() {return prev_;}
    	const MySharedPtr& prev() const {return prev_;}

    	void init()
    	{
    		Base::Shared::init();
    		entry_	= nullptr;
    	}
    };

    typedef IntrusiveList<MyShared>												SharedPool;

    typedef std::unordered_map<ID, PageCacheEntryType*, IDKeyHash, IDKeyEq>     IDPageOpMap;
    typedef std::unordered_map<BigInt, CtrShared*>                              CtrSharedMap;

    typedef std::set<UBigInt>                              						PositionSet;
    typedef IDHashMap<ID, BigInt>                              					LocalIDMap;

    typedef std::unordered_set<ID, IDKeyHash, IDKeyEq>     						IDSet;

    typedef std::unique_ptr<SuperblockCtrType>									SuperblockCtrPtr;
    typedef std::unique_ptr<BlockMapType>										BlockMapPtr;
    typedef std::unique_ptr<IDMapType>											IDMapPtr;
    typedef std::unique_ptr<RootMapType>										RootMapPtr;

    typedef std::unique_ptr<char, void (*)(void*)>								CharBufferPtr;
    typedef std::unique_ptr<Page, void (*)(void*)>								PageBufferPtr;
    typedef std::unique_ptr<SymbolsBuffer<1>>									BlockMapBufferPtr;

    String				file_name_;

    RAFile				file_;


    PageCacheType       pages_;
    IDPageOpMap         pages_log_;
    IDSet				deleted_log_;

    CtrSharedMap        ctr_shared_;

    Logger              logger_;

    ContainerMetadataRepository* metadata_;


    const char*         type_name_ 						= "FileAllocator";

    SuperblockCtrPtr	superblock_;
    BlockMapPtr			block_map_;

    RootMapPtr        	root_map_;

    bool				id_map_entered_ = false;
    LocalIDMap			local_idmap_;

    CharBufferPtr		page_buffer_;
    BlockMapBufferPtr	blockmap_buffer_;

    // memory pool for PageShared objects
    SharedPool			shared_pool_;

    class Properties: public IAllocatorProperties {
    	const SuperblockCtrPtr& superblock_;

    public:
    	Properties(const SuperblockCtrPtr& superblock): superblock_(superblock) {}

    	virtual Int defaultPageSize() const
    	{
    		return superblock_->block_size();
    	}

    	virtual BigInt lastCommitId() const {
    		return superblock_->last_commit_id();
    	}

    	virtual void setLastCommitId(BigInt txn_id)
    	{
    		superblock_->setLastCommitId(txn_id);
    	}

    	virtual BigInt newTxnId()
    	{
    		return superblock_->new_ctr_name();
    	}

    	virtual bool isMVCC() const	{
    		return superblock_->is_mvcc();
    	}

    	virtual void setMVCC(bool mvcc)
    	{
    		return superblock_->setMVCC(mvcc);
    	}
    };

    Properties properties_;

    OpenMode mode_;
    bool is_new_;

    typename SharedPool::size_type shared_pool_max_size_ = 256;

    BigInt created_ 		= 0;
    BigInt updated_ 		= 0;
    BigInt deleted_ 		= 0;
    BigInt read_locked_ 	= 0;
    BigInt cache_hits_		= 0;
    BigInt cache_misses_	= 0;
    BigInt stored_			= 0;

    BigInt shared_created_	= 0;
    BigInt shared_deleted_	= 0;

    public:

    class Cfg {
    	Int 	default_block_size_ 			= 4096;
    	UBigInt initial_allocation_size_		= 8;
    	UBigInt allocation_size_				= 64;
    	BigInt	pages_buffer_size_ 				= 1024;
    	bool	sync_on_commit_					= true;

    	Int block_size_mask_ 					= 12;

    public:

    	Int block_size() const {
    		return default_block_size_;
    	}

    	Int block_size_mask() const {
    		return block_size_mask_;
    	}

    	Cfg& block_size(Int size)
    	{
    		default_block_size_ = size;

    		block_size_mask_ = Log2(size);

    		return *this;
    	}

    	UBigInt allocation_size() const {
    		return allocation_size_;
    	}

    	Cfg& allocation_size(UBigInt size)
    	{
    		allocation_size_ = size;
    		return *this;
    	}

    	UBigInt initial_allocation_size() const {
    		return initial_allocation_size_;
    	}

    	Cfg& initial_allocation_size(UBigInt size)
    	{
    		initial_allocation_size_ = size;
    		return *this;
    	}

    	BigInt pages_buffer_size() const {
    		return pages_buffer_size_;
    	}

    	Cfg& pages_buffer_size(BigInt size)
    	{
    		pages_buffer_size_ = size;
    		return *this;
    	}

    	bool sync_on_commit() const {
    		return sync_on_commit_;
    	}

    	void sync_on_commit(bool sync)
    	{
    		sync_on_commit_ = sync;
    	}
    };

private:

    Cfg cfg_;

public:
    FileAllocator(
    		StringRef& file_name,
    		OpenMode mode = OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE,
    		const Cfg& cfg = Cfg()
    ):
    	file_name_(file_name),
    	pages_(cfg.pages_buffer_size()),
    	logger_("memoria::FileAllocator", Logger::DERIVED, &memoria::vapi::logger),
        metadata_(MetadataRepository<Profile>::getMetadata()),
        page_buffer_(nullptr, free),
        properties_(superblock_),
        mode_(mode),
        cfg_(cfg)
    {
    	initMetadata();

    	mode = mode | OpenMode::READ;

    	if (to_bool(mode & OpenMode::TRUNC) && !to_bool(mode & OpenMode::WRITE))
    	{
    		throw vapi::Exception(MA_SRC, "TRUNC is only valid for WRITE-enabled databases");
    	}

    	File file(file_name);

    	if (file.isExists())
    	{
    		if (!file.isDirectory())
    		{
    			openFile(mode);

    			is_new_ = to_bool(mode & OpenMode::TRUNC);

    			if (!is_new_)
    			{
    				superblock_ = SuperblockCtrPtr(new SuperblockCtrType(file_));
    			}
    			else {
    				superblock_ = SuperblockCtrPtr(new SuperblockCtrType(file_, cfg.block_size()));
    			}
    		}
    		else {
    			throw vapi::FileException(MA_SRC, "Requested file is a directory");
    		}
    	}
    	else {
    		openFile(mode);

    		superblock_ = SuperblockCtrPtr(new SuperblockCtrType(file_, cfg.block_size()));
    		is_new_		= true;
    	}

    	page_buffer_ 		= CharBufferPtr(T2T<char*>(malloc(cfg.block_size())), free);
    	blockmap_buffer_	= BlockMapBufferPtr(new SymbolsBuffer<1>(cfg.allocation_size()));

    	blockmap_buffer_->clear();

    	for (Int c = 0; c < shared_pool_max_size_; c++)
    	{
    		shared_pool_.insert(shared_pool_.begin(), new MyShared());
    	}

    	if (is_new_)
    	{
    		allocateFileSpace(cfg_.initial_allocation_size());
    		// make superblock's space allocated
    		this->allocateEmptyBlock();

    		block_map_ 	= BlockMapPtr(new BlockMapType(this, CTR_CREATE, 0));

    		commitTemporaryToBlockMap(cfg_.initial_allocation_size());
    		finishFileSpaceAllocation(cfg_.initial_allocation_size());

    		root_map_ 	= RootMapPtr(new RootMapType(this, CTR_CREATE, 2));

    		superblock_->storeSuperblock();

    		commit();
    	}
    	else {
    		block_map_ 	= BlockMapPtr(new BlockMapType(this, CTR_FIND, 0));
    		root_map_ 	= RootMapPtr(new RootMapType(this, CTR_FIND, 2));
    	}
    }

    FileAllocator(const MyType&) 		= delete;
    FileAllocator(MyType&&) 			= delete;

    MyType& operator=(const MyType&) 	= delete;
    MyType& operator=(MyType&&) 		= delete;

    virtual ~FileAllocator()
    {
    	PageCacheEntryType* head = pages_.list().begin().node();

    	while(head)
    	{
    		auto tmp = head->next();
    		freeEntry(head);
    		head = tmp;
    	}

    	MyShared* shared_head = shared_pool_.begin().node();

    	while(shared_head)
    	{
    		auto tmp = shared_head->next();

    		delete shared_head;

    		shared_head = tmp;
    	}

    	try {
    		this->close(false);
    	}
    	catch (...) {
    		//log exception here
    	}
    }

    static void initMetadata()
    {
    	BlockMapType::initMetadata();
    	RootMapType::initMetadata();
    }

    const Cfg& cfg() const {
    	return cfg_;
    }

    bool is_new() const {
    	return is_new_;
    }

    bool is_read_only() const {
    	return !to_bool(mode_ & OpenMode::WRITE);
    }

    bool is_clean() const
    {
    	return pages_log_.size() == 0 && !superblock_->is_updated();
    }

    UBigInt allocation_size() const {
    	return cfg_.allocation_size();
    }

    typename SharedPool::size_type shared_pool_size() const {
    	return shared_pool_.size();
    }

    typename SharedPool::size_type shared_pool_max_size() const {
    	return shared_pool_max_size_;
    }

    typename SharedPool::size_type shared_pool_capacity() const {
    	return shared_pool_max_size_ - shared_pool_.size();
    }

    BigInt free_cache_slots() const
    {
    	return static_cast<BigInt>(cfg_.pages_buffer_size()) - (pages_log_.size() + read_locked_);
    }

    BigInt cache_eviction_capacity() const {
    	return free_cache_slots() - created_;
    }

    BigInt created() const {
    	return created_;
    }

    BigInt updated() const {
    	return updated_;
    }

    BigInt deleted() const {
    	return deleted_;
    }

    BigInt locked() const {
    	return shared_pool_max_size() - shared_pool_size();
    }

    BigInt read_locked() const {
    	return read_locked_;
    }

    BigInt shared_created() const {return shared_created_;}
    BigInt shared_deleted() const {return shared_deleted_;}

    RootMapType* roots() {
    	return root_map_.get();
    }

    const RootMapType* roots() const {
    	return root_map_.get();
    }

    StringRef file_name() const {
    	return file_name_;
    }

    UBigInt file_size()
    {
    	return file_.seek(0, SeekType::END);
    }

    const SuperblockCtrType* superblock() const {
    	return superblock_.get();
    }

    void sync() {
    	file_.sync();
    }

    // IAllocator

    virtual PageG getPage(const ID& id, BigInt name)
    {
    	MEMORIA_ASSERT_TRUE(id);

    	PageCacheEntryType* entry = get_from_log(id);

    	if (entry)
    	{
    		pages_.touch(entry);

    		cache_hits_++;

    		createSharedIfNecessary(entry, Shared::UPDATE)->refresh(); // it is not necessary to refresh here
    	}
    	else {
    		entry = get_entry(id);

    		createSharedIfNecessary(entry, Shared::READ);
    	}

    	MEMORIA_ASSERT_TRUE(entry->shared());

    	return PageG(entry->shared());
    }

    virtual PageG getPageForUpdate(const ID& id, BigInt name)
    {
    	MEMORIA_ASSERT_TRUE(id);

    	PageCacheEntryType* entry = get_from_log(id);

    	if (entry)
    	{
    		pages_.touch(entry);

    		cache_hits_++;
    	}
    	else {
    		entry = get_entry(id);

    		MEMORIA_ASSERT_FALSE(entry->front_page());

    		entry->front_page() = copyPage(entry->page());

    		entry->page_op() 	= PageOp::UPDATE;

    		pages_log_[id] = entry;
    	}

    	createSharedIfNecessary(entry, Shared::UPDATE)->refresh();

    	MEMORIA_ASSERT_TRUE(entry->shared());

    	return PageG(entry->shared());
    }

    virtual PageG getPageG(Page* page)
    {
    	return getPage(page->gid(), -1);
    }

    virtual PageG updatePage(Shared* shared, BigInt name)
    {
    	MyShared* my_shared = static_cast<MyShared*>(shared);

    	if (my_shared->state() == Shared::READ)
    	{
    		read_locked_--;

    		MEMORIA_ASSERT_TRUE(read_locked_ >= 0);
    		MEMORIA_ASSERT_FALSE(get_from_log(shared->id()))

    		PageCacheEntryType* entry = my_shared->entry();

    		MEMORIA_ASSERT_TRUE(entry->page());
    		MEMORIA_ASSERT_FALSE(entry->front_page());

    		entry->front_page() = copyPage(entry->page());

    		entry->page_op() 	= PageOp::UPDATE;

    		pages_log_[entry->key()] = entry;

    		shared->set_page(entry->front_page());

    		shared->state() = Shared::UPDATE;

    		shared->refresh();

    		updated_++;
    	}

    	return PageG(shared);
    }

    virtual void removePage(const ID& id, BigInt name)
    {
    	deleted_log_.insert(id);

    	deleted_++;

    	if (pages_.contains_key(id))
    	{
    		PageCacheEntryType* entry = get_entry(id);
    		entry->page_op() = PageOp::DELETE;

    		if (entry->shared())
    		{
    			if (entry->shared()->state() == Shared::READ)
    			{
    				read_locked_--;
    				MEMORIA_ASSERT_TRUE(read_locked_ >= 0);
    			}

    			// FIXME it isn't really necessary to inform PageGuards that the page is marked deleted
    			entry->shared()->state() = Shared::DELETE;
    			entry->shared()->refresh();
    		}

    		pages_log_[id] = entry;
    	}
    	else {
    		 PageCacheEntryType* entry = get_from_log(id);
    		 if (entry)
    		 {
    			 MEMORIA_ASSERT_TRUE(entry->page_op() != PageOp::NONE)

    			 entry->page_op() = PageOp::DELETE;

    			 if (entry->shared())
    			 {
    				 MEMORIA_ASSERT_TRUE(entry->shared()->state() != Shared::READ);

    				 // FIXME it isn't really necessary to inform PageGuards that the page is marked deleted
    				 entry->shared()->state() = Shared::DELETE;
    				 entry->shared()->refresh();
    			 }
    		 }
    	}
    }

    virtual PageG createPage(Int initial_size, BigInt name)
    {
    	MEMORIA_ASSERT(initial_size, ==, cfg_.block_size());

    	if (free_cache_slots() == 0) {
    		throw Exception(MA_SRC, "Update buffer is full");
    	}

    	UBigInt pos = this->allocateEmptyBlock();
    	ID gid		= this->createGID(pos);

    	PageCacheEntryType* entry = new PageCacheEntryType(gid, pos);

    	entry->counter() = 1;
    	entry->page_op() = PageOp::UPDATE;

    	entry->front_page() = createPage(gid);

    	pages_log_[gid]  = entry;

    	createShared(entry, Shared::UPDATE);

    	created_++;

    	return PageG(entry->shared());
    }

    virtual void resizePage(Shared* page, Int new_size)
    {
    	throw Exception(MA_SRC, "Page resizing is not yet supported");
    }

    virtual void releasePage(Shared* shared) noexcept
    {
    	MyShared* my_shared = static_cast<MyShared*>(shared);

    	if (my_shared->state() == Shared::READ)
    	{
    		read_locked_--;
    		MEMORIA_ASSERT_TRUE(read_locked_ >= 0);
    	}

    	PageCacheEntryType* entry = my_shared->entry();

    	MEMORIA_ASSERT_TRUE(entry->shared() == my_shared);

    	freePageShared(entry->shared());

    	if ((!entry->is_allocated()) && (!get_from_log(entry->key())))
    	{
    		freeEntry(entry);
    	}
    }

    virtual CtrShared* getCtrShared(BigInt name)
    {
    	auto i = ctr_shared_.find(name);

    	if (i != ctr_shared_.end())
    	{
    		return i->second;
    	}
    	else
    	{
    		throw Exception(MEMORIA_SOURCE, SBuf()<<"Unknown CtrShared requested for name "<<name);
    	}
    }

    virtual bool isCtrSharedRegistered(BigInt name)
    {
    	return ctr_shared_.find(name) != ctr_shared_.end();
    }

    virtual void unregisterCtrShared(CtrShared* shared)
    {
    	ctr_shared_.erase(shared->name());
    }

    virtual void registerCtrShared(CtrShared* shared)
    {
    	BigInt name = shared->name();

    	auto i = ctr_shared_.find(name);

    	if (i == ctr_shared_.end())
    	{
    		ctr_shared_[name] = shared;
    	}
    	else if (i->second == NULL)
    	{
    		i->second = shared;
    	}
    	else
    	{
    		throw Exception(MEMORIA_SOURCE, SBuf()<<"CtrShared for name "<<name<<" is already registered");
    	}
    }



    // memory pool allocator

    virtual void* allocateMemory(std::size_t size)
    {
    	return malloc(size);
    }

    virtual void freeMemory(void* ptr)
    {
    	free(ptr);
    }


    virtual Logger& logger() {
        return logger_;
    }

    virtual Logger* getLogger() {
        return &logger_;
    }


    // ICtrDirectory

    virtual ID getRootID(BigInt name)
    {
    	if (name == 0)
    	{
    		return superblock_->blockmap_root_id();
    	}
    	else if (name == 1)
    	{
    		return superblock_->idmap_root_id();
    	}
    	else if (name == 2)
    	{
    		return superblock_->rootmap_id();
    	}
    	else
    	{
    		auto iter = root_map_->find(name);

    		if (!iter.isEnd())
    		{
    			return iter.getValue();
    		}
    		else {
    			return ID(0);
    		}
    	}
    }

    virtual void setRoot(BigInt name, const ID& root)
    {
    	if (name == 0)
    	{
    		superblock_->set_blockmap_root_id(root);
    	}
    	else if (name == 1)
    	{
    		superblock_->set_idmap_root_id(root);
    	}
    	else if (name == 2)
    	{
    		superblock_->set_rootmap_id(root);
    	}
    	else if (root.isSet())
    	{
    		root_map_->operator[](name).setData(root);
    	}
    	else {
    		root_map_->remove(name);
    	}
    }

    virtual bool hasRoot(BigInt name)
    {
    	if (name == 0)
    	{
    		return superblock_->blockmap_root_id().isSet();
    	}
    	else if (name == 1)
    	{
    		return superblock_->idmap_root_id().isSet();
    	}
    	else if (name == 2)
    	{
    		return superblock_->rootmap_id().isSet();
    	}
    	else
    	{
    		auto iter = root_map_->find(name);
    		return (!iter.isEnd()) && iter.key() == name;
    	}
    }

    virtual void markUpdated(BigInt name) {}

    virtual BigInt currentTxnId() const	{
    	return 0;
    }

    virtual BigInt createCtrName()
    {
    	return superblock_->new_ctr_name();
    }

    virtual IAllocatorProperties& properties()
    {
    	return properties_;
    }

    virtual ID newId() {
    	return superblock_->new_id();
    }


    //public stuff

    ContainerMetadataRepository* getMetadata() const
    {
    	return metadata_;
    }

    void close(bool commit = true)
    {
    	if (commit)
    	{
    		this->commit();
    	}

    	file_.close();
    }

    void commit(bool force_sync = false)
    {
    	flush(force_sync);
    }

    virtual void flush(bool force_sync = false)
    {
    	// ensure file has enough room for backup

    	if (is_read_only())
    	{
    		if (!is_clean())
    		{
    			rollback(force_sync);

    			throw vapi::Exception(MA_SRC, "Can't modify read-only allocator");
    		}
    	}

    	for (auto gid: deleted_log_)
    	{
    		freeEmptyBlock(gid);
    	}

    	UBigInt updated = computeUpdatedPages();

    	while (superblock_->free_blocks() < updated)
    	{
    		ensureCapacity(updated);
    		updated = computeUpdatedPages();
    	}

#ifdef MEMORIA_TESTS
    	if (failure() == FAILURE_BEFORE_BACKUP) {
    		throw Exception(MA_SRC, "Artificial failure before backup of updated blocks");
    	}
#endif

    	// backup updated pages

    	UBigInt backup_list_head = 0;
    	UBigInt backup_list_size = 0;

    	auto blockmap_iter = block_map_->begin();

    	for (auto i: pages_log_)
    	{
    		PageCacheEntryType* entry = i.second;
    		if (isUpdatedEntry(entry))
    		{
    			blockmap_iter.selectFw(1, 0);

    			MEMORIA_ASSERT_FALSE(blockmap_iter.isEnd());

    			Page* page = entry->page();

    			page->target_block_pos() 	= entry->position();
    			page->next_block_pos() 		= backup_list_head;

    			UBigInt new_pos = blockmap_iter.pos() * cfg_.block_size();

    			storePage(new_pos, page);

    			backup_list_head = new_pos;

    			backup_list_size++;

    			blockmap_iter++;
    		}
    	}

    	superblock_->set_backup_list(backup_list_head, backup_list_size);

    	superblock_->storeBackup();
    	syncIfConfigured(force_sync);

    	// commit updates

    	for (auto i: pages_log_)
    	{
    		PageCacheEntryType* entry = i.second;

    		// commit page update
    		if (entry->page_op() == PageOp::UPDATE)
    		{
    			MEMORIA_ASSERT_TRUE(entry->front_page());

    			if (entry->shared())
    			{
    				entry->shared()->state() = Shared::READ;
    				entry->shared()->set_page(entry->front_page());

    				entry->shared()->refresh();

    				read_locked_++;
    			}

    			if (!entry->is_allocated())
    			{
    				MEMORIA_ASSERT_FALSE(entry->page());
    				pages_.insert(entry);
    			}
    			else {
    				MEMORIA_ASSERT_TRUE(entry->page());
    			}

    			if (entry->page())
    			{
    				MEMORIA_ASSERT(entry->page()->gid(), ==, entry->front_page()->gid());
    			}

    			std::swap(entry->page(), entry->front_page());

    			storePage(entry->position(), entry->page());

    			entry->page_op() = PageOp::NONE;

    			if (entry->front_page())
    			{
    				freePagePtr(entry->front_page());
    			}
    		}
    		else // commit page deletion
    		{
    			pages_.remove_entry(entry);

    			if (!entry->shared())
    			{
    				freeEntry(entry);
    			}
    		}
    	}

    	for (auto i = ctr_shared_.begin(); i != ctr_shared_.end(); i++)
    	{
    		i->second->commit();
    	}

    	pages_log_.clear();
    	deleted_log_.clear();

    	created_ 	 = 0;
    	updated_ 	 = 0;
    	deleted_ 	 = 0;

    	cache_misses_ 	= 0;
    	cache_hits_ 	= 0;
    	stored_			= 0;

    	// It looks like it is not necessary to sync here, because at this point
    	// all sensitive information has been already backuped
    	//syncIfConfigured(force_sync);

#ifdef MEMORIA_TESTS
    	if (failure() == FAILURE_BEFORE_COMMIT) {
    		throw Exception(MA_SRC, "Artificial failure before commit of superblock");
    	}
#endif

    	superblock_->storeSuperblock();

    	syncIfConfigured(force_sync);

    	MEMORIA_ASSERT(block_map_->size() * cfg_.block_size(), ==, superblock_->file_size());
    	MEMORIA_ASSERT(file_size(), ==, superblock_->file_size());
    }

    void dumpStat()
    {
    	cout<<"PageLog: "<<pages_log_.size()<<" "<<deleted_log_.size()<<" "
    	    <<cache_hits_<<" "<<cache_misses_<<" "<<stored_<<" "<<created_<<" "<<updated_
    	    <<" "<<deleted_<<endl;
    }

    void dumpPage(const PageG& page)
    {
    	auto page_metadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

    	vapi::dumpPageData(page_metadata, page.page(), std::cout);
    }

    void dumpPage(const Page* page)
    {
    	auto page_metadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

    	vapi::dumpPageData(page_metadata, page, std::cout);
    }

    void dumpAllocatedPages(StringRef path)
    {
    	typedef FSDumpContainerWalker<Page> Walker;
    	Walker walker(metadata_, path);

    	walker.beginAllocator("FileAllocator", nullptr);
    	walker.beginSection("AllocatedPages");

    	auto iter = this->block_map_->seek(1);

    	iter.selectFw(1, 1);

    	while (!iter.isEnd())
    	{
    		BigInt pos 	= iter.pos();
    		ID gid 		= pos * superblock_->block_size();

    		MEMORIA_ASSERT_TRUE(iter.symbol());

    		PageG page 	= this->getPage(gid, -1);

    		std::stringstream str;

    		char prev = str.fill();

    		str.fill('0');
    		str.width(4);

    		str<<pos;

    		str.fill(prev);

    		str<<"___"<<gid;

    		walker.singleNode(str.str().c_str(), page.page());

    		iter++;
    		iter.selectFw(1, 1);
    	}

    	walker.endSection();
    	walker.endAllocator();
    }

    void clearCache()
    {
    	vector<PageCacheEntryType*> removed_entries;

    	PageCacheEntryType* entry = pages_.list().begin().node();

    	BigInt total = 0, removed = 0;

    	while(entry)
    	{
    		if (entry->page_op() == PageOp::NONE && !entry->shared())
    		{
    			MEMORIA_ASSERT_FALSE(entry->front_page());

    			removed_entries.push_back(entry);

    			removed++;
    		}

    		total++;

    		entry = entry->next();
    	}

    	for (auto e: removed_entries)
    	{
    		pages_.remove_entry(e);

    		freeEntry(e);
    	}
    }

    virtual void rollback(bool force_sync = false)
    {
    	for (auto i = pages_log_.begin(); i != pages_log_.end(); i++)
    	{
    		PageCacheEntryType* entry = i->second;

    		MEMORIA_ASSERT_TRUE(entry->front_page());

    		freePagePtr(entry->front_page());

    		if (!entry->is_allocated())
    		{
    			MEMORIA_ASSERT_FALSE(entry->page());
    		}
    		else {
    			MEMORIA_ASSERT_TRUE(entry->page());

    			entry->front_page() = nullptr;
    			entry->page_op() = PageOp::NONE;

    			if (entry->shared())
    			{
    				entry->shared()->state() = Shared::READ;
    				entry->shared()->set_page(entry->page());

    				entry->shared()->refresh();

    				read_locked_++;
    			}
    		}
    	}

    	for (auto i = ctr_shared_.begin(); i != ctr_shared_.end(); i++)
    	{
    		i->second->rollback();
    	}

    	superblock_->rollback();

    	pages_log_.clear();
    	deleted_log_.clear();

    	created_ 	 = 0;
    	updated_ 	 = 0;
    	deleted_ 	 = 0;

    	cache_misses_ 	= 0;
    	cache_hits_ 	= 0;

    	stored_		= 0;

    	// check if file size matches blockmap size
    	MEMORIA_ASSERT(block_map_->size() * cfg_.block_size(), ==, superblock_->file_size());

    	// return the file space allocated in transaction
    	checkAndFixFileSize(file_, superblock_);
    }

    virtual bool check()
    {
    	bool result = block_map_->checkTree();
    	result = root_map_->checkTree() || result;

    	for (auto iter = this->root_map_->Begin(); !iter.isEnd(); )
    	{
    		BigInt ctr_name = iter.key();

    		PageG page = this->getPage(iter.getValue(), ctr_name);

    		ContainerMetadata* ctr_meta = metadata_->getContainerMetadata(page->ctr_type_hash());

    		result = ctr_meta->getCtrInterface()->check(&page->id(), ctr_name, this) || result;

    		iter.next();
    	}

    	return result;
    }

    virtual void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
    	walker->beginAllocator("FileAllocator", allocator_descr);

    	dumpDirectories(walker);

    	auto iter = root_map_->Begin();

    	while (!iter.isEnd())
    	{
    		BigInt ctr_name = iter.key();
    		ID root_id		= iter.value();

    		PageG page 		= this->getPage(root_id, ctr_name);

    		Int master_hash = page->master_ctr_type_hash();
    		Int ctr_hash 	= page->ctr_type_hash();

    		ContainerMetadata* ctr_meta = metadata_->getContainerMetadata(master_hash != 0 ? master_hash : ctr_hash);

    		ctr_meta->getCtrInterface()->walk(&page->gid(), ctr_name, this, walker);

    		iter++;
    	}

    	walker->endAllocator();
    }


    static Int testFile(StringRef file_name)
    {
    	return SuperblockCtrType::testHeader(file_name);
    }

    enum class RecoveryStatus: Int {
    	CLEAN, FILE_SIZE, LOG
    };

    static RecoveryStatus recover(StringRef file_name)
    {
    	RAFile file;
    	file.open(file_name.c_str(), OpenMode::RW);

    	SuperblockCtrPtr superblock(new SuperblockCtrType(file, false));

    	if (superblock->is_dirty())
    	{
    		UBigInt list_size_cnt = 0;

    		UBigInt list_head = superblock->backup_list_start();
    		UBigInt list_size = superblock->backup_list_size();

    		Int block_size = superblock->block_size();

    		PageBufferPtr page(T2T<Page*>(malloc(block_size)), free);

    		// recover updated blocks from backup blocks
    		while (list_head)
    		{
    			file.seek(list_head, SeekType::SET);
    			file.readAll(page.get(), block_size);

    			UBigInt target = page->target_block_pos();
    			file.seek(target, SeekType::SET);

    			file.write(page.get(), block_size);

    			list_head = page->next_block_pos();

    			list_size_cnt++;
    		}

    		MEMORIA_ASSERT(list_size_cnt, == ,list_size);

    		// recover superblock
    		superblock->rollback();

    		superblock->storeSuperblock();
    		superblock->storeBackup();

    		file.sync();

    		// return allocated file space
    		checkAndFixFileSize(file, superblock);

    		return RecoveryStatus::LOG;
    	}
    	else
    	{
    		return checkAndFixFileSize(file, superblock);
    	}
    }

private:

    static RecoveryStatus checkAndFixFileSize(IRandomAccessFile& file, const SuperblockCtrPtr& superblock)
    {
    	BigInt size = file.seek(0, SeekType::END);

    	if (size > superblock->file_size())
    	{
    		file.truncate(superblock->file_size());

    		return RecoveryStatus::FILE_SIZE;
    	}
    	else {
    		return RecoveryStatus::CLEAN;
    	}
    }

    void openFile(OpenMode mode)
    {
    	file_.open(file_name_.c_str(), mode);
    }

    PageCacheEntryType* get_from_log(const ID &page_id) const
    {
    	auto i = pages_log_.find(page_id);
    	if (i != pages_log_.end())
    	{
    		return i->second;
    	}
    	else {
    		return nullptr;
    	}
    }

    PageCacheEntryType* get_entry(const ID &page_id)
    {
    	cache_hits_++;

    	return pages_.get_entry(page_id, [this](PageCacheEntryType& entry) {
    		UBigInt pos = this->get_position(entry.key());

    		if (!entry.page())
    		{
    			entry.page() = createPage(ID(0));
    		}

    		cache_hits_--;
    		cache_misses_++;

    		entry.position() = pos;

    		this->loadPage(pos, entry.page());

    		MEMORIA_WARNING(entry.key(), !=, entry.page()->gid());
    	});
    }


    MyShared* createSharedIfNecessary(PageCacheEntryType* entry, Int op)
    {
    	if (!entry->shared())
    	{
    		return createShared(entry, op);
    	}
    	else if (op == Shared::UPDATE && entry->shared()->state() == Shared::READ)
    	{
    		return updateShared(entry);
    	}

    	return entry->shared();
    }

    MyShared* createShared(PageCacheEntryType* entry, Int op)
    {
    	MyShared* shared = allocatePageShared();

    	shared->id()    = entry->key();
    	shared->state()	= op;

    	shared->set_page(entry->current_page());
    	shared->set_allocator(this);

    	entry->shared() = shared;
    	shared->entry() = entry;

    	if (op == Shared::READ)
    	{
    		read_locked_++;
    	}

    	shared_created_++;

    	return shared;
    }

    MyShared* updateShared(PageCacheEntryType* entry)
    {
    	MyShared* shared = entry->shared();

    	shared->set_page(entry->current_page());
    	shared->state() = Shared::UPDATE;

    	shared->refresh();

    	read_locked_--;
    	shared_created_++;

    	return shared;
    }

    Page* copyPage(Page* src)
    {
    	Int page_size = src->page_size();
    	char* buffer = (char*) malloc(page_size);

    	CopyByteBuffer(src, buffer, page_size);
    	Page* new_page = T2T<Page*>(buffer);

    	return new_page;
    }

    Page* createPage(const ID& id)
    {
    	void* buf = malloc(cfg_.block_size());
    	memset(buf, 0, cfg_.block_size());

    	Page* p = new (buf) Page(id);

    	p->page_size() = cfg_.block_size();

    	return p;
    }


    void freeEntry(PageCacheEntryType* entry)
    {
    	freePagePtr(entry->page());
    	freePagePtr(entry->front_page());

    	if (entry->shared())
    	{
    		entry->shared()->entry() = nullptr;
    		entry->shared()->resetPage();
    	}

    	delete entry;
    }

    // Operations on embedded containers
    UBigInt allocateEmptyBlock()
    {
    	if (superblock_->use_temporary_allocator())
    	{
    		return superblock_->allocateTemporaryBlock();
    	}
    	else
    	{
    		if (superblock_->free_blocks() == 0)
    		{
    			enlargeFile(cfg_.allocation_size());
    		}

    		auto iter = block_map_->select(0, 1);

    		MEMORIA_ASSERT_TRUE(iter.symbol() == 0);
    		MEMORIA_ASSERT_FALSE(iter.isEnd());

    		iter.setSymbol(1);
    		superblock_->decFreeBlocks();

    		MEMORIA_ASSERT(superblock_->free_blocks(), ==, block_map_->rank(0));

    		return iter.pos() * cfg_.block_size();
    	}
    }

    void freeEmptyBlock(const ID& gid)
    {
    	UBigInt pos = get_position(gid);

    	UBigInt block = pos >> cfg_.block_size_mask();

    	auto iter = block_map_->seek(block);

    	MEMORIA_ASSERT_FALSE(iter.isEnd());

    	BigInt old_rank = block_map_->rank(0);

    	Int sym = iter.symbol();

    	if (sym)
    	{
    		iter.setSymbol(0);
    		superblock_->incFreeBlocks(1);
    	}

    	BigInt new_rank = block_map_->rank(0);

    	if (sym && (old_rank != new_rank - 1))
    	{
    		cout<<"Free blocks problem: "<<old_rank<<" "<<new_rank<<endl;
    		block_map_->seek(0).dumpPath();
    	}


    	MEMORIA_WARNING(superblock_->free_blocks(), !=, block_map_->rank(0));
    }

    ID createGID(UBigInt position)
    {
    	return position;
    }

    UBigInt get_position(const ID& id)
    {
    	return id.value();
    }

    void loadPage(UBigInt pos, Page* page)
    {
    	loadPage(file_, cfg_.block_size(), pos, page, page_buffer_, metadata_);
    }

    static void loadPage(
    		IRandomAccessFile& file,
    		Int block_size,
    		UBigInt pos,
    		Page* page,
    		CharBufferPtr& page_buffer,
    		ContainerMetadataRepository* metadata
    )
    {
    	file.seek(pos, SeekType::SET);
    	file.read(page_buffer.get(), block_size);

    	Page* disk_page = T2T<Page*>(page_buffer.get());

    	Int page_data_size 	= disk_page->page_size();
    	Int ctr_type_hash	= disk_page->ctr_type_hash();
    	Int page_type_hash	= disk_page->page_type_hash();

    	MEMORIA_ASSERT(page_data_size, ==, block_size);

    	PageMetadata* page_metadata 		= metadata->getPageMetadata(ctr_type_hash, page_type_hash);
    	const IPageOperations* operations 	= page_metadata->getPageOperations();

    	operations->deserialize(page_buffer.get(), page_data_size, T2T<void*>(page));
    }


    void storePage(UBigInt pos, Page* page)
    {
    	stored_++;

    	storePage(file_, cfg_.block_size(), pos, page, page_buffer_, metadata_);
    }



    static void storePage(
    		IRandomAccessFile& file,
    		Int block_size,
    		UBigInt pos,
    		Page* page,
    		CharBufferPtr& page_buffer,
    		ContainerMetadataRepository* metadata
    )
    {
    	MEMORIA_ASSERT(page->page_size(), ==, block_size);

    	file.seek(pos, SeekType::SET);

    	Int ctr_type_hash	= page->ctr_type_hash();
    	Int page_type_hash	= page->page_type_hash();

    	PageMetadata* page_metadata 		= metadata->getPageMetadata(ctr_type_hash, page_type_hash);
    	const IPageOperations* operations 	= page_metadata->getPageOperations();

    	memset(page_buffer.get(), 0, block_size);

    	operations->serialize(page, page_buffer.get());

    	file.write(page_buffer.get(), block_size);
    }

    void clearPageBuffer()
    {
    	memset(page_buffer_.get(), 0, cfg_.block_size());
    }

    void allocateFileSpace(UBigInt allocation_size)
    {
    	clearPageBuffer();

    	UBigInt file_size = superblock_->file_size();

    	file_.seek(0, SeekType::END);

    	for (UBigInt c = 0; c < allocation_size; c++)
    	{
    		file_.write(page_buffer_.get(), cfg_.block_size());
    	}

    	auto pos = file_.seek(0, SeekType::CUR);

    	MEMORIA_ASSERT(pos, ==, file_size + allocation_size * cfg_.block_size());

    	superblock_->enlargeFile(allocation_size * cfg_.block_size());
    	superblock_->setTemporaryBlockMap(file_size, allocation_size);
    }

    void commitTemporaryToBlockMap(UBigInt allocation_size)
    {
    	auto iter = block_map_->seek(block_map_->size());

    	auto src_buffer = blockmap_buffer_.get();

    	DataSourceProxy<UBigInt> proxy(*src_buffer, allocation_size);

    	iter.insert(proxy);
    	blockmap_buffer_->reset();
    }

    void finishFileSpaceAllocation(UBigInt allocation_size)
    {
    	auto iter = block_map_->seek(superblock_->total_blocks());

    	auto buffer = superblock_->temporary_blockmap_buffer();

    	UBigInt allocated_before = superblock_->temporary_allocated_blocks();

    	DataSourceProxy<UBigInt> proxy(buffer, allocation_size);

    	// this operation must not allocate new blocks
    	iter.update(buffer);

    	UBigInt allocated_after = superblock_->temporary_allocated_blocks();

    	MEMORIA_ASSERT(allocated_before, ==, allocated_after);

    	superblock_->updateMainBlockMapMetadata();
    	superblock_->clearTemporaryBlockMap();
    }

    void enlargeFile(UBigInt allocation_size)
    {
    	allocateFileSpace(allocation_size);
    	commitTemporaryToBlockMap(allocation_size);
    	finishFileSpaceAllocation(allocation_size);

    	MEMORIA_ASSERT(superblock_->free_blocks(), >, 0);
    }

    void ensureCapacity(UBigInt capacity)
    {
    	while (superblock_->free_blocks() < capacity)
    	{
    		enlargeFile(cfg_.allocation_size());
    	}
    }


    UBigInt computeUpdatedPages()
    {
    	UBigInt updated = 0;

    	for (auto i: pages_log_)
    	{
    		if (isUpdatedEntry(i.second))
    		{
    			updated++;
    		}
    	}

    	return updated;
    }

    bool isUpdatedEntry(PageCacheEntryType* entry)
    {
    	// check if entry is UPDATE entry
    	return entry->page_op() == PageOp::UPDATE && entry->is_allocated();
    }

    void freePagePtr(PagePtr& ptr)
    {
    	::free(ptr);
    	ptr = nullptr;
    }

    MyShared* allocatePageShared()
    {
    	MEMORIA_ASSERT_TRUE(shared_pool_.size() > 0);

    	MyShared* shared = shared_pool_.takeTop();

    	shared->init();

    	return shared;
    }

    void freePageShared(MySharedPtr& shared)
    {
    	MEMORIA_ASSERT_FALSE(shared->next());
    	MEMORIA_ASSERT_FALSE(shared->prev());

    	MEMORIA_ASSERT(shared_pool_.size(), <, shared_pool_max_size_);

    	shared_pool_.insert(shared_pool_.begin(), shared);

    	shared_deleted_++;

    	shared = nullptr;
    }

    void syncIfConfigured(bool force_sync)
    {
    	if (cfg_.sync_on_commit() || force_sync)
    	{
    		file_.sync();
    	}
    }


    void dumpDirectories(ContainerWalker* walker)
    {
    	walker->beginSection("0_Directories");

    	root_map_->walkTree(walker);
    	block_map_->walkTree(walker);

    	walker->endSection();
    }


#ifdef MEMORIA_TESTS
public:
    enum {
    	NO_FAILURE				= 0,
    	FAILURE_BEFORE_BACKUP 	= 1,
    	FAILURE_BEFORE_COMMIT 	= 2
    };

    Int failure_ = NO_FAILURE;

    Int& failure() {return failure_;}
    const Int& failure() const {return failure_;}

#endif

};

}



#endif
