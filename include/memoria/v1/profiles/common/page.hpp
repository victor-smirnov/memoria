
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/metadata/metadata.hpp>
#include <memoria/v1/metadata/block.hpp>

#include <memoria/v1/core/tools/buffer.hpp>
#include <memoria/v1/core/tools/id.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/core/container/logs.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {

template <int32_t Size>
class BitBuffer: public StaticBuffer<Size % 32 == 0 ? Size / 32 : ((Size / 32) + 1)> {
    typedef StaticBuffer<
                (Size % 32 == 0 ? Size / 32 : ((Size / 32) + 1))
    >                                                                           Base;
public:

    typedef int32_t                                                             Index;
    typedef typename Base::ElementType                                          Bits;

    static const int32_t kBitSize           = Size;

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
        SetBit(*this, index + RESERVED_BITSIZE, bit);
    }
};

template <int32_t Size>
struct TypeHash<BitBuffer<Size> > {
public:
    static const uint64_t Value = 123456 * Size;
};

template <typename PageIdType, int32_t FlagsCount = 32>
class AbstractPage {
//    static_assert(std::is_trivial<PageIdType>::value, "PageIdType must be a trivial type");

public:
    static constexpr uint32_t VERSION = 1;
    typedef BitBuffer<FlagsCount> FlagsType;

private:
    typedef AbstractPage<PageIdType, FlagsCount> Me;

    uint32_t    crc_;
    uint64_t    ctr_type_hash_;
    uint64_t    page_type_hash_;
    int32_t     page_size_;

    uint64_t    next_block_pos_;
    uint64_t    target_block_pos_;

    PageIdType  id_;
    PageIdType  uuid_;

    FlagsType   flags_;

    int32_t     deleted_;

    //Txn rollback intrusive list fields. Not used by containers.


public:
    typedef TypeList<
                ConstValue<uint32_t, VERSION>,
                decltype(flags_),
                decltype(id_),
                decltype(uuid_),
                decltype(crc_),
                decltype(ctr_type_hash_),
                decltype(page_type_hash_),
                decltype(deleted_),
                decltype(page_size_),
                decltype(next_block_pos_),
                decltype(target_block_pos_)
    >                                                                           FieldsList;

    using BlockID  = PageIdType;

    AbstractPage() = default;

    AbstractPage(const PageIdType &id): id_(id), uuid_(id), flags_() {}

    const PageIdType &id() const {
        return id_;
    }

    PageIdType &id() {
        return id_;
    }

    const PageIdType &uuid() const {
        return uuid_;
    }

    PageIdType &uuid() {
        return uuid_;
    }

    void init() {}

    FlagsType &flags() {
        return flags_;
    }

    uint32_t &crc() {
        return crc_;
    }

    const uint32_t &crc() const {
        return crc_;
    }

    uint64_t &ctr_type_hash() {
        return ctr_type_hash_;
    }

    const uint64_t &ctr_type_hash() const {
        return ctr_type_hash_;
    }

    
    uint64_t &page_type_hash() {
        return page_type_hash_;
    }

    const uint64_t &page_type_hash() const {
        return page_type_hash_;
    }


    int32_t &deleted() {
        return deleted_;
    }

    const int32_t& deleted() const {
        return deleted_;
    }

    int32_t& page_size() {
        return page_size_;
    }

    const int32_t& page_size() const {
        return page_size_;
    }

    int32_t data_size() const {
        return sizeof(Me);
    }

    bool is_updated() {
        return flags_.isBit(0);
    }

    void set_updated(bool updated) {
        return flags_.setBit(0, updated);
    }

    uint64_t& next_block_pos() {
        return next_block_pos_;
    }

    const uint64_t& next_block_pos() const {
        return next_block_pos_;
    }

    uint64_t& target_block_pos() {
        return target_block_pos_;
    }

    const uint64_t& target_block_pos() const {
        return target_block_pos_;
    }



    void *operator new(size_t size, void *page) {
        return page;
    }

    void operator delete(void *buf) {}

    //Rebuild page content such indexes using provided data.
    void Rebiuild(){}

    void generateLayoutEvents(IBlockLayoutEventHandler* handler) const {}

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->value("GID",               &uuid_);
        handler->value("ID",                &id_);
        handler->value("CRC",               &crc_);
        handler->value("CTR_HASH",          &ctr_type_hash_);
        handler->value("PAGE_TYPE_HASH",    &page_type_hash_);
        handler->value("DELETED",           &deleted_);
        handler->value("PAGE_SIZE",         &page_size_);

        handler->value("NEXT_BLOCK_POS",    &next_block_pos_);
        handler->value("TARGET_BLOCK_POS",  &target_block_pos_);
    }


    void copyFrom(const Me* page)
    {
        this->id()              = page->id();
        this->gid()             = page->gid();
        this->crc()             = page->crc();

        this->ctr_type_hash()           = page->ctr_type_hash();
        
        this->page_type_hash()  = page->page_type_hash();
        this->deleted()         = page->deleted();
        this->page_size()       = page->page_size();
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<uint32_t>::serialize(buf, crc());
        FieldFactory<uint64_t>::serialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::serialize(buf, page_type_hash());
        FieldFactory<int32_t>::serialize(buf, page_size_);

        FieldFactory<uint64_t>::serialize(buf, next_block_pos_);
        FieldFactory<uint64_t>::serialize(buf, target_block_pos_);

        FieldFactory<PageIdType>::serialize(buf, id());
        FieldFactory<PageIdType>::serialize(buf, uuid());

        FieldFactory<int32_t>::serialize(buf, deleted_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<uint32_t>::deserialize(buf, crc());
        FieldFactory<uint64_t>::deserialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::deserialize(buf, page_type_hash());
        FieldFactory<int32_t>::deserialize(buf, page_size_);

        FieldFactory<uint64_t>::deserialize(buf, next_block_pos_);
        FieldFactory<uint64_t>::deserialize(buf, target_block_pos_);

        FieldFactory<PageIdType>::deserialize(buf, id());
        FieldFactory<PageIdType>::deserialize(buf, uuid());

        FieldFactory<int32_t>::deserialize(buf, deleted_);
    }
};



template <typename PageIdType, int32_t FlagsCount>
struct TypeHash<AbstractPage<PageIdType, FlagsCount>>: HasValue<
		uint64_t,
		HashHelper<
            AbstractPage<PageIdType, FlagsCount>::VERSION,
            TypeHashV<typename AbstractPage<PageIdType, FlagsCount>::FlagsType>,
            TypeHashV<typename AbstractPage<PageIdType, FlagsCount>::BlockID>,
            TypeHashV<int32_t>,
            8
		>
>{};





template <typename AllocatorT>
class PageShared {

    typedef PageShared<AllocatorT>          MyType;
    typedef MyType*                         MyTypePtr;

    using PageT   = typename AllocatorT::BlockType;
    using BlockID = typename AllocatorT::BlockID;


    BlockID     id_;
    PageT*      page_;
    int32_t     references_;
    int32_t     state_;

    AllocatorT* allocator_;

    MyType* owner_;

    MyTypePtr delegate_;

public:

    enum {UNDEFINED, READ, UPDATE, _DELETE};

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

    int32_t references() const {
        return references_;
    }

    int32_t& references() {
        return references_;
    }

    int32_t state() const {
        return state_;
    }

    int32_t& state() {
        return state_;
    }

    const BlockID& id() const {
        return id_;
    }

    BlockID& id() {
        return id_;
    }

    template <typename Page>
    void set_page(Page* page)
    {
        this->page_ = static_cast<PageT*>(page);
    }

    void resetPage() {
        this->page_ = nullptr;
    }

    int32_t ref() {
        return ++references_;
    }

    int32_t unref()
    {
        int32_t refs = --references_;

        if (refs == 0)
        {
            unrefDelegate();
        }

        return refs;
    }

    bool deleted() const
    {
        return state_ == _DELETE;
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

    MyTypePtr& owner() {
        return owner_;
    }

    const MyTypePtr& owner() const {
        return owner_;
    }

    const MyTypePtr& delegate() const {
        return delegate_;
    }

    void setDelegate(MyType* delegate)
    {
        MEMORIA_V1_ASSERT_TRUE(delegate != this);

        if (delegate_)
        {
            delegate_->owner() = nullptr;
            if (delegate_->unref() == 0)
            {
                delegate_->allocator()->releasePage(delegate_);
                delegate_ = nullptr;
            }
        }

        delegate_ = delegate;
        delegate_->owner() = this;

        delegate_->ref();
    }


    void refresh()
    {
        if (owner_)
        {
            owner_->refreshData(this);
        }
    }

    void init()
    {
        id_         = BlockID{};
        references_ = 0;
        state_      = READ;
        page_       = nullptr;
        allocator_  = nullptr;

        owner_      = nullptr;
        delegate_   = nullptr;
    }

private:
    void refreshData(MyType* shared)
    {
        this->page_     = shared->page_;
        this->state_    = shared->state_;

        refresh();
    }

    void unrefDelegate()
    {
        if (delegate_ && delegate_->unref() == 0)
        {
            delegate_->allocator()->releaseBlock(delegate_);
            delegate_ = nullptr;
        }
    }
};




template <typename PageT, typename AllocatorT>
class BlockGuard {

public:

    typedef BlockGuard<PageT, AllocatorT>                                MyType;
    typedef PageT                                                       Page;
    typedef AllocatorT                                                  Allocator;
    typedef PageShared<AllocatorT>                                      Shared;

private:
    Shared*     shared_;

public:


    BlockGuard(Shared* shared): shared_(shared)
    {
        inc();
        ref();
    }


    BlockGuard(): shared_(nullptr)
    {
        inc();
    }

    BlockGuard(const MyType& guard): shared_(guard.shared_)
    {
        ref();
        check();
        inc();
    }

    template <typename Page>
    BlockGuard(const BlockGuard<Page, AllocatorT>& guard): shared_(guard.shared_)
    {
        ref();
        check();
        inc();
    }

    template <typename Page>
    BlockGuard(BlockGuard<Page, AllocatorT>&& guard): shared_(guard.shared_)
    {
        guard.shared_   = NULL;
        check();
        inc();
    }

    ~BlockGuard()
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
    MyType& operator=(const BlockGuard<P, AllocatorT>& guard)
    {
        unref();
        shared_ = guard.shared_;
        check();
        ref();
        return *this;
    }

    MyType& operator=(MyType&& guard)
    {
        unref();
        shared_ = guard.shared_;

        guard.shared_ = NULL;
        check();
        return *this;
    }


    template <typename P>
    MyType& operator=(BlockGuard<P, AllocatorT>&& guard)
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
        if (shared_)// && !shared_->updated())
        {
            auto guard = shared_->allocator()->updateBlock(shared_);

            if (guard.shared() != shared_)
            {
                *this = guard;
            }
        }
    }

    void resize(int32_t new_size)
    {
        if (shared_ != nullptr)
        {
            shared_->allocator()->resizeBlock(shared_, new_size);
        }
    }

    void clear() {
        *this = nullptr;
    }

    const Shared* shared() const {
        return shared_;
    }

    Shared* shared() {
        return shared_;
    }

    template <typename Page, typename Allocator> friend class BlockGuard;

private:

    void check() {}

    void inc() {}

    void dec() {}

    void ref()
    {
        if (shared_ != nullptr)
        {
            shared_->ref();
        }
    }

    void unref()
    {
        if (shared_ != nullptr)
        {
            if (shared_->unref() == 0)
            {
                shared_->allocator()->releaseBlock(shared_);
            }
        }
    }

};


template <typename T, typename A>
std::ostream& operator<<(std::ostream& out, const BlockGuard<T, A>& pg)
{
    if (pg.isSet()) {
        out<<pg->id();
    }
    else {
        out<<"nullptr";
    }
    return out;
}


template <typename T, typename A>
LogHandler* logIt(LogHandler* log, const BlockGuard<T, A>& value) {
    log->log(value.page());
    log->log(" ");
    return log;
}



template <typename T>
std::ostream& operator<<(std::ostream& out, const PageID<T>& id)
{
    IDValue idv(id);
    out<<idv;
    return out;
}


template <typename T>
OutputStreamHandler& operator<<(OutputStreamHandler& out, const PageID<T>& id)
{
    out << id.value();
    return out;
}

template <typename T>
InputStreamHandler& operator>>(InputStreamHandler& in, PageID<T>& id)
{
    in >> id.value();
    return in;
}

}}
