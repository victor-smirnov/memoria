
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
    using CommitID = int64_t;

    virtual CommitID commit_id() noexcept = 0;
    virtual VoidResult describe_to_cout() noexcept = 0;
};

template <typename Profile>
struct ISWMRStoreWritableCommit: virtual ISWMRStoreCommitBase<Profile>, virtual IStoreWritableSnapshotCtrOps<Profile> {
    //virtual VoidResult commit() noexcept = 0;

    virtual VoidResult set_persistent(bool persistent) noexcept = 0;
    virtual bool is_persistent() noexcept = 0;
};

template <typename Profile>
struct ISWMRStoreReadOnlyCommit: virtual ISWMRStoreCommitBase<Profile> {
    virtual VoidResult drop() noexcept = 0;
};

template <typename Profile>
struct ISWMRStoreHistoryView {
    using CommitID = int64_t;
    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;

    virtual ~ISWMRStoreHistoryView() noexcept {}

    virtual VoidResult check() noexcept = 0;

    virtual Result<std::vector<CommitID>> persistent_commits() noexcept = 0;

    virtual Result<std::vector<CommitID>> children(CommitID) noexcept = 0;
    virtual Result<Optional<CommitID>> parent(CommitID) noexcept = 0;
};

using StoreCheckCallbackFn = std::function<VoidResult(LDDocument&)>;

template <typename Profile>
struct ISWMRStore {
    using WritableCommitPtr = SharedPtr<ISWMRStoreWritableCommit<Profile>>;
    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;

    using HistoryPtr = SharedPtr<ISWMRStoreHistoryView<Profile>>;

    using CommitID = int64_t;
    using SequenceID = uint64_t;

    virtual ~ISWMRStore() noexcept {}

    virtual Result<std::vector<CommitID>> persistent_commits() noexcept = 0;

    virtual Result<ReadOnlyCommitPtr> open(CommitID commit_id) noexcept = 0;
    virtual Result<ReadOnlyCommitPtr> open() noexcept = 0;
    virtual BoolResult drop_persistent_commit(CommitID commit_id) noexcept = 0;

    virtual VoidResult rollback_last_commit() noexcept = 0;
    virtual Result<WritableCommitPtr> begin() noexcept = 0;

    virtual VoidResult flush() noexcept = 0;
    virtual VoidResult close() noexcept = 0;

    virtual Result<Optional<SequenceID>> check(const Optional<SequenceID>& from, StoreCheckCallbackFn callback) noexcept = 0;
    virtual Result<Optional<SequenceID>> check(StoreCheckCallbackFn callback) noexcept {
        return check(Optional<SequenceID>{}, callback);
    };

    virtual Result<HistoryPtr> history_view() noexcept = 0;
};


Result<SharedPtr<ISWMRStore<MemoryCoWProfile<>>>> open_mapped_swmr_store(U8StringView path);
Result<SharedPtr<ISWMRStore<MemoryCoWProfile<>>>> create_mapped_swmr_store(U8StringView path, uint64_t store_size_mb);


template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> create(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ProfileCtrID<Profile>& ctr_id
) noexcept
{
    auto ptr = memoria_static_pointer_cast<IStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return create(ptr, ctr_type_name, ctr_id);
}

template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> create(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const CtrName& ctr_type_name
) noexcept
{
    auto ptr = memoria_static_pointer_cast<IStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return create(ptr, ctr_type_name);
}


template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> find_or_create(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ProfileCtrID<Profile>& ctr_id
) noexcept
{
    auto ptr = memoria_static_pointer_cast<IStoreWritableSnapshotCtrOps<Profile>>(alloc);
    return find_or_create(ptr, ctr_type_name, ctr_id);
}

template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> find(
        SnpSharedPtr<ISWMRStoreReadOnlyCommit<Profile>> alloc,
        const ProfileCtrID<Profile>& ctr_id
) noexcept
{
    auto ptr = memoria_static_pointer_cast<IStoreSnapshotCtrOps<Profile>>(alloc);
    return find<CtrName>(ptr, ctr_id);
}

template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> find(
        SnpSharedPtr<ISWMRStoreWritableCommit<Profile>> alloc,
        const ProfileCtrID<Profile>& ctr_id
) noexcept
{
    auto ptr = memoria_static_pointer_cast<IStoreSnapshotCtrOps<Profile>>(alloc);
    return find<CtrName>(ptr, ctr_id);
}


}
