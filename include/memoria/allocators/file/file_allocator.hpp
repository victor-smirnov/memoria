
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
//#include <memoria/allocators/file/file_allocator_idmap.hpp>
//#include <memoria/allocators/file/file_allocator_blockmap.hpp>

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

//    template <typename, typename> friend class IDMapFileAllocator;
//    template <typename, typename> friend class BlockMapFileAllocator;

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
    		return (!entry.is_updated()) && entry.current_page()->references() == 0;
    	}
    };

    enum class PageOp {
    	UPDATE = Shared::UPDATE, DELETE = Shared::DELETE, NONE
    };

    template <typename Node>
    struct LRUNode: Node {

    	typedef Page* PagePtr;

    	PagePtr page_;
    	PagePtr front_page_;

    	PageOp  page_op_;

    	UBigInt position_;

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
    		return front_page_ != nullptr || page_op_ != PageOp::NONE;
    	}
    };




    typedef LRUCache<
    		ID,
    		PageCacheEvictionPredicate,
    		LRUNode,
    		IDHashMap
    > 																			PageCacheType;

    typedef typename PageCacheType::Entry										PageCacheEntryType;

    typedef std::unordered_map<ID, PageCacheEntryType*, IDKeyHash, IDKeyEq>     IDPageOpMap;
    typedef std::unordered_map<BigInt, CtrShared*>                              CtrSharedMap;

    typedef std::set<UBigInt>                              						PositionSet;
    typedef IDHashMap<ID, BigInt>                              					LocalIDMap;

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
    CtrSharedMap        ctr_shared_;

    Logger              logger_;

    ContainerMetadataRepository*    metadata_;


    const char*         type_name_ 						= "FileAllocator";

    SuperblockCtrPtr	superblock_;
    BlockMapPtr			block_map_;

    RootMapPtr        	root_map_;

    bool				id_map_entered_ = false;
    LocalIDMap			local_idmap_;

    CharBufferPtr		page_buffer_;
    BlockMapBufferPtr	blockmap_buffer_;

    StaticPool<ID, Shared>  pool_;

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

    bool is_new_;

public:
    FileAllocator(StringRef& file_name):
    	file_name_(file_name),
    	pages_(1024),
    	logger_("memoria::FileAllocator", Logger::DERIVED, &memoria::vapi::logger),
        metadata_(MetadataRepository<Profile>::getMetadata()),
        page_buffer_(nullptr, free),
        properties_(superblock_)
    {
    	BlockMapType::initMetadata();
    	RootMapType::initMetadata();

    	File file(file_name);

    	if (file.isExists())
    	{
    		if (!file.isDirectory())
    		{
    			openFile();
    			superblock_ = SuperblockCtrPtr(new SuperblockCtrType(file_));
    		}
    		else {
    			throw FileException(MA_SRC, "Requested file is a directory");
    		}

    		is_new_ = false;
    	}
    	else {
    		createFile();

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
    {}

    bool is_new() const {
    	return is_new_;
    }

    // IAllocator

    virtual PageG getPage(const ID& id, Int flags)
    {
    	if (id.isNull())
    	{
    		return PageG();
    	}

    	PageCacheEntryType* entry = get_from_log(id);

    	if (flags == Base::READ)
    	{
        	if (entry)
        	{
        		Shared* shared = get_shared(entry->front_page(), (Int)entry->page_op());
        		pages_.touch(entry);
        		return PageG(shared);
        	}
        	else {
        		PageCacheEntryType* entry = get_entry(id);
        		Shared* shared = get_shared(entry->page(), Shared::READ);
        		return PageG(shared);
        	}
    	}
    	else {
    		if (entry)
    		{
    			pages_.touch(entry);
    			return PageG(get_shared(entry->front_page(), (Int)entry->page_op()));
    		}
    		else {
    			PageCacheEntryType* entry = get_entry(id);

    			Int page_size = entry->page()->page_size();
    			char* buffer = (char*) malloc(page_size);

    			CopyByteBuffer(entry->page(), buffer, page_size);
    			Page* new_page = T2T<Page*>(buffer);

    			entry->front_page() = new_page;

    			pages_log_[id] = entry;

    			Shared* shared = pool_.get(id);

    			if (shared == nullptr)
    			{
    				shared = pool_.allocate(id);

    				shared->set_allocator(this);
    				shared->id() = id;
    			}

    			shared->set_page(new_page);
    			shared->state() = Shared::UPDATE;

    			return PageG(shared);
    		}
    	}
    }

    virtual PageG getPageG(Page* page)
    {
    	return getPage(page->id(), Base::READ);
    }

    virtual void updatePage(Shared* shared)
    {
    	if (shared->state() == Shared::READ)
    	{
    		Int page_size = shared->get()->page_size();

    		Byte* buffer = T2T<Byte*>(malloc(page_size));

    		CopyByteBuffer(shared->get(), buffer, page_size);
    		Page* front_page = T2T<Page*>(buffer);

    		PageCacheEntryType* entry = get_entry(shared->id());

    		entry->page_op() 	= PageOp::UPDATE;
    		entry->front_page() = front_page;

    		pages_log_[front_page->id()] = entry;

    		shared->set_page(front_page);
    		shared->state() = Shared::UPDATE;
    	}
    }

    virtual void removePage(const ID& id)
    {
    	Shared* shared = pool_.get(id);
    	if (shared)
    	{
    		// FIXME it isn't really necessary to inform PageGuards that the page is deleted
    		shared->state() = Shared::DELETE;

    		PageCacheEntryType* entry = get_from_log(id);

    		if (!entry)
    		{
    			entry = get_entry(id);

    			pages_log_[id] = entry;
    		}

    		entry->page_op() = PageOp::DELETE;
    	}
    	else {
    		PageCacheEntryType* entry = get_from_log(id);

    		if (!entry)
    		{
    			if (pages_.contains_key(id))
    			{
    				entry = get_entry(id);
    			}
    			else {
    				entry = new PageCacheEntryType();

    				entry->key() = id;
    			}

    			pages_log_[id] = entry;
    		}

    		entry->page_op() = PageOp::DELETE;
    	}

//    	bool result = id_map_->remove(id.value());
//    	MEMORIA_ASSERT_TRUE(result);
    }

    virtual PageG createPage(Int initial_size)
    {
    	MEMORIA_ASSERT(initial_size, ==, default_block_size_);

    	UBigInt pos = this->allocateEmptyBlock();
    	ID gid		= this->createGID(pos);

    	PageCacheEntryType* entry = new PageCacheEntryType(gid, pos);

    	entry->counter() = 1;
    	entry->page_op() = PageOp::UPDATE;

    	createEmptyPage(*entry, initial_size);

    	pages_log_[gid] 	= entry;

    	Shared* shared  	= pool_.allocate(gid);

    	shared->id()        = gid;
    	shared->state()     = Shared::UPDATE;

    	shared->set_page(entry->front_page());
    	shared->set_allocator(this);

    	return PageG(shared);
    }

    virtual void resizePage(Shared* page, Int new_size)
    {
    	throw Exception(MA_SRC, "Page resizing is not yet supported");
    }

    virtual void releasePage(Shared* shared)
    {
    	pool_.release(shared->id());
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

    void close()
    {
    	commit();
    	MEMORIA_ASSERT_TRUE(file_.close() == nullptr);
    }

    void sync() {
    	file_.sync();
    }

    void commit()
    {
    	// ensure file has enough room for backup

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
    		//std::cout<<"Log: "<<i.first<<" "<<(Int)i.second->page_op()<<std::endl;

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

    	superblock_->storeBackup();

    	sync();

    	// commit updates

    	for (auto i: pages_log_)
    	{
    		PageCacheEntryType* entry = i.second;

    		if (entry->page_op() == PageOp::UPDATE)
    		{
    			MEMORIA_ASSERT_TRUE(entry->front_page());

    			Shared* shared = pool_.get(entry->key());
    			if (shared)
    			{
    				shared->state() = Shared::READ;
    				shared->set_page(entry->front_page());
    			}

    			pages_.insert(entry);

    			std::swap(entry->page(), entry->front_page());

    			storePage(entry->position(), entry->page());

    			entry->page_op() = PageOp::NONE;

    			::free(entry->front_page());
    		}
    		else // commit page deletion
    		{
    			if (pages_.contains_entry(entry))
    			{
    				PageCacheEntryType* cache_entry = pages_.remove_entry(entry->key());

    				if (cache_entry != entry)
    				{
    					freeEntry(cache_entry);
    				}
    			}

    			freeEntry(entry);
    		}
    	}

    	for (auto i = ctr_shared_.begin(); i != ctr_shared_.end(); i++)
    	{
    		i->second->commit();
    	}

    	pages_log_.clear();

    	sync();

    	superblock_->storeSuperblock();
    }

    void rollback()
    {
    	for (auto i = pages_log_.begin(); i != pages_log_.end(); i++)
    	{
    		PageOp op = i->second;
    		::free(op.page_);
    	}

    	for (auto i = ctr_shared_.begin(); i != ctr_shared_.end(); i++)
    	{
    		i->second->rollback();
    	}

    	pages_log_.clear();
    }

private:

    void openFile()
    {
    	file_.open(file_name_.c_str(), IRandomAccessFile::READ | IRandomAccessFile::WRITE);
    }

    void createFile()
    {
    	file_.open(file_name_.c_str(), IRandomAccessFile::READ | IRandomAccessFile::WRITE | IRandomAccessFile::CREATE);
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
    			entry.page() = T2T<Page*>(::malloc(default_block_size_));
    		}

    		this->loadPage(pos, entry.page());
    	});
    }

    Shared* get_shared(Page* page, Int op)
    {
      	MEMORIA_ASSERT_TRUE(page != nullptr);

    	Shared* shared = pool_.get(page->id());

    	if (shared == nullptr)
    	{
    		shared = pool_.allocate(page->id());

    		shared->id()    = page->id();
    		shared->state()	= op;

    		shared->set_page(page);
    		shared->set_allocator(this);
    	}

    	return shared;
    }

    void createEmptyPage(PageCacheEntryType& entry, Int initial_size)
    {
    	if (!entry.front_page())
    	{
    		if (entry.page())
    		{
    			std::swap(entry.front_page(), entry.page());
    		}
    		else {
    			void* buf = malloc(initial_size);
    			memset(buf, 0, initial_size);

    			Page* p = new (buf) Page(entry.key());

    			p->page_size() = initial_size;

    			entry.front_page() = p;
    		}
    	}
    }

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
    	file_.seek(pos, IRandomAccessFile::SET);
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

    	file_.seek(pos, IRandomAccessFile::SET);

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

    	file_.seek(0, IRandomAccessFile::END);

    	for (UBigInt c = 0; c < allocation_batch_size_; c++)
    	{
    		file_.write(page_buffer_.get(), default_block_size_);
    	}

    	auto pos = file_.seek(0, IRandomAccessFile::CUR);

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
    	return entry->page_op() == PageOp::UPDATE && pages_.contains_key(entry->key());
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
