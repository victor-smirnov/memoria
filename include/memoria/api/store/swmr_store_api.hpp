
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
#include <memoria/core/linked/linked.hpp>


#include <functional>

namespace memoria {

template <typename Profile>
struct ISWMRStoreCommitBase: virtual IROStoreSnapshotCtrOps<Profile> {
    using CommitID = ApiProfileSnapshotID<Profile>;

    virtual CommitID commit_id() = 0;
    virtual void describe_to_cout() = 0;

    virtual bool is_transient() = 0;
    virtual bool is_system_commit() = 0;
};

template <typename Profile>
struct ISWMRStoreWritableCommit: virtual ISWMRStoreCommitBase<Profile>, virtual IROStoreWritableSnapshotCtrOps<Profile> {
    virtual void set_transient(bool transient) = 0;
};

template <typename Profile>
struct ISWMRStoreReadOnlyCommit: virtual ISWMRStoreCommitBase<Profile> {
    virtual void drop() = 0;
};

template <typename Profile>
struct ISWMRStoreHistoryView {
    using CommitID = ApiProfileSnapshotID<Profile>;
    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;

    virtual ~ISWMRStoreHistoryView() noexcept = default;

    virtual Optional<bool> is_transient(const CommitID& commit_id) = 0;
    virtual Optional<bool> is_system_commit(const CommitID& commit_id) = 0;

    virtual Optional<CommitID> parent(const CommitID& commit_id) = 0;
    virtual Optional<std::vector<CommitID>> children(const CommitID& commit_id) = 0;
    virtual Optional<std::vector<CommitID>> commits(U8StringView branch) = 0;
    virtual Optional<CommitID> branch_head(U8StringView name) = 0;
    virtual std::vector<U8String> branch_names() = 0;
};



template <typename Profile>
struct SWMRStoreGraphVisitor {

    enum class CtrType {
        ALLOCATOR, HISTORY, BLOCKMAP, DIRECTORY, DATA
    };

    using CommitID   = ApiProfileSnapshotID<Profile>;
    using SequenceID = uint64_t;

    using CtrPtrT   = CtrSharedPtr<CtrReferenceable<Profile>>;
    using BlockPtrT = CtrBlockPtr<Profile>;

    virtual ~SWMRStoreGraphVisitor() noexcept = default;

    virtual void start_graph() = 0;
    virtual void end_graph() = 0;

    virtual void start_commit(const CommitID&, const SequenceID&) = 0;
    virtual void end_commit() = 0;

    virtual void start_ctr(CtrPtrT, bool updated, CtrType ctr_type) = 0;
    virtual void end_ctr()   = 0;

    virtual void start_block(BlockPtrT, bool updated, uint64_t counters) = 0;
    virtual void end_block() = 0;
};


using StoreCheckCallbackFn = std::function<void (LDDocument&)>;

template <typename Profile>
struct IBasicSWMRStore {

    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;
    using WritableCommitPtr = SharedPtr<ISWMRStoreWritableCommit<Profile>>;

    using SequenceID = uint64_t;

    virtual ~IBasicSWMRStore() noexcept = default;

    virtual ReadOnlyCommitPtr open()  = 0;
    virtual WritableCommitPtr begin() = 0;

    virtual ReadOnlyCommitPtr flush() = 0;

    virtual Optional<SequenceID> check(StoreCheckCallbackFn callback) = 0;

    virtual U8String describe() const = 0;

    virtual U8String to_string(const ApiProfileBlockID<Profile>&) = 0;
};


template <typename Profile>
struct ISWMRStore: IBasicSWMRStore<Profile> {
    using Base = IBasicSWMRStore<Profile>;

    using typename Base::WritableCommitPtr;
    using typename Base::ReadOnlyCommitPtr;

    using HistoryPtr = SharedPtr<ISWMRStoreHistoryView<Profile>>;

    using CommitID = ApiProfileSnapshotID<Profile>;
    using SequenceID = uint64_t;

    virtual Optional<std::vector<CommitID>> commits(U8StringView branch) = 0;

    virtual Optional<CommitID> parent(const CommitID& commit_id) = 0;
    virtual Optional<std::vector<CommitID>> children(const CommitID& commit_id) = 0;
    virtual Optional<bool> is_transient(const CommitID& commit_id) = 0;
    virtual Optional<bool> is_system_commit(const CommitID& commit_id) = 0;

    using Base::open;
    using Base::begin;

    virtual std::vector<U8String> branches() = 0;
    virtual ReadOnlyCommitPtr open(U8StringView branch)  = 0;
    virtual ReadOnlyCommitPtr open(const CommitID& commit_id, bool open_transient_commits = false) = 0;

    virtual WritableCommitPtr begin(U8StringView branch) = 0;

    virtual WritableCommitPtr branch_from(U8StringView source, U8StringView branch_name) = 0;
    virtual WritableCommitPtr branch_from(const CommitID& commit_id, U8StringView branch_name) = 0;

    virtual Optional<CommitID> remove_branch(U8StringView branch_name)  = 0;
    virtual Optional<CommitID> remove_commit(const CommitID& commit_id) = 0;

    virtual bool can_rollback_last_consistency_point() noexcept = 0;
    virtual void rollback_last_consistency_point() = 0;
    virtual void rollback_volatile_commits() = 0;
    virtual int64_t count_volatile_commits() = 0;

    virtual Optional<SequenceID> check(const Optional<SequenceID>& from, StoreCheckCallbackFn callback) = 0;
    virtual Optional<SequenceID> check(StoreCheckCallbackFn callback) {
        return check(Optional<SequenceID>{}, callback);
    };

    virtual HistoryPtr history_view() = 0;

    virtual void close() = 0;

    virtual uint64_t count_refs(const ApiProfileBlockID<Profile>& block_id) = 0;
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

    using CommitID = int64_t;

    using typename Base::WritableCommitPtr;
    using typename Base::ReadOnlyCommitPtr;

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
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IROStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return create(ptr, ctr_type_name, ctr_id);
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> create(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const CtrName& ctr_type_name
){
    auto ptr = memoria_static_pointer_cast<IROStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return create(ptr, ctr_type_name);
}


template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find_or_create(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IROStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return find_or_create(ptr, ctr_type_name, ctr_id);
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find(
        SnpSharedPtr<ISWMRStoreReadOnlyCommit<Profile>> alloc,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IROStoreSnapshotCtrOps<Profile>>(alloc);
    return find<CtrName>(ptr, ctr_id);
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IROStoreSnapshotCtrOps<Profile>>(alloc);
    return find<CtrName>(ptr, ctr_id);
}

}
