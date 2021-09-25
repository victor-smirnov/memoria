
// Copyright 2020 Victor Smirnov
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

#include <memoria/api/store/store_api_common.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/checks.hpp>
#include <memoria/core/tools/any_id.hpp>


#include <functional>

namespace memoria {

template <typename Profile>
struct ISWMRStoreSnapshotBase: virtual IROStoreSnapshotCtrOps<Profile> {
    using SnapshotID = ApiProfileSnapshotID<Profile>;

    virtual SnapshotID snapshot_id() = 0;
    virtual void describe_to_cout() = 0;

    virtual bool is_transient() = 0;
    virtual bool is_system_snapshot() = 0;

    virtual LDDocumentView metadata() = 0;
};

template <typename Profile>
struct ISWMRStoreWritableSnapshot: virtual ISWMRStoreSnapshotBase<Profile>, virtual IROStoreWritableSnapshotCtrOps<Profile> {
    using SnapshotID = ApiProfileSnapshotID<Profile>;

    virtual void set_transient(bool transient) = 0;

    virtual void prepare(ConsistencyPoint cp = ConsistencyPoint::AUTO) = 0;
    virtual void rollback() = 0;

    virtual bool remove_snapshot(const SnapshotID& snapshot_id) = 0;
    virtual bool remove_branch(U8StringView branch_name)  = 0;
};

template <typename Profile>
struct ISWMRStoreReadOnlySnapshot: virtual ISWMRStoreSnapshotBase<Profile> {
    virtual void drop() = 0;
};

template <typename Profile>
struct ISWMRStoreHistoryView {
    using SnapshotID = ApiProfileSnapshotID<Profile>;
    using ReadOnlySnapshotPtr = SharedPtr<ISWMRStoreReadOnlySnapshot<Profile>>;

    virtual ~ISWMRStoreHistoryView() noexcept = default;

    virtual Optional<bool> is_transient(const SnapshotID& snapshot_id) = 0;
    virtual Optional<bool> is_system_snapshot(const SnapshotID& snapshot_id) = 0;

    virtual Optional<SnapshotID> parent(const SnapshotID& snapshot_id) = 0;
    virtual Optional<std::vector<SnapshotID>> children(const SnapshotID& snapshot_id) = 0;
    virtual Optional<std::vector<SnapshotID>> snapshots(U8StringView branch) = 0;
    virtual Optional<SnapshotID> branch_head(U8StringView name) = 0;
    virtual std::vector<U8String> branch_names() = 0;
};



template <typename Profile>
struct SWMRStoreGraphVisitor {

    enum class CtrType {
        ALLOCATOR, HISTORY, BLOCKMAP, DIRECTORY, DATA
    };

    using SnapshotID   = ApiProfileSnapshotID<Profile>;
    using SequenceID = uint64_t;

    using CtrPtrT   = CtrSharedPtr<CtrReferenceable<Profile>>;
    using BlockPtrT = CtrBlockPtr<Profile>;

    virtual ~SWMRStoreGraphVisitor() noexcept = default;

    virtual void start_graph() = 0;
    virtual void end_graph() = 0;

    virtual void start_snapshot(const SnapshotID&, const SequenceID&) = 0;
    virtual void end_snapshot() = 0;

    virtual void start_ctr(CtrPtrT, bool updated, CtrType ctr_type) = 0;
    virtual void end_ctr()   = 0;

    virtual void start_block(BlockPtrT, bool updated, uint64_t counters) = 0;
    virtual void end_block() = 0;
};

template <typename Profile>
struct IBasicSWMRStore {

    using ReadOnlySnapshotPtr = SharedPtr<ISWMRStoreReadOnlySnapshot<Profile>>;
    using WritableSnapshotPtr = SharedPtr<ISWMRStoreWritableSnapshot<Profile>>;

    using SequenceID = uint64_t;

    virtual ~IBasicSWMRStore() noexcept = default;

    virtual ReadOnlySnapshotPtr open()  = 0;
    virtual WritableSnapshotPtr begin() = 0;

    virtual Optional<SequenceID> check(const CheckResultConsumerFn& consumer) = 0;

    virtual U8String describe() const = 0;
};


enum class FlushType {
    DEFAULT, FULL
};

template <typename Profile>
struct ISWMRStore: IBasicSWMRStore<Profile> {
    using Base = IBasicSWMRStore<Profile>;

    using typename Base::WritableSnapshotPtr;
    using typename Base::ReadOnlySnapshotPtr;

    using HistoryPtr = SharedPtr<ISWMRStoreHistoryView<Profile>>;

    using SnapshotID = ApiProfileSnapshotID<Profile>;
    using SequenceID = uint64_t;

    virtual Optional<std::vector<SnapshotID>> snapshots(U8StringView branch) = 0;

    virtual uint64_t cp_allocation_threshold() const = 0;
    virtual uint64_t set_cp_allocation_threshold(uint64_t value) = 0;

    virtual uint64_t cp_snapshot_threshold() const  = 0;
    virtual uint64_t set_cp_snapshot_threshold(uint64_t value) = 0;

    virtual int64_t cp_timeout() const = 0;
    virtual int64_t set_cp_timeout(int64_t value) = 0;

    virtual Optional<SnapshotID> parent(const SnapshotID& snapshot_id) = 0;
    virtual Optional<std::vector<SnapshotID>> children(const SnapshotID& snapshot_id) = 0;
    virtual Optional<bool> is_transient(const SnapshotID& snapshot_id) = 0;
    virtual Optional<bool> is_system_snapshot(const SnapshotID& snapshot_id) = 0;

    using Base::open;
    using Base::begin;

    virtual ReadOnlySnapshotPtr flush(FlushType ft = FlushType::DEFAULT) = 0;

    virtual std::vector<U8String> branches() = 0;
    virtual ReadOnlySnapshotPtr open(U8StringView branch)  = 0;
    virtual ReadOnlySnapshotPtr open(const SnapshotID& snapshot_id, bool open_transient_snapshots = false) = 0;

    virtual WritableSnapshotPtr begin(U8StringView branch) = 0;

    virtual WritableSnapshotPtr branch_from(U8StringView source, U8StringView branch_name) = 0;
    virtual WritableSnapshotPtr branch_from(const SnapshotID& snapshot_id, U8StringView branch_name) = 0;

    virtual int64_t count_volatile_snapshots() = 0;

    virtual Optional<SequenceID> check(const CheckResultConsumerFn& consumer) = 0;

    virtual HistoryPtr history_view() = 0;

    virtual void close() = 0;

    virtual uint64_t count_refs(const AnyID& block_id) = 0;
    virtual void traverse(SWMRStoreGraphVisitor<Profile>& visitor) = 0;
};


class SWMRParams {
    Optional<uint64_t> file_size_; // in MB
    bool read_only_{false};
public:
    SWMRParams(uint64_t file_size) noexcept :
        file_size_(file_size)
    {}

    SWMRParams() noexcept :
        file_size_()
    {}

    SWMRParams& open_read_only(bool ro_mode = true) noexcept {
        read_only_ = ro_mode;
        return *this;
    }

    const Optional<uint64_t>& file_size() const noexcept {
        return file_size_;
    }

    bool is_read_only() const noexcept {
        return read_only_;
    }
};

std::unique_ptr<SWMRStoreGraphVisitor<CoreApiProfile<>>> create_graphviz_dot_visitor(U8StringView path);



SharedPtr<ISWMRStore<CoreApiProfile<>>> open_swmr_store(U8StringView path, const SWMRParams& params = SWMRParams());
SharedPtr<ISWMRStore<CoreApiProfile<>>> create_swmr_store(U8StringView path, const SWMRParams& params);
bool is_swmr_store(U8StringView path);

SharedPtr<ISWMRStore<CoreApiProfile<>>> open_lite_swmr_store(U8StringView path, const SWMRParams& params = SWMRParams());
SharedPtr<ISWMRStore<CoreApiProfile<>>> create_lite_swmr_store(U8StringView path, const SWMRParams& params);
bool is_lite_swmr_store(U8StringView path);

SharedPtr<ISWMRStore<CoreApiProfile<>>> open_lite_raw_swmr_store(Span<uint8_t> buffer);
SharedPtr<ISWMRStore<CoreApiProfile<>>> create_lite_raw_swmr_store(Span<uint8_t> buffer);
bool is_lite_raw_swmr_store(Span<uint8_t> buffer);


template <typename Profile>
struct ILMDBStore: IBasicSWMRStore<Profile> {
    using Base = IBasicSWMRStore<Profile>;

    using SnapshotID = int64_t;

    using typename Base::WritableSnapshotPtr;
    using typename Base::ReadOnlySnapshotPtr;

    virtual void set_async(bool is_async) = 0;
    virtual void copy_to(U8String path, bool with_compaction = true) = 0;
    virtual void flush(bool force = true) = 0;
};

SharedPtr<ILMDBStore<CoreApiProfile<>>> open_lmdb_store(U8StringView path);
SharedPtr<ILMDBStore<CoreApiProfile<>>> open_lmdb_store_readonly(U8StringView path);
SharedPtr<ILMDBStore<CoreApiProfile<>>> create_lmdb_store(U8StringView path, uint64_t store_size_mb);
bool is_lmdb_store(U8StringView path);



template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> create(
        SnpSharedPtr<ISWMRStoreWritableSnapshot<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IROStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return create(ptr, ctr_type_name, ctr_id);
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> create(
        SnpSharedPtr<ISWMRStoreWritableSnapshot<Profile>> alloc,
        const CtrName& ctr_type_name
){
    auto ptr = memoria_static_pointer_cast<IROStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return create(ptr, ctr_type_name);
}


template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find_or_create(
        SnpSharedPtr<ISWMRStoreWritableSnapshot<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IROStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return find_or_create(ptr, ctr_type_name, ctr_id);
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find(
        SnpSharedPtr<ISWMRStoreReadOnlySnapshot<Profile>> alloc,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IROStoreSnapshotCtrOps<Profile>>(alloc);
    return find<CtrName>(ptr, ctr_id);
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find(
        SnpSharedPtr<ISWMRStoreWritableSnapshot<Profile>> alloc,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IROStoreSnapshotCtrOps<Profile>>(alloc);
    return find<CtrName>(ptr, ctr_id);
}

}
