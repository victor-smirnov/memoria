
// Copyright 2011-2022 Victor Smirnov
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
        typename BlockIdT,
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
    mutable std::atomic<int64_t> references_;

    static_assert(std::is_trivially_copyable_v<decltype(references_)>, "");

    BlockIdT uid_;
    BlockIdT id_;
    SnapshotID snapshot_id_;

public:
    using FieldsList = TypeList<
                ConstValue<uint32_t, VERSION>,
                decltype(ctr_type_hash_),
                decltype(block_type_hash_),
                decltype(memory_block_size_),
                decltype(next_block_pos_),
                decltype(target_block_pos_),
                int64_t, //references
                decltype(uid_),
                decltype(id_),
                decltype(snapshot_id_)
    >;

    using BlockID = BlockIdT;

    AbstractPage() noexcept= default;

    AbstractPage(const BlockID &uid) noexcept:
        uid_(uid), id_(uid)
    {}



    const BlockID& uid() const noexcept {
        return uid_;
    }

    BlockID& uid() noexcept {
        return uid_;
    }

    const BlockID &id() const noexcept {
        return id_;
    }

    BlockID &id() noexcept {
        return id_;
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

    void ref_block(int64_t amount = 1) const noexcept {
        references_.fetch_add(amount, std::memory_order_relaxed);
    }

    bool unref_block() const noexcept
    {
        auto refs = references_.fetch_sub(1, std::memory_order_acq_rel);
        if (refs < 1) {
             terminate(format_u8("Internal error. Negative refcount detected for block {}", id_).data());
        }
        return refs == 1;
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->value("ID",                &id_);
        handler->value("UID",               &uid_);
        handler->value("SNAPSHOT_ID",       &snapshot_id_);
        handler->value("CTR_HASH",          &ctr_type_hash_);
        handler->value("PAGE_TYPE_HASH",    &block_type_hash_);
        handler->value("PAGE_SIZE",         &memory_block_size_);

        handler->value("NEXT_BLOCK_POS",    &next_block_pos_);
        handler->value("TARGET_BLOCK_POS",  &target_block_pos_);

        int64_t refs = references();
        handler->value("REFERENCES",        &refs);
    }


    template <template <typename T> class FieldFactory, typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<uint64_t>::serialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::serialize(buf, block_type_hash());
        FieldFactory<int32_t>::serialize(buf, memory_block_size());

        FieldFactory<uint64_t>::serialize(buf, next_block_pos());
        FieldFactory<uint64_t>::serialize(buf, target_block_pos());

        FieldFactory<BlockID>::serialize(buf, uid());
        FieldFactory<SnapshotID>::serialize(buf, snapshot_id());

        int64_t refs = references();
        FieldFactory<int64_t>::serialize(buf, refs);
    }

    template <template <typename T> class FieldFactory, typename SerializationData, typename IDResolver>
    void cow_serialize(SerializationData& buf, const IDResolver* id_resolver) const
    {
        FieldFactory<uint64_t>::serialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::serialize(buf, block_type_hash());
        FieldFactory<int32_t>::serialize(buf, memory_block_size());

        FieldFactory<uint64_t>::serialize(buf, next_block_pos());
        FieldFactory<uint64_t>::serialize(buf, target_block_pos());

        FieldFactory<BlockID>::serialize(buf, uid());
        FieldFactory<SnapshotID>::serialize(buf, snapshot_id());

        int64_t refs = references();
        FieldFactory<int64_t>::serialize(buf, refs);
    }

    template <typename IDResolver>
    void cow_resolve_ids(const IDResolver* id_resolver)
    {}

    template <template <typename T> class FieldFactory, typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<uint64_t>::deserialize(buf, ctr_type_hash());
        FieldFactory<uint64_t>::deserialize(buf, block_type_hash());
        FieldFactory<int32_t>::deserialize(buf,  memory_block_size());

        FieldFactory<uint64_t>::deserialize(buf, next_block_pos());
        FieldFactory<uint64_t>::deserialize(buf, target_block_pos());

        FieldFactory<BlockID>::deserialize(buf,    uid());
        FieldFactory<SnapshotID>::deserialize(buf, snapshot_id());

        int64_t refs;
        FieldFactory<int64_t>::deserialize(buf, refs);
        set_references(refs);
    }
};





template <typename StoreT, typename PageT, typename BlockID, typename Base = EmptyType>
class PageShared: public Base {
protected:
    BlockID id_;
    PageT*  block_;
    int64_t references_;

    StoreT* store_;

    bool mutable_{false};
    bool orphan_{false};
public:
    using BlockType = PageT;

    PageShared() noexcept {}

    PageShared(const BlockID& id, PageT* block, StoreT* store) noexcept:
        id_(id),
        block_(block),
        references_(0),
        store_(store)
    {}

    PageShared(const BlockID& id, PageT* block) noexcept:
        id_(id),
        block_(block),
        references_(0),
        store_(nullptr)
    {}

    virtual ~PageShared() noexcept = default;

    bool is_orphan() const noexcept {return orphan_;}
    void set_orphan(bool value) noexcept {
        orphan_ = value;
    }

    bool is_mutable() const noexcept {return mutable_;}
    void set_mutable(bool value) noexcept {
        mutable_ = value;
    }

    void assert_mutable() const {
        if (!mutable_) {
            MEMORIA_MAKE_GENERIC_ERROR("Block {} is immutable", id_).do_throw();
        }
    }

    template <typename BlockT1>
    const BlockT1* block() const noexcept {
        return ptr_cast<const BlockT1>(block_);
    }

    template <typename BlockT1>
    BlockT1* block() noexcept {
        return ptr_cast<BlockT1>(block_);
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

    template <typename BlockT1>
    operator BlockT1* () noexcept {
        return block<BlockT1>();
    }

    template <typename BlockT1>
    operator const BlockT1* () noexcept {
        return block<BlockT1>();
    }

    int64_t references() const noexcept {
        return references_;
    }

    const BlockID& id() const noexcept {
        return id_;
    }

    template <typename Page>
    void set_block(Page* block) {
        this->block_ = static_cast<PageT*>(block);
    }

    void ref() noexcept {
        references_++;
    }

    bool unref() noexcept {
        return --references_ == 0;
    }

    StoreT* store() noexcept {
        return store_;
    }

    void set_store(StoreT* store) noexcept {
        store_ = store;
    }
};

}
