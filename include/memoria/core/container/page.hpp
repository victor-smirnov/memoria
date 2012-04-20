
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_PAGE_HPP
#define	_MEMORIA_CORE_CONTAINER_PAGE_HPP

#include <memoria/metadata/metadata.hpp>
#include <memoria/metadata/page.hpp>

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
        return is_empty();
    }

    bool is_empty() const {
    	return Base::value() == 0;
    }

    bool is_not_null() const {
    	return !is_null();
    }

    bool is_set() const {
    	return Base::value() != 0;
    }

    void set_null() {
        Base::Clear();
    }

    ValueType& operator=(const ValueType& other)
	{
    	Base::value() = other.value();
    	return *this;
	}

    ValueType& operator=(const T& other)
    {
    	Base::value() = other;
    	return *this;
    }

    bool operator==(const ValueType& other) const
    {
    	return Base::value() == other.value();
    }

    bool operator!=(const ValueType& other) const
    {
    	return Base::value() != other.value();
    }

    bool operator<(const ValueType& other) const
    {
    	return Base::value() < other.value();
    }

    operator BigInt () {
    	return Base::value();
    }

    operator BigInt () const {
    	return Base::value();
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

//    const FlagsType &flags() const {
//        return flags_;
//    };

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

    void GenerateLayoutEvents(IPageLayoutEventHandler* handler) const {}

    void GenerateDataEvents(IPageDataEventHandler* handler) const
    {
    	IDValue id(&id_);
    	handler->Value("ID",				&id);
    	handler->Value("CRC", 				&crc_);
    	handler->Value("MODEL_HASH", 		&model_hash_);
    	handler->Value("PAGE_TYPE_HASH", 	&page_type_hash_);
    	handler->Value("REFERENCES", 		&references_);
    	handler->Value("DELETED", 			&deleted_);
    }

    template <typename PageType>
    void CopyFrom(const PageType* page)
    {
        this->id()              = page->id();
        this->crc()             = page->crc();
        this->model_hash()      = page->model_hash();
        this->page_type_hash()  = page->page_type_hash();
        this->references()		= page->references();
        this->deleted()			= page->deleted();
    }

    template <template <typename> class FieldFactory>
    void Serialize(SerializationData& buf) const
    {
    	FieldFactory<PageIdType>::serialize(buf, id());
    	FieldFactory<Int>::serialize(buf, crc());
    	FieldFactory<Int>::serialize(buf, model_hash());
    	FieldFactory<Int>::serialize(buf, page_type_hash());
    	FieldFactory<Int>::serialize(buf, references_);
    	FieldFactory<Int>::serialize(buf, deleted_);
    }

    template <template <typename> class FieldFactory>
    void Deserialize(DeserializationData& buf)
    {
    	FieldFactory<PageIdType>::deserialize(buf, id());
    	FieldFactory<Int>::deserialize(buf, crc());
    	FieldFactory<Int>::deserialize(buf, model_hash());
    	FieldFactory<Int>::deserialize(buf, page_type_hash());
    	FieldFactory<Int>::deserialize(buf, references_);
    	FieldFactory<Int>::deserialize(buf, deleted_);
    }
};


template <typename AllocatorT>
class PageShared {

	typedef typename AllocatorT::Page 		PageT;
	typedef typename AllocatorT::Page::ID 	ID;


	ID		id_;
	PageT* 	page_;
	Int 	references_;
	Int 	state_;

	AllocatorT*	allocator_;

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

	PageT* get() {
		return page_;
	}

	const PageT* get() const {
		return page_;
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

	const ID& id() const {
		return id_;
	}

	ID& id() {
		return id_;
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

	AllocatorT* allocator() {
		return allocator_;
	}

	void set_allocator(AllocatorT* allocator)
	{
		allocator_ = allocator;
	}

	void init()
	{
		references_ = 0;
		state_		= READ;
		page_		= NULL;
		allocator_	= NULL;
	}
};

template <typename PageT, typename AllocatorT>
class PageGuard {

public:

	typedef PageGuard<PageT, AllocatorT> 								MyType;
	typedef PageT														Page;
	typedef AllocatorT 													Allocator;
	typedef PageShared<AllocatorT>										Shared;

private:
	Shared* 	shared_;

public:


	PageGuard(Shared* shared): shared_(shared)
	{
		inc();
		ref();
	}


	PageGuard(): shared_(NULL)
	{
		inc();
	}

	PageGuard(const MyType& guard): shared_(guard.shared_)
	{
		ref();
		check();
		inc();
	}

	template <typename Page>
	PageGuard(const PageGuard<Page, AllocatorT>& guard): shared_(guard.shared_)
	{
		ref();
		check();
		inc();
	}

	template <typename Page>
	PageGuard(PageGuard<Page, AllocatorT>&& guard): shared_(guard.shared_)
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

	const MyType& operator=(const MyType& guard)
	{
		if (shared_ != guard.shared_)
		{
			unref();
			shared_ = guard.shared_;
			check();
			ref();
		}

		return *this;
	}


	template <typename P>
	MyType& operator=(const PageGuard<P, AllocatorT>& guard)
	{
		unref();
		shared_ = guard.shared_;
		check();
		ref();
		return *this;
	}

#ifndef __clang__
	MyType& operator=(MyType&& guard)
	{
		unref();
		shared_ = guard.shared_;

		guard.shared_ = NULL;
		check();
		return *this;
	}
#endif

	template <typename P>
	MyType& operator=(PageGuard<P, AllocatorT>&& guard)
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

	bool is_empty() const {
		return shared_ == NULL || shared_->get() == NULL;
	}

	bool is_set() const {
		return shared_ != NULL && shared_->get() != NULL;
	}

	bool operator==(const MyType& other) const
	{
		return shared_ != NULL && other.shared_ != NULL && shared_->id() == other.shared_->id();
	}

	bool operator!=(const MyType& other) const
	{
		return shared_ != NULL && other.shared_ != NULL && shared_->id() != other.shared_->id();
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

	bool is_updated() const
	{
		return shared_->updated();
	}

	void update()
	{
		if (shared_ != NULL && !shared_->updated())
		{
			shared_->allocator()->UpdatePage(shared_);
		}
	}

	void Clear() {
		*this = NULL;
	}

	const Shared* shared() const {
		return shared_;
	}

	Shared* shared() {
		return shared_;
	}

	template <typename Page, typename Allocator> friend class PageGuard;

private:

	void check() {}

	void inc() {}

	void dec() {}

	void ref()
	{
		if (shared_ != NULL)
		{
			shared_->ref();
		}
	}

	void unref()
	{
		if (shared_ != NULL)
		{
			if (shared_->unref() == 0)
			{
				shared_->allocator()->ReleasePage(shared_);
			}
		}
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
