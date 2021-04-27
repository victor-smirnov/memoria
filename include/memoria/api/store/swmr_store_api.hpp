
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
#include <memoria/core/linked/linked.hpp>

#include <functional>

namespace memoria {

template <typename Profile>
struct ISWMRStoreCommitBase: virtual IStoreSnapshotCtrOps<Profile> {
    using CommitID = ApiProfileSnapshotID<Profile>;

    virtual CommitID commit_id() = 0;
    virtual void describe_to_cout() = 0;
};

template <typename Profile>
struct ISWMRStoreWritableCommit: virtual ISWMRStoreCommitBase<Profile>, virtual IStoreWritableSnapshotCtrOps<Profile> {
    virtual void set_persistent(bool persistent) = 0;
    virtual bool is_persistent() = 0;
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

    virtual void check() = 0;

    virtual std::vector<CommitID> persistent_commits() = 0;

    virtual std::vector<CommitID> children(CommitID) = 0;
    virtual Optional<CommitID> parent(CommitID) = 0;
};

using StoreCheckCallbackFn = std::function<void (LDDocument&)>;

template <typename Profile>
struct IBasicSWMRStore {

    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;
    using WritableCommitPtr = SharedPtr<ISWMRStoreWritableCommit<Profile>>;

    using SequenceID = uint64_t;

    virtual ~IBasicSWMRStore() noexcept = default;

    virtual ReadOnlyCommitPtr open() = 0;
    virtual WritableCommitPtr begin() = 0;

    virtual void flush() = 0;

    virtual Optional<SequenceID> check(StoreCheckCallbackFn callback) = 0;
};


template <typename Profile>
struct ISWMRStore: IBasicSWMRStore<Profile> {
    using Base = IBasicSWMRStore<Profile>;

    using typename Base::WritableCommitPtr;
    using typename Base::ReadOnlyCommitPtr;

    using HistoryPtr = SharedPtr<ISWMRStoreHistoryView<Profile>>;

    using CommitID = ApiProfileSnapshotID<Profile>;
    using SequenceID = uint64_t;

    virtual std::vector<CommitID> persistent_commits() = 0;

    using Base::open;
    virtual ReadOnlyCommitPtr open(const CommitID& commit_id) = 0;

    virtual bool drop_persistent_commit(const CommitID& commit_id) = 0;

    virtual bool can_rollback_last_commit() noexcept = 0;
    virtual void rollback_last_commit() = 0;

    virtual Optional<SequenceID> check(const Optional<SequenceID>& from, StoreCheckCallbackFn callback) = 0;
    virtual Optional<SequenceID> check(StoreCheckCallbackFn callback) {
        return check(Optional<SequenceID>{}, callback);
    };

    virtual HistoryPtr history_view() = 0;

    virtual void close() = 0;
};

SharedPtr<ISWMRStore<CoreApiProfile<>>> open_swmr_store(U8StringView path);
SharedPtr<ISWMRStore<CoreApiProfile<>>> create_swmr_store(U8StringView path, uint64_t store_size_mb);

SharedPtr<ISWMRStore<CoreApiProfile<>>> open_lite_swmr_store(U8StringView path);
SharedPtr<ISWMRStore<CoreApiProfile<>>> create_lite_swmr_store(U8StringView path, uint64_t store_size_mb);

SharedPtr<ISWMRStore<CoreApiProfile<>>> open_lite_raw_swmr_store(Span<uint8_t> buffer);
SharedPtr<ISWMRStore<CoreApiProfile<>>> create_lite_raw_swmr_store(Span<uint8_t> buffer);

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




template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> create(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return create(ptr, ctr_type_name, ctr_id);
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> create(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const CtrName& ctr_type_name
){
    auto ptr = memoria_static_pointer_cast<IStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return create(ptr, ctr_type_name);
}


template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find_or_create(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return find_or_create(ptr, ctr_type_name, ctr_id);
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find(
        SnpSharedPtr<ISWMRStoreReadOnlyCommit<Profile>> alloc,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IStoreSnapshotCtrOps<Profile>>(alloc);
    return find<CtrName>(ptr, ctr_id);
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const ApiProfileCtrID<Profile>& ctr_id
){
    auto ptr = memoria_static_pointer_cast<IStoreSnapshotCtrOps<Profile>>(alloc);
    return find<CtrName>(ptr, ctr_id);
}

}
