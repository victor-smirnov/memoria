
// Copyright 2011-2021 Victor Smirnov
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
#include <memoria/core/memory/ptr_cast.hpp>

#include <type_traits>
#include <atomic>


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

    using BlockID   = PageIdType;
    using BlockGUID = PageGuidType;

    AbstractPage() noexcept= default;

    AbstractPage(const PageIdType &id, const PageGuidType &guid) noexcept:
        id_(id),
        uuid_(guid) {}

    AbstractPage(const PageIdType &id, const PageGuidType &guid, const IdValueHolder& ivh) noexcept:
        id_value_(ivh),
        id_(id),
        uuid_(guid)
    {}

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
        references_.fetch_add(amount, std::memory_order_relaxed);
    }

    bool unref_block() noexcept
    {
        auto refs = references_.fetch_sub(1, std::memory_order_acq_rel);
        if (refs < 1) {
             terminate(format_u8("Internal error. Negative refcount detected for block {}", id_value_).data());
        }
        return refs == 1;
    }

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        return wrap_throwing([&]{
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
        });
    }


    template <template <typename T> class FieldFactory, typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        return wrap_throwing([&]{
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
        });
    }

    template <template <typename T> class FieldFactory, typename SerializationData, typename IDResolver>
    VoidResult cow_serialize(SerializationData& buf, const IDResolver* id_resolver) const noexcept
    {
        return wrap_throwing([&]{
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
        });
    }

    template <typename IDResolver>
    VoidResult cow_resolve_ids(const IDResolver* id_resolver) noexcept
    {
        return wrap_throwing([&]{
            auto memref_id = id_resolver->resolve_id(id_);
            id_ = memref_id;
            return VoidResult::of();
        });
    }

    template <template <typename T> class FieldFactory, typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        return wrap_throwing([&]{
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
        });
    }
};





template <typename StoreT, typename PageT, typename BlockID, typename Base = EmptyType>
class PageShared: public Base {

    BlockID     id_;
    PageT*      block_;
    int32_t     references_;
    int32_t     state_;

    StoreT* allocator_;

    bool mutable_{false};
public:
    enum {UNDEFINED, READ, UPDATE, _DELETE};

    using BlockType = PageT;

    PageShared() noexcept {}

    PageShared(const BlockID& id, PageT* block, int32_t state, StoreT* allocator) noexcept:
        id_(id),
        block_(block),
        references_(0),
        state_(state),
        allocator_(allocator)
    {}

    PageShared(const BlockID& id, PageT* block, int32_t state) noexcept:
        id_(id),
        block_(block),
        references_(0),
        state_(state),
        allocator_(nullptr)
    {}

    virtual ~PageShared() noexcept = default;

    bool is_mutable() const noexcept {return mutable_;}
    void set_mutable(bool value) noexcept {
        mutable_ = value;
    }

    void assert_mutable() const {
        if (!mutable_) {
            MEMORIA_MAKE_GENERIC_ERROR("Block {} is immutable", id_).do_throw();
        }
    }

    template <typename Page>
    const Page* block() const noexcept {
        return ptr_cast<const Page>(block_);
    }

    template <typename Page>
    Page* block() noexcept {
        return ptr_cast<Page>(block_);
    }

    PageT* ptr() const noexcept {
        return block_;
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

    BlockID& id() noexcept {
        return id_;
    }

    const BlockID& id() const noexcept {
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

    void ref() noexcept {
        references_++;
    }

    bool unref() noexcept
    {
        return --references_ == 0;
    }

    bool deleted() const noexcept
    {
        return state_ == _DELETE;
    }

    bool updated() const noexcept
    {
        return state_ != READ;
    }

    StoreT* store() noexcept {
        return allocator_;
    }

    void set_allocator(StoreT* allocator) noexcept
    {
        allocator_ = allocator;
    }

    void init() noexcept
    {
        id_         = BlockID{};
        references_ = 0;
        state_      = READ;
        block_      = nullptr;
        allocator_  = nullptr;
    }
};

}
