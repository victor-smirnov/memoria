
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef		_MEMORIA_MODULES_CONTAINERS_STREAM_POSIX_MANAGER_HPP
#define		_MEMORIA_MODULES_CONTAINERS_STREAM_POSIX_MANAGER_HPP

//#include <map>
#include <unordered_map>
#include <string>

#include <memoria/vapi.hpp>
#include <memoria/vapi/models/logs.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/tools/pool.hpp>

#include <malloc.h>



namespace memoria {


typedef struct
{
	template <typename T, size_t S>
	long operator() (const AbstractPageID<T,S> &k) const { return k.value(); }
} IDKeyHash;

typedef struct
{
	template <typename T, size_t S>
	bool operator() (const AbstractPageID<T,S> &x, const AbstractPageID<T,S> &y) const { return x == y; }
} IDKeyEq;

using namespace memoria::vapi;

template <typename Profile, typename PageType, typename TxnType>
class InMemAllocator: public AbstractAllocatorFactory<Profile, AbstractAllocatorName<PageType, 4096> >::Type {

	typedef IAbstractAllocator<PageType>										Base;
	typedef InMemAllocator<Profile, PageType, TxnType>							Me;

public:
	typedef typename Base::Page 												Page;
	typedef typename Base::PageG 												PageG;
	typedef typename Base::Shared 												Shared;
	typedef typename Base::CtrShared 											CtrShared;
	typedef typename Page::ID 													ID;
	static const Int PAGE_SIZE 													= 4096;

	typedef Base 																AbstractAllocator;

	typedef Ctr<typename CtrTF<Profile, Root>::CtrTypes>						RootMapType;

private:
	typedef InMemAllocator<Profile, PageType, TxnType> 						MyType;

	struct PageOp
	{
		enum {UPDATE = Shared::UPDATE, DELETE = Shared::DELETE, NONE};
		ID id_;
		Page* page_;
		Int op_;

		PageOp(Page* page): id_(page->id()), page_(page), op_(UPDATE) 	{}
		PageOp(const ID& id): id_(id), page_(NULL), op_(DELETE) 		{}
		PageOp(): id_(), page_(NULL), op_(NONE) 						{}
	};

	typedef std::unordered_map<ID, Page*, IDKeyHash, IDKeyEq> 					IDPageMap;
	typedef std::unordered_map<ID, PageOp, IDKeyHash, IDKeyEq> 					IDPageOpMap;
	typedef std::unordered_map<BigInt, CtrShared*> 								CtrSharedMap;


	IDPageMap 			pages_;
	IDPageOpMap 		pages_log_;
	CtrSharedMap		ctr_shared_;

	Logger 				logger_;
	Int 				counter_;
	ContainerMetadataRepository* 	metadata_;

	ID 					root_;
	ID 					root_log_;
	bool				updated_;

	MyType& 			me_;
	const char* 		type_name_;
	BigInt 				allocs1_;
	BigInt 				allocs2_;
	Me* 				roots_;
	RootMapType* 		root_map_;

	StaticPool<ID, Shared>	pool_;


public:
	InMemAllocator() :
		logger_("memoria::StreamAllocator", Logger::DERIVED, &memoria::vapi::logger),
		counter_(100), metadata_(MetadataRepository<Profile>::GetMetadata()), root_(0), root_log_(0), updated_(false), me_(*this),
		type_name_("StreamAllocator"), allocs1_(0), allocs2_(0), roots_(this)//, root_map_(*this, 0, true)
	{
		root_map_ = new RootMapType(*this, 0, true);
	}

	InMemAllocator(const InMemAllocator& other):
			logger_(other.logger_),
			counter_(other.counter_), metadata_(other.metadata_), root_(other.root_), root_log_(0), updated_(false), me_(*this),
			type_name_("StreamAllocator"), allocs1_(other.allocs1_), allocs2_(other.allocs2_), roots_(this), //root_map_(*this, 0, false),
			pool_(other.pool_)
	{
		for (auto i = other.pages_.begin(); i != other.pages_.end(); i++)
		{
			char* buffer = (char*) malloc(PAGE_SIZE);
			CopyBuffer(i->second, buffer, PAGE_SIZE);
			pages_[i->first] = T2T<Page*>(buffer);
		}

		delete root_map_;
	}

	virtual ~InMemAllocator() throw ()
	{
		try {
			Int npages = 0;
			for (auto i = pages_.begin(); i != pages_.end(); i++)
			{
				MEMORIA_TRACE(me(), "Free Page", i->second);
				::free(i->second);
				npages++;
			}

			//FIXME: clear pages_log_
			//FIXME: clear ctr_shared_

			if (allocs1_ - npages > 0)
			{
				MEMORIA_ERROR(me(), "Page leak detected:", npages, allocs1_, (allocs1_ - npages));
			}
		}
		catch (...) {
		}
	}

	ContainerMetadataRepository* GetMetadata() const {
		return metadata_;
	}

	virtual Logger& logger() {
		return logger_;
	}

	virtual Logger* GetLogger() {
		return &logger_;
	}

	RootMapType* roots() {
		return root_map_;
	}

	const ID &root() const
	{
		if (updated_)
		{
			return root_log_;
		}
		else {
			return root_;
		}
	}


	virtual void ReleasePage(Shared* shared)
	{
		pool_.Release(shared->id());
	}

	PageOp get_in_log(const ID &page_id)
	{
		auto i = pages_log_.find(page_id);
		if (i != pages_log_.end())
		{
			return i->second;
		}
		else {
			return PageOp();
		}
	}

	Page *get0(const ID &page_id)
	{
		auto i = pages_.find(page_id);
		if (i != pages_.end())
		{
			if (i->second == NULL)
			{
				throw NullPointerException(MEMORIA_SOURCE, "Null page for the specified page_id");
			}
			return i->second;
		}
		else {
			throw NullPointerException(MEMORIA_SOURCE, "Can't find page for the specified page_id: " + ToString(page_id.value()));
		}
	}

	Page *get1(const ID &page_id)
	{
		if (page_id.is_null())
		{
			return NULL;
		}
		else {
			return get0(page_id);
		}
	}

	// Begin RootMapInterface

	void set_root(const ID& id) {
		root_map_->set_root(id);
	}

	void remove_by_key(BigInt name)
	{
		root_map_->Remove(name);
	}

	void set_value_for_key(BigInt name, const ID& page_id)
	{
		root_map_->operator[](name).SetData(page_id);
	}

	ID get_value_for_key(BigInt name)
	{
		auto iter = root_map_->Find(name);

		if (!iter.IsEnd())
		{
			return iter.GetData();
		}
		else {
			return ID(0);
			//throw new MemoriaException(MEMORIA_SOURCE, "Can't find Root ID for model " + ToString(name));
		}
	}

	// End RootMapInterface

	Int get_page_size()
	{
		return PAGE_SIZE;
	}

	Shared* get_shared(Page* page, Int op)
	{
		Shared* shared = pool_.Get(page->id());

		if (shared == NULL)
		{
			shared = pool_.Allocate(page->id());

			shared->id() 		= page->id();
			shared->state() 	= op;
			shared->set_page(page);
			shared->set_allocator(this);
		}

		return shared;
	}


	virtual PageG GetPage(const ID& id, Int flags)
	{
		if (id.is_null())
		{
			return PageG();
		}


		PageOp op = get_in_log(id);

		if (flags == Base::READ)
		{
			if (op.op_ != PageOp::NONE)
			{
				Shared* shared = get_shared(op.page_, op.op_);
				return PageG(shared);
			}
			else
			{
				Page* 	page = get0(id);
				Shared* shared = get_shared(page, Shared::READ);
				return PageG(shared);
			}
		}
		else {
			if (op.op_ == PageOp::NONE)
			{
				Page* page = get0(id);

				char* buffer = (char*) malloc(PAGE_SIZE);
				allocs1_++;
				CopyBuffer(page, buffer, PAGE_SIZE);
				Page* page2 = T2T<Page*>(buffer);

				pages_log_[id] = page2;

				Shared* shared = pool_.Get(id);

				if (shared == NULL)
				{
					shared = pool_.Allocate(id);

					shared->set_allocator(this);
					shared->id() = id;
				}

				shared->set_page(page2);
				shared->state() = Shared::UPDATE;

				return PageG(shared);
			}
			else
			{
				return PageG(get_shared(op.page_, op.op_));
			}
		}
	}

	virtual PageG GetPageG(Page* page)
	{
//		Shared* shared = pool_.Get(id);
//		if (shared == NULL)
//		{
//			shared = pool_.Allocate(id);
//
//			shared->set_allocator(this);
//			shared->id() = id;
//		}
//		else {
//			return PageG(shared);
//		}

		return GetPage(page->id(), Base::READ);
	}

	virtual void UpdatePage(Shared* shared)
	{
		if (shared->state() == Shared::READ)
		{
			char* buffer = (char*) malloc(PAGE_SIZE);
			allocs1_++;

			CopyBuffer(shared->get(), buffer, PAGE_SIZE);
			Page* page0 = T2T<Page*>(buffer);

			pages_log_[page0->id()] = page0;

			shared->set_page(page0);
			shared->state() = Shared::UPDATE;
		}
	}

	virtual void  RemovePage(const ID& id)
	{
		Shared* shared = pool_.Get(id);
		if (shared != NULL)
		{
			// FIXME it doesn't really necessary to inform PageGuards that the page is deleted
			shared->state() = Shared::DELETE;
		}

		auto i = pages_log_.find(id);
		if (i != pages_log_.end())
		{
			(i->second).op_ = PageOp::DELETE;
		}
		else {
			pages_log_[id] = PageOp(id);
		}
	}


	/**
	 * If a tree page is created using new (allocator) PageType call
	 * than Page() constructor is invoked twice with undefined results
	 */
	virtual PageG CreatePage(Int initial_size = PAGE_SIZE)
	{
		allocs1_++;
		char* buf = (char*) malloc(PAGE_SIZE);
//		Clean(buf, PAGE_SIZE);

		for (int c = 0; c < PAGE_SIZE; c++)
		{
			buf[c] = 0;
		}

		ID id = counter_++;

		Page* p = new (buf) Page(id);

		pages_log_[id] = p;

		Shared* shared 	= pool_.Allocate(id);

		shared->id() 		= id;
		shared->state() 	= Shared::UPDATE;

		shared->set_page(p);
		shared->set_allocator(this);

		return PageG(shared);
	}

	void stat() {
		cout<<allocs1_<<" "<<allocs2_<<" "<<pages_.size()<<endl;
	}

	void clear()
	{
		pages_.clear();
		pages_log_.clear();
		ctr_shared_.clear();

		//pool_.Clear();


		root_ 		= 0;
		root_log_ 	= 0;
		counter_ 	= 100;
	}

	void commit()
	{
		for (auto i = pages_log_.begin(); i != pages_log_.end(); i++)
		{
			PageOp op = i->second;

			if (op.op_ == PageOp::UPDATE)
			{
				op.page_->set_updated(false);

				auto j = pages_.find(op.id_);
				if (j != pages_.end())
				{
					::free(j->second);
					allocs2_--;
				}

				pages_[op.id_] = op.page_;

				Shared* page = pool_.Get(op.id_);
				if (page != NULL)
				{
					page->set_page(op.page_);
				}
			}
			else {
				auto j = pages_.find(op.id_);
				if (j != pages_.end())
				{
					Shared* shared = pool_.Get(op.id_);
					if (shared != NULL)
					{
						::free(shared->get());
						shared->set_page((Page*)NULL);
					}
					else {
						::free(j->second);
					}

					pages_.erase(op.id_);
					allocs2_--;
				}
			}
		}

		for (auto i = ctr_shared_.begin(); i != ctr_shared_.end(); i++)
		{
			CtrShared* shared = i->second;
			if (shared->updated())
			{
				shared->root() 		= shared->root_log();
				shared->root_log() 	= 0;
				shared->updated() 	= false;
			}
		}

		if (updated_)
		{
			root_ = root_log_;
			root_log_ = 0;
			updated_ = false;
		}

		pages_log_.clear();
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
			CtrShared* shared = i->second;

			if (shared->updated())
			{
				shared->root_log() = 0;
				shared->updated() = false;
			}
		}

		if (updated_)
		{
			updated_ 	= false;
			root_log_ 	= 0;
		}

		pages_log_.clear();
	}


	virtual void ResizePage(Shared* page, Int new_size)
	{
	}

	virtual PageG GetRoot(BigInt name, Int flags)
	{
		if (name == 0)
		{
			return GetPage(root(), flags);
		}
		else {
			return GetPage(roots_->get_value_for_key(name), flags);
		}
	}

	virtual ID GetRootID(BigInt name)
	{
		if (name == 0)
		{
			return root();
		}
		else {
			return roots_->get_value_for_key(name);
		}
	}

	virtual void SetRoot(BigInt name, const ID& root)
	{
		new_root(name, root);
	}

	virtual CtrShared* GetCtrShared(BigInt name)
	{
		auto i = ctr_shared_.find(name);

		if (i != ctr_shared_.end())
		{
			return i->second;
		}
		else
		{
			throw MemoriaException(MEMORIA_SOURCE, "Unknown CtrShared requested for name "+ToString(name));
		}
	}

	virtual void RegisterCtrShared(CtrShared* shared)
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
			throw MemoriaException(MEMORIA_SOURCE, "CtrShared for name "+ToString(name)+" is already registered");
		}
	}

	virtual void UnregisterCtrShared(CtrShared* shared)
	{
		ctr_shared_.erase(shared->name());
	}

	virtual bool IsCtrSharedRegistered(BigInt name)
	{
		return ctr_shared_.find(name) != ctr_shared_.end();
	}

	virtual void load(InputStreamHandler *input)
	{
		//FIXME: clear allocator
		commit();
		//clear();

		char signature[12];

		MEMORIA_TRACE(me(),"Read header from:", input->pos());
		input->read(signature, sizeof(signature));
		MEMORIA_TRACE(me(),"Current:", input->pos());

		if (!(signature[0] == 'M' && signature[1] == 'E' && signature[2] == 'M' && signature[3] == 'O' && signature[4] == 'R' && signature[5] == 'I' && signature[6] == 'A'))
		{
			throw MemoriaException(MEMORIA_SOURCE, "The stream does not start from MEMORIA signature: "+String(signature));
		}

		if (!(signature[7] == 0 || signature[7] == 1))
		{
			throw BoundsException(MEMORIA_SOURCE, "Endiannes filed value is out of bounds", signature[7], 0, 1);
		}

		if (signature[8] != 0)
		{
			throw MemoriaException(MEMORIA_SOURCE, "This is not a stream container");
		}

		bool first = true;
		Short size;
		while (input->read(size))
		{
			char buf[PAGE_SIZE];
			for (Int c = 0; c < PAGE_SIZE; c++)
				buf[c] = 0;

			Int page_hash;

			MEMORIA_TRACE(me(),"File pos before reading page hash:", input->pos());
			input->read(page_hash);

			MEMORIA_TRACE(me(),"Page size", size, "from", input->pos(), "page_hash=", page_hash);
			input->read(buf, 0, size);

			Page* page = T2T<Page*>(buf);

			PageMetadata* pageMetadata = metadata_->GetPageMetadata(page_hash);

			char* mem = new char[PAGE_SIZE];
			for (Int c = 0; c < PAGE_SIZE; c++)
			{
				mem[c] = 0;
			}

			pageMetadata->GetPageOperations()->Deserialize(page, size, mem);

			page = T2T<Page*>(mem);

			pages_[page->id()] = page;

			MEMORIA_TRACE(me(), "Register page", page, page->id());

			if (first)
			{
				root_ = page->id();
				first = false;
			}
		}

		Int maxId = -1;
		for (auto i = pages_.begin(); i != pages_.end(); i++)
		{
			Int idValue = i->second->id().value();
			if (idValue > maxId)
			{
				maxId = idValue;
			}
		}

		counter_ = maxId + 1;
	}

	virtual void store(OutputStreamHandler *output)
	{
		char signature[12] = "MEMORIA";
		for (UInt c = 7; c < sizeof(signature); c++) signature[c] = 0;

		output->write(&signature, 0, sizeof(signature));

		char buf[PAGE_SIZE];

		if (!root_.is_null())
		{
			dump_page(output, buf, pages_[root_]);

			for (typename IDPageMap::iterator i = pages_.begin(); i!= pages_.end(); i++)
			{
				Page *page = i->second;
				if (page != NULL && !(page->id() == root_))
				{
					dump_page(output, buf, page);
				}
			}
		}

		output->close();
	}

	void dump_page(OutputStreamHandler *output, char* buf, Page *page)
	{
		if (page->page_type_hash() != 0)
		{
			if (page->references() > 0) {cout<<"Dump "<<page->id()<<" "<<page->references()<<endl;}

			MEMORIA_TRACE(me(), "Dump page with hashes", page->page_type_hash(), page->model_hash(), "with id", page->id(), page, &page->id());

			PageMetadata* pageMetadata = metadata_->GetPageMetadata(page->page_type_hash());

			for (Int c = 0; c < PAGE_SIZE; c++)
			{
				buf[c] = 0;
			}

			const IPageOperations* operations = pageMetadata->GetPageOperations();

			operations->Serialize(page, buf);

			Int ptr = operations->GetPageSize(page);

			Short size = ptr;

			output->write(size);
			output->write(page->page_type_hash());

			MEMORIA_TRACE(me(), "Page size", size, "at", output->pos(), page->page_type_hash());
			output->write(buf, 0, size);
		}
		else {
			MEMORIA_TRACE(me(), "hash for page", page->id(), "is not specified");
		}
	}

	void set_root(BigInt name, const ID &page_id)
	{
		if (name == 0)
		{
			root_log_ = page_id;
			updated_ = true;
		}
		else {
			roots_->set_value_for_key(name, page_id);
		}
	}

	void remove_root(BigInt name)
	{
		if (name == 0)
		{
			root_log_.Clear();
			updated_ = true;
		}
		else {
			roots_->remove_by_key(name);
		}
	}

	virtual void new_root(BigInt name, const ID &page_id)
	{
		MEMORIA_TRACE(me(), "Register new root", page_id, "for", name);

		if (page_id.is_null())
		{
			remove_root(name);
		}
		else {
			set_root(name, page_id);
		}
	}

	bool is_log(Int level) {
		return logger_.IsLogEnabled(level);
	}

	const char* type_name() {
		return type_name_;
	}

	// Allocator implementaion


	virtual memoria::vapi::Page* CreatePageWrapper()
	{
		return new PageWrapper<Page, PAGE_SIZE>();
	}

	virtual void GetRootPageId(IDValue& id)
	{
		id = IDValue(&root_);
	}

	BigInt GetPageCount() {
		return pages_.size();
	}

	void GetPage(memoria::vapi::Page* page, const IDValue& idValue)
	{
		if (page == NULL)
		{
			throw NullPointerException(MEMORIA_SOURCE, "page must not be null");
		}

		Page* page0 =  this->get1(idValue);
		page->SetPtr(page0);
	}



	MyType* me() {
		return static_cast<MyType*>(this);
	}

	const MyType* me() const {
		return static_cast<const MyType*>(this);
	}


	void DumpPages(ostream& out = cout)
	{
		for (auto i = pages_.begin(); i != pages_.end(); i++)
		{
			Page* page = i->second;
			PageMetadata* pageMetadata = metadata_->GetPageMetadata(page->page_type_hash());

			PageWrapper<Page, PAGE_SIZE> pw(page);
			memoria::vapi::DumpPage(pageMetadata, &pw, out);
			out<<endl;
			out<<endl;
		}
	}

	bool Check()
	{
		bool result = false;

		for (auto iter = this->roots()->Begin(); !iter.IsEnd(); )
		{
			PageG page = this->GetPage(iter.GetData(), Base::READ);

			ContainerMetadata* ctr_meta = metadata_->GetContainerMetadata(page->model_hash());

			result = ctr_meta->GetCtrInterface()->Check(&page->id(), this) || result;

			iter.Next();
		}

		return result;
	}


	virtual void* AllocateMemory(size_t size)
	{
		return malloc(size);
	}

	virtual void FreeMemory(void* ptr)
	{
		free(ptr);
	}

};

}



#endif
