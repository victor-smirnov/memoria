
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_PAGE_HPP
#define	_MEMORIA_CORE_CONTAINER_PAGE_HPP

#include <memoria/metadata/metadata.hpp>


#include <memoria/core/tools/buffer.hpp>
#include <memoria/vapi/models/logs.hpp>

namespace memoria    {

extern Int PageCtrCnt[10];
extern Int PageDtrCnt[10];

extern Int PageCtr;
extern Int PageDtr;

extern bool GlobalDebug;

template <typename T, size_t Size = sizeof(T)>
class AbstractPageID: public ValueBuffer<T, Size> {
public:
    typedef AbstractPageID<T, Size>                 ValueType;
    typedef ValueBuffer<T, Size>                 	Base;

    AbstractPageID(): Base() {}

    AbstractPageID(const T &t) : Base(t) {}

    AbstractPageID(const IDValue& id): Base() {
        Base::CopyFrom(id.ptr());
    }

    bool is_null() const {
        return IsClean(Base::ptr(), Size);
    }

    bool is_not_null() const {
    	return !is_null();
    }

    void set_null() {
        Clean(Base::ptr(), Size);
    }
};

template <typename T, size_t Size>
static LogHandler& operator<<(LogHandler &log, const AbstractPageID<T, Size>& value)
{
    IDValue id(&value);
    log.log(id);
    log.log(" ");
    return log;
}

template <typename T, size_t Size>
static LogHandler* LogIt(LogHandler* log, const AbstractPageID<T, Size>& value)
{
    IDValue id(&value);
    log->log(id);
    log->log(" ");
    return log;
}


template <Int Size>
class BitBuffer: public Buffer<Size % 8 == 0 ? Size / 8 : ((Size / 8) + 1)> {
    typedef Buffer<(Size % 8 == 0 ? Size / 8 : ((Size / 8) + 1))>               Base;
public:

    typedef Int                         Index;
    typedef Long                        Bits;

    static const Int kBitSize           = Size;

    static const int RESERVED_SIZE      = 0;
    static const int RESERVED_BITSIZE   = RESERVED_SIZE * 8;

    BitBuffer() : Base() {}

    bool is_bit(Index index) const {
        return GetBit(*this, index + RESERVED_BITSIZE);
    }

    Bits get_bits(Index idx, Index count) const {
        return GetBits(*this, idx, count);
    }

    void set_bits(Index idx, Bits bits, Index count) {
        SetBits(*this, idx, bits, count);
    }

    void set_bit(int index, int bit) {
        SetBit(*this, index + RESERVED_BITSIZE, bit);
    }


};

template <typename PageIdType, Int FlagsCount = 32>
class AbstractPage {

    typedef AbstractPage<PageIdType, FlagsCount> Me;

    typedef BitBuffer<FlagsCount> FlagsType;

    FlagsType   flags_;
    PageIdType  id_;

    Int         crc_;
    Int         model_hash_;
    Int         page_type_hash_;
    Int 		references_;
    Int 		deleted_;

public:
    typedef PageIdType      ID;

    AbstractPage(): flags_(), id_() {}

    AbstractPage(const PageIdType &id): flags_(), id_(id) {}

    const PageIdType &id() const {
        return id_;
    }

    PageIdType &id() {
        return id_;
    }

    const FlagsType &flags() const {
        return flags_;
    };

    void init() {}

    FlagsType &flags() {
        return flags_;
    };

    Int &crc() {
        return crc_;
    }

    const Int &crc() const {
        return crc_;
    }

    Int &model_hash() {
        return model_hash_;
    }

    const Int &model_hash() const {
        return model_hash_;
    }

    Int &page_type_hash() {
        return page_type_hash_;
    }

    const Int &page_type_hash() const {
        return page_type_hash_;
    }

    Int &references() {
        return references_;
    }

    const Int references() const {
        return references_;
    }

    Int &deleted() {
    	return deleted_;
    }

    const Int deleted() const {
    	return deleted_;
    }

    Int ref() {
    	return ++references_;
    }

    Int unref() {
    	return --references_;
    }

    Int data_size() const {
        return sizeof(Me);
    }

    bool is_updated() {
    	return flags_.is_bit(0);
    }

    void set_updated(bool updated) {
    	return flags_.set_bit(0, updated);
    }

    void *operator new(size_t size, void *page) {
        return page;
    }

    void operator delete(void *buf) {}

    //Rebuild page content such indexes using provided data.
    void Rebiuild(){}

    template <template <typename> class FieldFactory>
    void BuildFieldsList(MetadataList &list, Long &abi_ptr) const {
        FieldFactory<PageIdType>::create(list, id(),        "ID",               abi_ptr);
        FieldFactory<FlagsType>::create(list,  flags(),     "FLAGS",            abi_ptr);
        FieldFactory<Int>::create(list,  crc(),             "CRC",              abi_ptr);
        FieldFactory<Int>::create(list,  model_hash(),      "MODEL_HASH",       abi_ptr);
        FieldFactory<Int>::create(list,  page_type_hash(),  "PAGE_TYPE_HASH",   abi_ptr);
        FieldFactory<Int>::create(list,  references_,  		"REFERENCES",   	abi_ptr);
        FieldFactory<Int>::create(list,  deleted_,  		"DELETED",   		abi_ptr);
    }

    template <typename PageType>
    void CopyFrom(const PageType* page)
    {
        this->id()              = page->id();
        this->flags()           = page->flags();
        this->crc()             = page->crc();
        this->model_hash()      = page->model_hash();
        this->page_type_hash()  = page->page_type_hash();
        this->references()		= page->references();
        this->deleted()			= page->deleted();
    }
};


template <typename PageT>
class PageShared {

	PageT* page_;
	Int references_;
	Int state_;

public:

	enum {READ, UPDATE, DELETE};

	template <typename Page>
	const Page* page() const {
		return static_cast<const Page*>(page_);
	}

	template <typename Page>
	Page* page() {
		return static_cast<Page*>(page_);
	}

	template <typename Page>
	operator Page* () {
		return page<Page>();
	}

	template <typename Page>
	operator const Page* () {
		return page<Page>();
	}

	Int references() const {
		return references_;
	}

	Int& references() {
		return references_;
	}

	Int state() const {
		return state_;
	}

	Int& state() {
		return state_;
	}

	template <typename Page>
	void set_page(Page* page)
	{
		this->page_ = static_cast<PageT*>(page);
	}

	Int ref() {
		return ++references_;
	}

	Int unref() {
		return --references_;
	}

	bool deleted() const
	{
		return state_ == DELETE;
	}

	bool updated() const
	{
		return state_ != READ;
	}

	void init()
	{
		references_ = 0;
		state_		= READ;
		page_		= NULL;
	}
};

template <typename PageT, typename AllocatorT>
class PageGuard {

public:

	typedef PageGuard<PageT, AllocatorT> 								MyType;
	typedef PageT														Page;
	typedef AllocatorT 													Allocator;
	typedef PageShared<typename AllocatorT::Page>						Shared;

private:
	Shared* 	shared_;
	AllocatorT*	allocator_;
public:


	PageGuard(Shared* shared, Allocator* allocator): shared_(shared), allocator_(allocator)
	{
		inc();
		ref();
	}


	PageGuard(Allocator* allocator): shared_(NULL), allocator_(allocator) {inc();}

	PageGuard(const MyType& guard): shared_(guard.shared_), allocator_(guard.allocator_)
	{
		ref();
		check();
		inc();
	}

	template <typename Page>
	PageGuard(const PageGuard<Page, AllocatorT>& guard): shared_(guard.shared_), allocator_(guard.allocator_)
	{
		ref();
		check();
		inc();
	}

	template <typename Page>
	PageGuard(PageGuard<Page, AllocatorT>&& guard): shared_(guard.shared_), allocator_(guard.allocator_)
	{
		guard.shared_	= NULL;
		check();
		inc();
	}

	~PageGuard()
	{
		dec();
		unref();
	}

	template <typename Page>
	operator const Page* () const
	{
		return static_cast<const Page*>(*shared_);
	}

	template <typename Page>
	operator Page* ()
	{
		return static_cast<Page*>(*shared_);
	}

	void check() {
	}

	void inc() {
		PageCtr++;
		if (PageCtr > PageDtr) PageDtr = PageCtr;
	}

	void dec() {
		PageCtr--;
	}

	void ref()
	{
		if (shared_ != NULL)
		{
			shared_->ref();
		}
	}

	void unref()
	{
		//FIXME it should be: page_->unref() == 0
		if (shared_ != NULL && shared_->unref() == 0)
		{
			allocator_->ReleasePage(shared_);
		}
	}

	const MyType& operator=(const MyType& guard)
	{
		unref();
		shared_ = guard.shared_;
		check();
		ref();
		return *this;
	}


	template <typename P>
	const MyType& operator=(const PageGuard<P, AllocatorT>& guard)
	{
		unref();
		shared_ = guard.page_;
		check();
		ref();
		return *this;
	}

	const MyType& operator=(MyType&& guard)
	{
		unref();
		shared_ = guard.shared_;
		guard.shared_ = NULL;
		check();
		return *this;
	}


	template <typename P>
	const MyType& operator=(PageGuard<P, AllocatorT>&& guard)
	{
		unref();
		shared_ = guard.shared_;
		guard.shared_ = NULL;
		check();
		return *this;
	}


	bool operator==(const PageT* page) const
	{
		return shared_ != NULL ? *shared_ == page : (char*)shared_ == (char*)page;
	}

	bool operator!=(const PageT* page) const
	{
		return shared_ != NULL ? *shared_ != page : (char*)shared_ != (char*)page;
	}

	bool operator==(const MyType& other) const
	{
		return shared_ == other.shared_;
	}

	bool operator!=(const MyType& other) const
	{
		return shared_ != other->shared_;
	}

	const PageT* page() const {
		return *shared_;
	}

	PageT* page() {
		return *shared_;
	}

	void set_page(PageT* page)
	{
		shared_->set_page(page);
	}

	const PageT* operator->() const {
		return *shared_;
	}

	PageT* operator->() {
		return *shared_;
	}

	AllocatorT* allocator() {
		return allocator_;
	}

	void set_allocator(AllocatorT* allocator)
	{
		allocator_ = allocator;
	}

	void update()
	{
		if (shared_ != NULL && !shared_->updated())
		{
			set_page(allocator_->UpdatePage(*shared_));
		}
	}

	template <typename Page, typename Allocator> friend class PageGuard;

private:
	template <typename Page>
	void set_page(PageGuard<Page, Allocator>&& other)
	{
		shared_ = other.shared_;
		other.shared_ = NULL;
	}
};


template <typename T, typename A>
LogHandler* LogIt(LogHandler* log, const PageGuard<T, A>& value) {
    log->log(value.page());
    log->log(" ");
    return log;
}


}

namespace std {

using namespace memoria;

template <typename T, size_t Size>
ostream& operator<<(ostream& out, const AbstractPageID<T, Size>& id)
{
	out<<id.value();
	return out;
}

}

#endif
