
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

namespace memoria {

template <typename Profile>
struct ISWMRStoreCommitBase: IStoreSnapshotCtrOps<Profile> {
    using CommitID = int64_t;

    virtual CommitID commit_id() noexcept = 0;
};

template <typename Profile>
struct ISWMRStoreWritableCommit: ISWMRStoreCommitBase<Profile>, IStoreWritableSnapshotCtrOps<Profile> {
    virtual VoidResult commit() noexcept = 0;
    virtual VoidResult rollback() noexcept = 0;

    virtual VoidResult set_persistent(bool persistent) noexcept = 0;
    virtual bool is_persistent() noexcept = 0;
};

template <typename Profile>
struct ISWMRStoreReadOnlyCommit: ISWMRStoreCommitBase<Profile> {
    virtual VoidResult drop() noexcept = 0;
};

template <typename Profile>
struct ISWMRStore {
    using WritableCommitPtr = SharedPtr<ISWMRStoreWritableCommit<Profile>>;
    using ReadOnlyCommitPtr = SharedPtr<ISWMRStoreReadOnlyCommit<Profile>>;
    using CommitID = int64_t;

    virtual ~ISWMRStore() noexcept {}

    virtual Result<std::vector<CommitID>> persistent_commits() noexcept = 0;

    virtual Result<ReadOnlyCommitPtr> open(CommitID commit_id) noexcept = 0;
    virtual Result<ReadOnlyCommitPtr> open() noexcept = 0;
    virtual BoolResult drop_persistent_commit(CommitID commit_id) noexcept = 0;

    virtual VoidResult rollback_last_commit() noexcept = 0;
    virtual Result<WritableCommitPtr> begin() noexcept = 0;

    virtual VoidResult close() noexcept = 0;
};



Result<SharedPtr<ISWMRStore<MemoryCoWProfile<>>>> open_mapped_swmr_store(U8StringView path);
Result<SharedPtr<ISWMRStore<MemoryCoWProfile<>>>> create_mapped_swmr_store(U8StringView path, uint64_t store_size_mb);


}
