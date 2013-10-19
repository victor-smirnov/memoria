
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

using namespace std;


using namespace memoria::vapi;

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

private:
    typedef FileAllocator<Profile, PageType, TxnType>                           MyType;



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

    	typedef Page* PagePtr;

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
    	PageCacheEntryPtr entry_;
    	bool is_new_ = false;
    public:
    	PageCacheEntryPtr& entry() {return entry_;}
    	const PageCacheEntryPtr& entry() const {return entry_;}

    	bool& is_new() {return is_new_;}
    	const bool& is_new() const {return is_new_;}
    };

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

    Int 				default_block_size_ 			= 4096;
    UBigInt 			allocation_batch_size_			= 64; // in blocks

    class Properties: public IAllocatorProperties {
    	const SuperblockCtrPtr& superblock_;

    public:
    	Properties(const SuperblockCtrPtr& superblock): superblock_(superblock) {}

    	virtual Int defaultPageSize() const
    	{
    		return superblock_->block_size();
    	}
    };

    Properties properties_;

    OpenMode mode_;
    bool is_new_;

    UBigInt created_ = 0;
    UBigInt updated_ = 0;
    UBigInt deleted_ = 0;
    UBigInt locked_  = 0;

public:
    FileAllocator(StringRef& file_name, OpenMode mode = OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE):
    	file_name_(file_name),
    	pages_(1024),
    	logger_("memoria::FileAllocator", Logger::DERIVED, &memoria::vapi::logger),
        metadata_(MetadataRepository<Profile>::getMetadata()),
        page_buffer_(nullptr, free),
        properties_(superblock_),
        mode_(mode)
    {
    	BlockMapType::initMetadata();
    	RootMapType::initMetadata();

    	mode = mode | OpenMode::READ;

    	if (to_bool(mode & OpenMode::TRUNC) && !to_bool(mode & OpenMode::WRITE))
    	{
    		throw Exception(MA_SRC, "TRUNC is only valid for WRITE-enabled databases");
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
    				superblock_ = SuperblockCtrPtr(new SuperblockCtrType(file_, default_block_size_));
    			}
    		}
    		else {
    			throw FileException(MA_SRC, "Requested file is a directory");
    		}
    	}
    	else {
    		openFile(mode);

    		superblock_ = SuperblockCtrPtr(new SuperblockCtrType(file_, default_block_size_));
    		is_new_		= true;
    	}

    	page_buffer_ 		= CharBufferPtr(T2T<char*>(malloc(default_block_size_)), free);
    	blockmap_buffer_	= BlockMapBufferPtr(new SymbolsBuffer<1>(allocation_batch_size_));

    	blockmap_buffer_->clear();

    	if (is_new_)
    	{
    		allocateFileSpace();
    		// make superblock's space allocated
    		this->allocateEmptyBlock();

    		block_map_ 	= BlockMapPtr(new BlockMapType(this, CTR_CREATE, 0));

    		commitTemporaryToBlockMap();
    		finishFileSpaceAllocation();

    		root_map_ 	= RootMapPtr(new RootMapType(this, CTR_CREATE, 2));

    		superblock_->storeSuperblock();

    		commit();
    	}
    	else {
    		block_map_ 	= BlockMapPtr(new BlockMapType(this, CTR_FIND, 0));
    		root_map_ 	= RootMapPtr(new RootMapType(this, CTR_FIND, 2));
    	}
    }

    virtual ~FileAllocator()
    {
    	try {
    		this->close(false);
    	}
    	catch (...) {
    		//log exception here
    	}
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

    UBigInt allocation_batch_size() const {
    	return allocation_batch_size_;
    }

    // IAllocator

    virtual PageG getPage(const ID& id, Int flags)
    {
    	//FIXME: throw exception
    	if (id.isNull())
    	{
    		return PageG();
    	}

    	PageCacheEntryType* entry = get_from_log(id);


    	if (flags == Base::READ)
    	{
        	if (entry)
        	{
        		pages_.touch(entry);

        		createSharedIfNecessary(entry, Shared::UPDATE);
        	}
        	else {
        		entry = get_entry(id);

        		createSharedIfNecessary(entry, Shared::READ);
        	}
    	}
    	else {
    		if (entry)
    		{
    			pages_.touch(entry);
    		}
    		else {
    			entry = get_entry(id);

    			MEMORIA_ASSERT_FALSE(entry->front_page());

    			entry->front_page() = copyPage(entry->page());

    			pages_log_[id] = entry;
    		}

    		createSharedIfNecessary(entry, Shared::UPDATE);
    	}

    	MEMORIA_ASSERT_TRUE(entry->shared());

    	return PageG(entry->shared());
    }

    virtual PageG getPageG(Page* page)
    {
    	return getPage(page->id(), Base::READ);
    }

    virtual void updatePage(Shared* shared)
    {
    	MyShared* my_shared = static_cast<MyShared*>(shared);

    	if (my_shared->state() == Shared::READ)
    	{
    		MEMORIA_ASSERT_FALSE(get_from_log(shared->id()))

    		PageCacheEntryType* entry = my_shared->entry();

    		MEMORIA_ASSERT_TRUE(entry->page());
    		MEMORIA_ASSERT_FALSE(entry->front_page());

    		entry->front_page() = copyPage(entry->page());

    		entry->page_op() 	= PageOp::UPDATE;

    		pages_log_[entry->key()] = entry;

    		shared->set_page(entry->front_page());

    		shared->state() = Shared::UPDATE;
    	}
    }

    virtual void removePage(const ID& id)
    {
    	deleted_log_.insert(id);

    	if (pages_.contains_key(id))
    	{
    		PageCacheEntryType* entry = get_entry(id);
    		entry->page_op() = PageOp::DELETE;

    		if (entry->shared())
    		{
    			// FIXME it isn't really necessary to inform PageGuards that the page is marked deleted
    			entry->shared()->state() = Shared::DELETE;
    		}
    	}
    }

    virtual PageG createPage(Int initial_size)
    {
    	MEMORIA_ASSERT(initial_size, ==, default_block_size_);

    	UBigInt pos = this->allocateEmptyBlock();
    	ID gid		= this->createGID(pos);

    	PageCacheEntryType* entry = new PageCacheEntryType(gid, pos);

    	entry->counter() = 1;
    	entry->page_op() = PageOp::UPDATE;

    	entry->front_page() = createPage(gid);

    	pages_log_[gid]  = entry;

    	createShared(entry, Shared::UPDATE);

    	return PageG(entry->shared());
    }

    virtual void resizePage(Shared* page, Int new_size)
    {
    	throw Exception(MA_SRC, "Page resizing is not yet supported");
    }

    virtual void releasePage(Shared* shared)
    {
    	MyShared* my_shared = static_cast<MyShared*>(shared);

    	PageCacheEntryType* entry = my_shared->entry();

    	MEMORIA_ASSERT_TRUE(entry->shared() == my_shared);

    	delete my_shared;

    	entry->shared() = nullptr;

    	if ((!entry->is_allocated()) && (!get_from_log(entry->key())))
    	{
    		//freeEntry(entry);
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
    		return !iter.isEnd();
    	}
    }

    virtual BigInt createCtrName()
    {
    	return superblock_->new_ctr_name();
    }

    virtual const IAllocatorProperties& properties() const
    {
    	return properties_;
    }

    //public stuff

    ContainerMetadataRepository* getMetadata() const
    {
    	return metadata_;
    }

    void close(bool commit = true)
    {
    	if (commit) {
    		this->commit();
    	}

    	file_.close();
    }

    void sync() {
    	file_.sync();
    }

    void commit()
    {
    	// ensure file has enough room for backup

    	if (is_read_only())
    	{
    		if (!is_clean())
    		{
    			rollback();

    			throw Exception(MA_SRC, "Can't modify read-only allocator");
    		}
    	}

    	UBigInt updated = computeUpdatedPages();

    	while (superblock_->free_blocks() < updated)
    	{
    		ensureCapacity(updated);
    		updated = computeUpdatedPages();
    	}

    	// backup updated pages

    	UBigInt backup_list_head = 0;
    	UBigInt backup_list_size = 0;

    	auto blockmap_iter = block_map_->begin();

    	for (auto i: pages_log_)
    	{
    		PageCacheEntryType* entry = i.second;
    		if (isUpdatedEntry(entry))
    		{
    			Page* page = entry->page();

    			page->target_block_pos() 	= entry->position();
    			page->next_block_pos() 		= backup_list_head;

    			blockmap_iter.selectFw(1, 0);

    			MEMORIA_ASSERT_FALSE(blockmap_iter.isEnd());

    			UBigInt new_pos = blockmap_iter.pos();

    			storePage(new_pos, page);

    			backup_list_head = new_pos;

    			backup_list_size++;
    		}
    	}

    	superblock_->set_backup_list(backup_list_head, backup_list_size);

    	superblock_->storeBackup(); // sync



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
    			}

    			if (!entry->is_allocated())
    			{
    				MEMORIA_ASSERT_FALSE(entry->page());
    				pages_.insert(entry);
    			}
    			else {
    				MEMORIA_ASSERT_TRUE(entry->page());
    			}

    			std::swap(entry->page(), entry->front_page());

    			storePage(entry->position(), entry->page());

    			entry->page_op() = PageOp::NONE;

    			if (entry->front_page())
    			{
    				::free(entry->front_page());
    				entry->front_page() = nullptr;
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

    	sync();

    	superblock_->storeSuperblock(); //sync
    }

    void rollback()
    {
    	for (auto i = pages_log_.begin(); i != pages_log_.end(); i++)
    	{
    		PageCacheEntryType* entry = i->second;

    		MEMORIA_ASSERT_TRUE(entry->front_page());

    		::free(entry->front_page());

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
    }

private:

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
    	return pages_.get_entry(page_id, [this](PageCacheEntryType& entry) {
    		UBigInt pos = this->get_position(entry.key());

    		if (!entry.page())
    		{
    			entry.page() = createPage(ID(0));
    		}

    		this->loadPage(pos, entry.page());
    	});
    }


    MyShared* createSharedIfNecessary(PageCacheEntryType* entry, Int op)
    {
    	if (!entry->shared())
    	{
    		return createShared(entry, op);
    	}

    	return entry->shared();
    }

    MyShared* createShared(PageCacheEntryType* entry, Int op)
    {
    	MyShared* shared = new MyShared();

    	shared->id()    = entry->key();
    	shared->state()	= op;

    	shared->set_page(entry->current_page());
    	shared->set_allocator(this);

    	entry->shared() = shared;
    	shared->entry() = entry;

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
    	void* buf = malloc(default_block_size_);
    	memset(buf, 0, default_block_size_);

    	Page* p = new (buf) Page(id);

    	p->page_size() = default_block_size_;

    	return p;
    }


//    void createEmptyPage(PageCacheEntryType& entry, Int initial_size)
//    {
//    	if (!entry.front_page())
//    	{
//    		if (entry.page())
//    		{
//    			std::swap(entry.front_page(), entry.page());
//    		}
//    		else {
//    			void* buf = malloc(initial_size);
//    			memset(buf, 0, initial_size);
//
//    			Page* p = new (buf) Page(entry.key());
//
//    			p->page_size() = initial_size;
//
//    			entry.front_page() = p;
//    		}
//    	}
//    }

    void freeEntry(PageCacheEntryType* entry)
    {
    	::free(entry->page());
    	::free(entry->front_page());

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
    			enlargeFile();
    		}

    		auto iter = block_map_->select(0, 1);

    		MEMORIA_ASSERT_FALSE(iter.isEnd());

    		iter.setSymbol(1);
    		superblock_->decFreeBlocks();

    		return iter.pos() * default_block_size_;
    	}
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
    	file_.seek(pos, SeekType::SET);
    	file_.read(page_buffer_.get(), default_block_size_);

    	Page* disk_page = T2T<Page*>(page_buffer_.get());

    	Int page_data_size 	= disk_page->page_size();
    	Int ctr_type_hash	= disk_page->ctr_type_hash();
    	Int page_type_hash	= disk_page->page_type_hash();

    	MEMORIA_ASSERT(page_data_size, ==, default_block_size_);

    	PageMetadata* page_metadata 		= metadata_->getPageMetadata(ctr_type_hash, page_type_hash);
    	const IPageOperations* operations 	= page_metadata->getPageOperations();

    	operations->deserialize(page_buffer_.get(), page_data_size, T2T<void*>(page));
    }

    void storePage(UBigInt pos, Page* page)
    {
    	MEMORIA_ASSERT(page->page_size(), ==, default_block_size_);

    	file_.seek(pos, SeekType::SET);

    	Int ctr_type_hash	= page->ctr_type_hash();
    	Int page_type_hash	= page->page_type_hash();

    	PageMetadata* page_metadata 		= metadata_->getPageMetadata(ctr_type_hash, page_type_hash);
    	const IPageOperations* operations 	= page_metadata->getPageOperations();

    	memset(page_buffer_.get(), 0, default_block_size_);

    	operations->serialize(page, page_buffer_.get());

    	file_.write(page_buffer_.get(), default_block_size_);
    }

    void clearPageBuffer()
    {
    	memset(page_buffer_.get(), 0, default_block_size_);
    }

    void allocateFileSpace()
    {
    	clearPageBuffer();

    	UBigInt file_size = superblock_->file_size();

    	file_.seek(0, SeekType::END);

    	for (UBigInt c = 0; c < allocation_batch_size_; c++)
    	{
    		file_.write(page_buffer_.get(), default_block_size_);
    	}

    	auto pos = file_.seek(0, SeekType::CUR);

    	MEMORIA_ASSERT(pos, ==, file_size + allocation_batch_size_ * default_block_size_);

    	superblock_->enlargeFile(allocation_batch_size_ * default_block_size_);
    	superblock_->setTemporaryBlockMap(file_size, allocation_batch_size_);
    }

    void commitTemporaryToBlockMap()
    {
    	auto iter = block_map_->seek(block_map_->size());
    	iter.insert(*blockmap_buffer_.get());
    	blockmap_buffer_->reset();
    }

    void finishFileSpaceAllocation()
    {
    	auto iter = block_map_->seek(superblock_->total_blocks());

    	auto buffer = superblock_->temporary_blockmap_buffer();

    	UBigInt allocated_before = superblock_->temporary_allocated_blocks();

    	// this operation must not allocate new blocks
    	UBigInt updated = iter.update(buffer);

    	MEMORIA_ASSERT(updated, ==, allocation_batch_size_);

    	UBigInt allocated_after = superblock_->temporary_allocated_blocks();

    	MEMORIA_ASSERT(allocated_before, ==, allocated_after);

    	superblock_->updateMainBlockMapMetadata();
    	superblock_->clearTemporaryBlockMap();
    }

    void enlargeFile()
    {
    	allocateFileSpace();
    	commitTemporaryToBlockMap();
    	finishFileSpaceAllocation();

    	MEMORIA_ASSERT(superblock_->free_blocks(), >, 0);
    }

    void ensureCapacity(UBigInt capacity)
    {
    	while (superblock_->free_blocks() < capacity)
    	{
    		enlargeFile();
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

    const SuperblockCtrType* superblock() const {
    	return superblock_.get();
    }

    SuperblockCtrType* superblock() {
    	return superblock_.get();
    }
};

}



#endif
