
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

template <typename Profile>
struct ProfileSpecificBlockTools;


template <
        typename PageIdType,
        typename PageGuidType,
        typename IdValueHolder,
        typename SnapshotID
>
class AbstractPage {

public:
    static constexpr uint32_t VERSION = 1;

private:

    uint64_t    ctr_type_hash_;
    uint64_t    block_type_hash_;
    int32_t     memory_block_size_;

    uint64_t    next_block_pos_;
    uint64_t    target_block_pos_;
    std::atomic<int64_t> references_;

    IdValueHolder id_value_;
    PageIdType  id_;
    PageGuidType uuid_;
    SnapshotID snapshot_id_;



public:
    using FieldsList = TypeList<
                ConstValue<uint32_t, VERSION>,
                decltype(id_value_),
                decltype(id_),
                decltype(uuid_),
                decltype(snapshot_id_),
                decltype(ctr_type_hash_),
                decltype(block_type_hash_),
                decltype(memory_block_size_),
                decltype(next_block_pos_),
                decltype(target_block_pos_),
                int64_t //references
    >;

    using BlockID  = PageIdType;

    AbstractPage() noexcept= default;

    AbstractPage(const PageIdType &id) noexcept: id_(id), uuid_(id) {}

    const IdValueHolder &id_value() const noexcept {
        return id_value_;
    }

    IdValueHolder &id_value() noexcept {
        return id_value_;
    }

    const PageIdType &id() const noexcept {
        return id_;
    }

    PageIdType &id() noexcept {
        return id_;
    }

    const PageGuidType &uuid() const noexcept {
        return uuid_;
    }

    PageGuidType &uuid() noexcept {
        return uuid_;
    }

    const SnapshotID &snapshot_id() const noexcept {
        return snapshot_id_;
    }

    SnapshotID &snapshot_id() noexcept {
        return snapshot_id_;
    }

    void init() noexcept {}

    uint64_t &ctr_type_hash() noexcept {
        return ctr_type_hash_;
    }

    const uint64_t &ctr_type_hash() const noexcept {
        return ctr_type_hash_;
    }

    
    uint64_t &block_type_hash() noexcept {
        return block_type_hash_;
    }

    const uint64_t &block_type_hash() const noexcept {
        return block_type_hash_;
    }


    int32_t& memory_block_size() noexcept {
        return memory_block_size_;
    }

    const int32_t& memory_block_size() const noexcept {
        return memory_block_size_;
    }

    uint64_t& next_block_pos() noexcept {
        return next_block_pos_;
    }

    const uint64_t& next_block_pos() const noexcept {
        return next_block_pos_;
    }

    uint64_t& target_block_pos() noexcept {
        return target_block_pos_;
    }

    const uint64_t& target_block_pos() const noexcept {
        return target_block_pos_;
    }

    int64_t references() const noexcept {
        return references_.load(std::memory_order_acquire);
    }

    void set_references(int64_t value) {
        references_.store(value);
    }

    void ref_block(int64_t amount = 1) noexcept {
//        auto refs =
                references_.fetch_add(amount, std::memory_order_relaxed);
        //std::cout << "Ref block " << id_value_ << " :: " << (refs + amount) << std::endl;
    }

    bool unref_block() noexcept {
        auto refs = references_.fetch_sub(1, std::memory_order_acq_rel);
        //std::cout << "Unref block " << id_value_ << " :: " << (refs - 1) << std::endl;

        if (refs < 1) {
            terminate(format_u8("Internal error. Negative refcount detected for block {}", id_value_).data());
        }

        return refs == 1;
    }

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->value("GID",               &uuid_);
        handler->value("ID",                &id_);
        handler->value("SNAPSHOT_ID",       &snapshot_id_);
        handler->value("CTR_HASH",          &ctr_type_hash_);
        handler->value("PAGE_TYPE_HASH",    &block_type_hash_);
        handler->value("PAGE_SIZE",         &memory_block_size_);

        handler->value("NEXT_BLOCK_POS",    &next_block_pos_);
        handler->value("TARGET_BLOCK_POS",  &target_block_pos_);

        int64_t refs = references();
        handler->value("REFERENCES",        &refs);

        return VoidResult::of();
    }


    template <template <typename T> class FieldFactory, typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        FieldFactory<uint64_t>::serialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::serialize(buf, block_type_hash());
        FieldFactory<int32_t>::serialize(buf, memory_block_size());

        FieldFactory<uint64_t>::serialize(buf, next_block_pos());
        FieldFactory<uint64_t>::serialize(buf, target_block_pos());

        FieldFactory<PageIdType>::serialize(buf, id());
        FieldFactory<PageGuidType>::serialize(buf, uuid());
        FieldFactory<SnapshotID>::serialize(buf, snapshot_id());

        int64_t refs = references();
        FieldFactory<int64_t>::serialize(buf, refs);

        return VoidResult::of();
    }

    template <template <typename T> class FieldFactory, typename SerializationData, typename IDResolver>
    VoidResult mem_cow_serialize(SerializationData& buf, const IDResolver* id_resolver) const noexcept
    {
        FieldFactory<uint64_t>::serialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::serialize(buf, block_type_hash());
        FieldFactory<int32_t>::serialize(buf, memory_block_size());

        FieldFactory<uint64_t>::serialize(buf, next_block_pos());
        FieldFactory<uint64_t>::serialize(buf, target_block_pos());

        BlockID actual_id(id_value_);

        FieldFactory<PageIdType>::serialize(buf, actual_id);
        FieldFactory<PageGuidType>::serialize(buf, uuid());
        FieldFactory<SnapshotID>::serialize(buf, snapshot_id());

        int64_t refs = references();
        FieldFactory<int64_t>::serialize(buf, refs);

        return VoidResult::of();
    }

    template <typename IDResolver>
    VoidResult mem_cow_resolve_ids(const IDResolver* id_resolver) noexcept
    {
        MEMORIA_TRY(memref_id, id_resolver->resolve_id(id_));
        id_ = memref_id;

        return VoidResult::of();
    }

    template <template <typename T> class FieldFactory, typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        FieldFactory<uint64_t>::deserialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::deserialize(buf, block_type_hash());
        FieldFactory<int32_t>::deserialize(buf,  memory_block_size());

        FieldFactory<uint64_t>::deserialize(buf, next_block_pos());
        FieldFactory<uint64_t>::deserialize(buf, target_block_pos());

        FieldFactory<PageIdType>::deserialize(buf,   id());
        FieldFactory<PageGuidType>::deserialize(buf, uuid());
        FieldFactory<SnapshotID>::deserialize(buf,   snapshot_id());

        int64_t refs;
        FieldFactory<int64_t>::deserialize(buf, refs);
        set_references(refs);

        return VoidResult::of();
    }
};



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
    }

private:
    void refreshData(MyType* shared) noexcept
    {
        this->block_     = shared->block_;
        this->state_    = shared->state_;

        refresh();
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
        inc();
    }

    template <typename Page>
    BlockGuard(const BlockGuard<Page, AllocatorT>& guard) noexcept: shared_(guard.shared_)
    {
        ref();
        inc();
    }

    template <typename Page>
    BlockGuard(BlockGuard<Page, AllocatorT>&& guard) noexcept: shared_(guard.shared_)
    {
        guard.shared_   = NULL;
        inc();
    }

    ~BlockGuard() noexcept
    {
        dec();
        unref();
    }



    const MyType& operator=(const MyType& guard) noexcept
    {
        if (shared_ != guard.shared_)
        {
            unref();
            shared_ = guard.shared_;
            ref();
        }

        return *this;
    }


    template <typename P>
    MyType& operator=(const BlockGuard<P, AllocatorT>& guard) noexcept
    {
        unref();
        shared_ = guard.shared_;
        ref();
        return *this;
    }

    MyType& operator=(MyType&& guard) noexcept
    {
        unref();
        shared_ = guard.shared_;

        guard.shared_ = NULL;
        return *this;
    }


    template <typename P>
    MyType& operator=(BlockGuard<P, AllocatorT>&& guard) noexcept
    {
        unref();

        shared_ = guard.shared_;

        guard.shared_ = NULL;
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
                shared_->store()->releaseBlock(shared_).get_or_throw();
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




template <typename BlockType_>
class LWBlockHandler {
    BlockType_* ptr_;

    template <typename> friend class LWBlockHandler;

public:
    enum {UNDEFINED, READ, UPDATE, _DELETE};

    using Shared = LWBlockHandler;
    using BlockType = BlockType_;

    LWBlockHandler() noexcept:
        ptr_()
    {}

    template <typename U>
    LWBlockHandler(LWBlockHandler<U>&& other) noexcept:
        ptr_(ptr_cast<BlockType>(other.ptr_))
    {}

    template <typename U>
    LWBlockHandler(const LWBlockHandler<U>& other) noexcept:
        ptr_(ptr_cast<BlockType>(other.ptr_))
    {}

    LWBlockHandler(BlockType* ptr) noexcept:
        ptr_(ptr)
    {}

    ~LWBlockHandler() noexcept = default;

    operator bool() const noexcept {
        return ptr_;
    }

    bool isSet() const noexcept {
        return ptr_;
    }

    bool is_null() const noexcept {
        return ptr_ == nullptr;
    }

    template <typename TT>
    bool operator==(const LWBlockHandler<TT>& other) const noexcept {
        return ptr_ == other.ptr_;
    }

    BlockType* block() noexcept {
        return ptr_;
    }

    const BlockType* block() const noexcept {
        return ptr_;
    }

    BlockType* get() noexcept {
        return ptr_;
    }

    const BlockType* get() const noexcept {
        return ptr_;
    }

    template <typename ParentBlockType, typename = std::enable_if_t<std::is_base_of<ParentBlockType, BlockType>::value, void>>
    operator LWBlockHandler<ParentBlockType>() noexcept {
        return LWBlockHandler<ParentBlockType>(ptr_);
    }

    VoidResult update() const noexcept {
        return VoidResult::of();
    }

    BlockType* operator->() noexcept {
        return ptr_;
    }

    const BlockType* operator->() const noexcept {
        return ptr_;
    }

    Shared* shared() noexcept {
        return this;
    }

    const Shared* shared() const noexcept {
        return this;
    }
};

template <typename T, typename U>
Result<T> static_cast_block(Result<LWBlockHandler<U>>&& src) noexcept {
    if (MMA_LIKELY(src.is_ok()))
    {
        T tgt = std::move(src).get();
        return Result<T>::of(std::move(tgt));
    }
    return std::move(src).transfer_error();
}


}
