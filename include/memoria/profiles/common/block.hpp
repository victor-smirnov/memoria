
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

#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/tools/id.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/tools/result.hpp>

#include <memoria/core/types/typehash.hpp>

#include <memoria/core/container/logs.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <type_traits>

namespace memoria {



template <typename PageIdType>
class AbstractPage {

public:
    static constexpr uint32_t VERSION = 1;

private:

    uint32_t    crc_;
    uint64_t    ctr_type_hash_;
    uint64_t    block_type_hash_;
    int32_t     memory_block_size_;

    uint64_t    next_block_pos_;
    uint64_t    target_block_pos_;

    PageIdType  id_;
    PageIdType  uuid_;

    int32_t     deleted_;

    //Txn rollback intrusive list fields. Not used by containers.
public:
    using FieldsList = TypeList<
                ConstValue<uint32_t, VERSION>,
                decltype(id_),
                decltype(uuid_),
                decltype(crc_),
                decltype(ctr_type_hash_),
                decltype(block_type_hash_),
                decltype(deleted_),
                decltype(memory_block_size_),
                decltype(next_block_pos_),
                decltype(target_block_pos_)
    >;

    using BlockID  = PageIdType;

    AbstractPage() = default;

    AbstractPage(const PageIdType &id): id_(id), uuid_(id) {}

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

    
    uint64_t &block_type_hash() {
        return block_type_hash_;
    }

    const uint64_t &block_type_hash() const {
        return block_type_hash_;
    }


    int32_t &deleted() {
        return deleted_;
    }

    const int32_t& deleted() const {
        return deleted_;
    }

    int32_t& memory_block_size() {
        return memory_block_size_;
    }

    const int32_t& memory_block_size() const {
        return memory_block_size_;
    }

    int32_t data_size() const {
        return sizeof(AbstractPage);
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


    //Rebuild block content such indexes using provided data.
    void Rebiuild(){}

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->value("GID",               &uuid_);
        handler->value("ID",                &id_);
        handler->value("CRC",               &crc_);
        handler->value("CTR_HASH",          &ctr_type_hash_);
        handler->value("PAGE_TYPE_HASH",    &block_type_hash_);
        handler->value("DELETED",           &deleted_);
        handler->value("PAGE_SIZE",         &memory_block_size_);

        handler->value("NEXT_BLOCK_POS",    &next_block_pos_);
        handler->value("TARGET_BLOCK_POS",  &target_block_pos_);

        return VoidResult::of();
    }


    void copyFrom(const AbstractPage* block)
    {
        this->id()              = block->id();
        this->gid()             = block->gid();
        this->crc()             = block->crc();

        this->ctr_type_hash()       = block->ctr_type_hash();
        
        this->block_type_hash()     = block->block_type_hash();
        this->deleted()             = block->deleted();
        this->memory_block_size()   = block->memory_block_size();
    }

    template <template <typename T> class FieldFactory, typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        FieldFactory<uint32_t>::serialize(buf, crc());
        FieldFactory<uint64_t>::serialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::serialize(buf, block_type_hash());
        FieldFactory<int32_t>::serialize(buf, memory_block_size_);

        FieldFactory<uint64_t>::serialize(buf, next_block_pos_);
        FieldFactory<uint64_t>::serialize(buf, target_block_pos_);

        FieldFactory<PageIdType>::serialize(buf, id());
        FieldFactory<PageIdType>::serialize(buf, uuid());

        FieldFactory<int32_t>::serialize(buf, deleted_);

        return VoidResult::of();
    }

    template <template <typename T> class FieldFactory, typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        FieldFactory<uint32_t>::deserialize(buf, crc());
        FieldFactory<uint64_t>::deserialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::deserialize(buf, block_type_hash());
        FieldFactory<int32_t>::deserialize(buf, memory_block_size_);

        FieldFactory<uint64_t>::deserialize(buf, next_block_pos_);
        FieldFactory<uint64_t>::deserialize(buf, target_block_pos_);

        FieldFactory<PageIdType>::deserialize(buf, id());
        FieldFactory<PageIdType>::deserialize(buf, uuid());

        FieldFactory<int32_t>::deserialize(buf, deleted_);

        return VoidResult::of();
    }
};



template <typename PageIdType>
struct TypeHash<AbstractPage<PageIdType>>: HasValue<
        uint64_t,
        HashHelper<
            AbstractPage<PageIdType>::VERSION,
            TypeHashV<typename AbstractPage<PageIdType>::FlagsType>,
            TypeHashV<typename AbstractPage<PageIdType>::BlockID>,
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
    PageT*      block_;
    int32_t     references_;
    int32_t     state_;

    AllocatorT* allocator_;

    MyType* owner_;

    MyTypePtr delegate_;

public:

    enum {UNDEFINED, READ, UPDATE, _DELETE};

    template <typename Page>
    const Page* block() const noexcept {
        return ptr_cast<const Page>(block_);
    }

    template <typename Page>
    Page* block() noexcept {
        return ptr_cast<Page>(block_);
    }

    PageT* get() noexcept {
        return block_;
    }

    const PageT* get() const noexcept {
        return block_;
    }

    template <typename Page>
    operator Page* () noexcept {
        return block<Page>();
    }

    template <typename Page>
    operator const Page* () noexcept {
        return block<Page>();
    }

    int32_t references() const noexcept {
        return references_;
    }

    int32_t& references() noexcept {
        return references_;
    }

    int32_t state() const noexcept {
        return state_;
    }

    int32_t& state() noexcept {
        return state_;
    }

    const BlockID& id() const noexcept {
        return id_;
    }

    BlockID& id() noexcept {
        return id_;
    }

    template <typename Page>
    void set_block(Page* block)
    {
        this->block_ = static_cast<PageT*>(block);
    }

    void resetPage() noexcept {
        this->block_ = nullptr;
    }

    int32_t ref() noexcept {
        return ++references_;
    }

    int32_t unref() noexcept
    {
        int32_t refs = --references_;

        if (refs == 0)
        {
            unrefDelegate();
        }

        return refs;
    }

    bool deleted() const noexcept
    {
        return state_ == _DELETE;
    }

    bool updated() const noexcept
    {
        return state_ != READ;
    }

    AllocatorT* store() noexcept {
        return allocator_;
    }

    void set_allocator(AllocatorT* allocator) noexcept
    {
        allocator_ = allocator;
    }

    MyTypePtr& owner() noexcept {
        return owner_;
    }

    const MyTypePtr& owner() const noexcept {
        return owner_;
    }

    const MyTypePtr& delegate() const noexcept {
        return delegate_;
    }

    void setDelegate(MyType* delegate) noexcept
    {
        MEMORIA_V1_ASSERT_TRUE(delegate != this);

        if (delegate_)
        {
            delegate_->owner() = nullptr;
            if (delegate_->unref() == 0)
            {
                delegate_->store()->releasePage(delegate_);
                delegate_ = nullptr;
            }
        }

        delegate_ = delegate;
        delegate_->owner() = this;

        delegate_->ref();
    }


    void refresh() noexcept
    {
        if (owner_)
        {
            owner_->refreshData(this);
        }
    }

    void init() noexcept
    {
        id_         = BlockID{};
        references_ = 0;
        state_      = READ;
        block_       = nullptr;
        allocator_  = nullptr;

        owner_      = nullptr;
        delegate_   = nullptr;
    }

private:
    void refreshData(MyType* shared) noexcept
    {
        this->block_     = shared->block_;
        this->state_    = shared->state_;

        refresh();
    }

    void unrefDelegate() noexcept
    {
        if (delegate_ && delegate_->unref() == 0)
        {
            delegate_->store()->releaseBlock(delegate_).terminate_if_error();
            delegate_ = nullptr;
        }
    }
};




template <typename PageT, typename AllocatorT>
class BlockGuard {

public:

    typedef BlockGuard<PageT, AllocatorT>                               MyType;
    typedef PageT                                                       Page;
    typedef PageT                                                       BlockType;
    typedef AllocatorT                                                  Allocator;
    typedef PageShared<AllocatorT>                                      Shared;

private:
    Shared*     shared_;

public:


    BlockGuard(Shared* shared) noexcept: shared_(shared)
    {
        inc();
        ref();
    }


    BlockGuard() noexcept: shared_(nullptr)
    {
        inc();
    }

    BlockGuard(const MyType& guard) noexcept: shared_(guard.shared_)
    {
        ref();
        check();
        inc();
    }

    template <typename Page>
    BlockGuard(const BlockGuard<Page, AllocatorT>& guard) noexcept: shared_(guard.shared_)
    {
        ref();
        check();
        inc();
    }

    template <typename Page>
    BlockGuard(BlockGuard<Page, AllocatorT>&& guard) noexcept: shared_(guard.shared_)
    {
        guard.shared_   = NULL;
        check();
        inc();
    }

    ~BlockGuard() noexcept
    {
        dec();
        unref();
    }

    template <typename Page>
    operator const Page* () const noexcept
    {
        return static_cast<const Page*>(*shared_);
    }

    template <typename Page>
    operator Page* () noexcept
    {
        return static_cast<Page*>(*shared_);
    }

    const MyType& operator=(const MyType& guard) noexcept
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
    MyType& operator=(const BlockGuard<P, AllocatorT>& guard) noexcept
    {
        unref();
        shared_ = guard.shared_;
        check();
        ref();
        return *this;
    }

    MyType& operator=(MyType&& guard) noexcept
    {
        unref();
        shared_ = guard.shared_;

        guard.shared_ = NULL;
        check();
        return *this;
    }


    template <typename P>
    MyType& operator=(BlockGuard<P, AllocatorT>&& guard) noexcept
    {
        unref();

        shared_ = guard.shared_;

        guard.shared_ = NULL;
        check();
        return *this;
    }


    bool operator==(const PageT* block) const noexcept
    {
        return shared_ != NULL ? *shared_ == block : (char*)shared_ == (char*)block;
    }

    bool operator!=(const PageT* block) const noexcept
    {
        return shared_ != NULL ? *shared_ != block : (char*)shared_ != (char*)block;
    }

    bool isEmpty() const noexcept {
        return shared_ == NULL || shared_->get() == NULL;
    }

    bool isSet() const noexcept {
        return shared_ != NULL && shared_->get() != NULL;
    }

    bool operator==(const MyType& other) const noexcept
    {
        return shared_ != NULL && other.shared_ != NULL && shared_->id() == other.shared_->id();
    }

    bool operator!=(const MyType& other) const noexcept
    {
        return shared_ != NULL && other.shared_ != NULL && shared_->id() != other.shared_->id();
    }

    operator bool() const noexcept {
        return this->isSet();
    }

    const PageT* block() const noexcept {
        return *shared_;
    }

    PageT* block() noexcept {
        return *shared_;
    }

    void set_block(PageT* block) noexcept
    {
        shared_->set_block(block);
    }

    const PageT* operator->() const noexcept {
        return *shared_;
    }

    PageT* operator->() noexcept {
        return *shared_;
    }

    bool is_updated() const noexcept
    {
        return shared_->updated();
    }



    VoidResult update() noexcept
    {
        if (shared_)
        {
            MEMORIA_TRY(guard, shared_->store()->updateBlock(shared_));
            if (guard.shared() != shared_)
            {
                *this = guard;
            }
        }

        return VoidResult::of();
    }

    VoidResult resize(int32_t new_size) noexcept
    {
        if (shared_ != nullptr)
        {
            return shared_->store()->resizeBlock(shared_, new_size);
        }

        return VoidResult::of();
    }

    void clear() noexcept {
        *this = nullptr;
    }

    const Shared* shared() const noexcept {
        return shared_;
    }

    Shared* shared() noexcept{
        return shared_;
    }

    template <typename Page, typename Allocator> friend class BlockGuard;

private:

    void check() noexcept {}

    void inc() noexcept {}

    void dec() noexcept {}

    void ref() noexcept
    {
        if (shared_ != nullptr)
        {
            shared_->ref();
        }
    }

    void unref() noexcept
    {
        if (shared_ != nullptr)
        {
            if (shared_->unref() == 0)
            {
                shared_->store()->releaseBlock(shared_).terminate_if_error();
            }
        }
    }
};

template <typename T, typename U, typename AllocatorT>
Result<T> static_cast_block(Result<BlockGuard<U, AllocatorT>>&& src) noexcept {
    if (MMA_LIKELY(src.is_ok()))
    {
        T tgt = std::move(src).get();
        return Result<T>::of(std::move(tgt));
    }
    return std::move(src).transfer_error();
}


template <typename T, typename A>
std::ostream& operator<<(std::ostream& out, const BlockGuard<T, A>& pg) noexcept
{
    if (pg.isSet()) {
        out << pg->id();
    }
    else {
        out << "nullptr";
    }

    return out;
}


template <typename T, typename A>
LogHandler* logIt(LogHandler* log, const BlockGuard<T, A>& value) noexcept {
    log->log(value.block());
    log->log(" ");
    return log;
}



template <typename T>
std::ostream& operator<<(std::ostream& out, const BlockID<T>& id) noexcept
{
    IDValue idv(id);
    out<<idv;
    return out;
}


template <typename T>
OutputStreamHandler& operator<<(OutputStreamHandler& out, const BlockID<T>& id) noexcept
{
    out << id.value();
    return out;
}

template <typename T>
InputStreamHandler& operator>>(InputStreamHandler& in, BlockID<T>& id) noexcept
{
    in >> id.value();
    return in;
}

}
