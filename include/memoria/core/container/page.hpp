
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_PAGE_HPP
#define _MEMORIA_CORE_CONTAINER_PAGE_HPP

#include <memoria/metadata/metadata.hpp>
#include <memoria/metadata/page.hpp>

#include <memoria/core/tools/buffer.hpp>
#include <memoria/core/tools/id.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/core/container/logs.hpp>

#include <type_traits>


namespace memoria    {

using namespace memoria::vapi;

extern Int PageCtrCnt[10];
extern Int PageDtrCnt[10];

extern Int PageCtr;
extern Int PageDtr;

extern bool GlobalDebug;

template <typename T>
class PageID: public ValueBuffer<T> {
public:
    typedef PageID<T>                                                           ValueType;
    typedef ValueBuffer<T>                                                      Base;

    PageID() = default;

    PageID(const T& t): Base(t) {}

    PageID(const memoria::vapi::IDValue& id): Base()
    {
        Base::copyFrom(id.ptr());
    }

    static_assert(std::is_trivial<ValueBuffer<T>>::value, "ValueBuffer<> must be a trivial type");

    bool isNull() const {
        return isEmpty();
    }

    bool isEmpty() const {
        return Base::value() == 0;
    }

    bool isNotEmpty() const {
        return Base::value() != 0;
    }

    bool isNotNull() const {
        return !isNull();
    }

    bool isSet() const {
        return Base::value() != 0;
    }

    void setNull() {
        Base::clear();
    }

    ValueType& operator=(const ValueType& other) = default;
//    {
//        Base::value() = other.value();
//        return *this;
//    }

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


template <typename T>
struct TypeHash<PageID<T>>: UIntValue<
    HashHelper<101, TypeHash<T>::Value>::Value
> {};




template <typename T>
static LogHandler& operator<<(LogHandler &log, const PageID<T>& value)
{
    IDValue id(&value);
    log.log(id);
    log.log(" ");
    return log;
}

template <typename T>
static LogHandler* logIt(LogHandler* log, const PageID<T>& value)
{
    IDValue id(&value);
    log->log(id);
    log->log(" ");
    return log;
}


template <Int Size>
class BitBuffer: public StaticBuffer<Size % 32 == 0 ? Size / 32 : ((Size / 32) + 1)> {
    typedef StaticBuffer<
                (Size % 32 == 0 ? Size / 32 : ((Size / 32) + 1))
    >                                                                           Base;
public:


    typedef Int                                                                 Index;
    typedef typename Base::ElementType                                          Bits;

    static const Int kBitSize           = Size;

    static const int RESERVED_SIZE      = 0;
    static const int RESERVED_BITSIZE   = RESERVED_SIZE * 8;

    BitBuffer() = default;

    bool isBit(Index index) const {
        return GetBit(*this, index + RESERVED_BITSIZE);
    }

    Bits getBits(Index idx, Index count) const {
        return GetBits(*this, idx, count);
    }

    void setBits(Index idx, Bits bits, Index count) {
        SetBits(*this, idx, bits, count);
    }

    void setBit(int index, int bit) {
        memoria::SetBit(*this, index + RESERVED_BITSIZE, bit);
    }
};

template <Int Size>
struct TypeHash<BitBuffer<Size> > {
public:
    static const UInt Value = 123456 * Size;
};

template <typename PageIdType, Int FlagsCount = 32>
class AbstractPage {
	static_assert(std::is_trivial<PageIdType>::value, "PageIdType must be a trivial type");

public:
    static const UInt VERSION                                                   = 1;
    typedef BitBuffer<FlagsCount> FlagsType;

private:
    typedef AbstractPage<PageIdType, FlagsCount> Me;



    FlagsType   flags_;
    PageIdType  id_;

    Int         crc_;
    Int         master_ctr_type_hash_;
    Int         owner_ctr_type_hash_;
    Int         ctr_type_hash_;
    Int         page_type_hash_;
    Int         references_;
    Int         deleted_;
    Int         page_size_;

public:
    typedef TypeList<
                ConstValue<UInt, VERSION>,
                decltype(flags_),
                decltype(id_),
                decltype(crc_),
                decltype(master_ctr_type_hash_),
                decltype(owner_ctr_type_hash_),
                decltype(ctr_type_hash_),
                decltype(page_type_hash_),
                decltype(references_),
                decltype(deleted_),
                decltype(page_size_)
    >                                                                           FieldsList;

    typedef PageIdType                                                          ID;

    AbstractPage() = default;

    AbstractPage(const PageIdType &id): flags_(), id_(id) {}

    const PageIdType &id() const {
        return id_;
    }

    PageIdType &id() {
        return id_;
    }

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

    Int &ctr_type_hash() {
        return ctr_type_hash_;
    }

    const Int &ctr_type_hash() const {
        return ctr_type_hash_;
    }

    Int &owner_ctr_type_hash() {
        return owner_ctr_type_hash_;
    }

    const Int &owner_ctr_type_hash() const {
        return owner_ctr_type_hash_;
    }

    Int &master_ctr_type_hash() {
        return master_ctr_type_hash_;
    }

    const Int &master_ctr_type_hash() const {
        return master_ctr_type_hash_;
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

    Int& page_size() {
        return page_size_;
    }

    const Int& page_size() const {
        return page_size_;
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
        return flags_.isBit(0);
    }

    void set_updated(bool updated) {
        return flags_.setBit(0, updated);
    }

    void *operator new(size_t size, void *page) {
        return page;
    }

    void operator delete(void *buf) {}

    //Rebuild page content such indexes using provided data.
    void Rebiuild(){}

    void generateLayoutEvents(IPageLayoutEventHandler* handler) const {}

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        IDValue id(&id_);
        handler->value("ID",                &id);
        handler->value("CRC",               &crc_);
        handler->value("MASTER_MODEL_HASH", &master_ctr_type_hash_);
        handler->value("OWNER_MODEL_HASH",  &owner_ctr_type_hash_);
        handler->value("MODEL_HASH",        &ctr_type_hash_);
        handler->value("PAGE_TYPE_HASH",    &page_type_hash_);
        handler->value("REFERENCES",        &references_);
        handler->value("DELETED",           &deleted_);
        handler->value("PAGE_SIZE",         &page_size_);
    }


    void copyFrom(const Me* page)
    {
        this->id()              = page->id();
        this->crc()             = page->crc();

        this->ctr_type_hash()           = page->ctr_type_hash();
        this->master_ctr_type_hash()    = page->master_ctr_type_hash();
        this->owner_ctr_type_hash()     = page->owner_ctr_type_hash();

        this->page_type_hash()  = page->page_type_hash();
        this->references()      = page->references();
        this->deleted()         = page->deleted();
        this->page_size()       = page->page_size();
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<PageIdType>::serialize(buf, id());
        FieldFactory<Int>::serialize(buf, crc());
        FieldFactory<Int>::serialize(buf, master_ctr_type_hash());
        FieldFactory<Int>::serialize(buf, owner_ctr_type_hash());
        FieldFactory<Int>::serialize(buf, ctr_type_hash());
        FieldFactory<Int>::serialize(buf, page_type_hash());
        FieldFactory<Int>::serialize(buf, references_);
        FieldFactory<Int>::serialize(buf, deleted_);
        FieldFactory<Int>::serialize(buf, page_size_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<PageIdType>::deserialize(buf, id());
        FieldFactory<Int>::deserialize(buf, crc());
        FieldFactory<Int>::deserialize(buf, master_ctr_type_hash());
        FieldFactory<Int>::deserialize(buf, owner_ctr_type_hash());
        FieldFactory<Int>::deserialize(buf, ctr_type_hash());
        FieldFactory<Int>::deserialize(buf, page_type_hash());
        FieldFactory<Int>::deserialize(buf, references_);
        FieldFactory<Int>::deserialize(buf, deleted_);
        FieldFactory<Int>::deserialize(buf, page_size_);
    }
};



template <typename PageIdType, Int FlagsCount>
struct TypeHash<AbstractPage<PageIdType, FlagsCount>> {
    static const UInt Value = HashHelper<
            AbstractPage<AbstractPage<PageIdType, FlagsCount>>::VERSION,
            TypeHash<typename AbstractPage<PageIdType, FlagsCount>::FlagsType>::Value,
            TypeHash<typename AbstractPage<PageIdType, FlagsCount>::ID>::Value,
            TypeHash<Int>::Value,
            8
    >::Value;
};





template <typename AllocatorT>
class PageShared {

    typedef typename AllocatorT::Page       PageT;
    typedef typename AllocatorT::Page::ID   ID;


    ID      id_;
    PageT*  page_;
    Int     references_;
    Int     state_;

    AllocatorT* allocator_;

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
        state_      = READ;
        page_       = NULL;
        allocator_  = NULL;
    }
};

template <typename PageT, typename AllocatorT>
class PageGuard {

public:

    typedef PageGuard<PageT, AllocatorT>                                MyType;
    typedef PageT                                                       Page;
    typedef AllocatorT                                                  Allocator;
    typedef PageShared<AllocatorT>                                      Shared;

private:
    Shared*     shared_;

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
        guard.shared_   = NULL;
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

    bool isEmpty() const {
        return shared_ == NULL || shared_->get() == NULL;
    }

    bool isSet() const {
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

    operator bool() const {
        return this->isSet();
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
            shared_->allocator()->updatePage(shared_);
        }
    }

    void resize(Int new_size)
    {
        if (shared_ != nullptr)
        {
            shared_->allocator()->resizePage(shared_, new_size);
        }
    }

    void clear() {
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
                shared_->allocator()->releasePage(shared_);
            }
        }
    }

};


template <typename T, typename A>
LogHandler* logIt(LogHandler* log, const PageGuard<T, A>& value) {
    log->log(value.page());
    log->log(" ");
    return log;
}


}

namespace std {

using namespace memoria;

template <typename T>
ostream& operator<<(ostream& out, const PageID<T>& id)
{
    IDValue idv(id);
    out<<idv;
    return out;
}

}

#endif
