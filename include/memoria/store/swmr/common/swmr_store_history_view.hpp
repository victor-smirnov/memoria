
// Copyright 2021 Victor Smirnov
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

#include <memoria/api/store/swmr_store_api.hpp>

#include <memoria/store/swmr/mapped/swmr_mapped_store_readonly_commit.hpp>

#include <atomic>
#include <memory>
#include <vector>

namespace memoria {

template <typename Profile> class SWMRStoreBase;

template <typename Profile>
class SWMRMappedStoreHistoryView: public ISWMRStoreHistoryView<ApiProfile<Profile>> {
    using Base = ISWMRStoreHistoryView<ApiProfile<Profile>>;
    using StorePtr  = SharedPtr<SWMRStoreBase<Profile>>;
    using CommitPtr = SharedPtr<SWMRStoreReadOnlyCommitBase<Profile>>;

    StorePtr store_;
    CommitPtr head_;

public:
    using typename Base::CommitID;

    SWMRMappedStoreHistoryView(StorePtr store, CommitPtr head):
        store_(store), head_(head)
    {}

    virtual void check() noexcept
    {
//        MEMORIA_TRY_VOID(store_->check_if_open());

//        auto res = head_->for_each_history_entry([&](auto commit_id, auto file_pos) -> VoidResult {
//            MEMORIA_TRY(cmt, store_->open(commit_id));

//            return cmt->check();

//            //return VoidResult::of();
//        });

//        return res;

//        return wrap_throwing([&](){
//            return VoidResult::of();
//        });
    }

    virtual std::vector<CommitID> persistent_commits() {
        return store_->persistent_commits();
    }

    virtual std::vector<CommitID> children(CommitID) {
        return std::vector<CommitID>{};
    }

    virtual Optional<CommitID> parent(CommitID) {
        return Optional<CommitID>{};
    }
};


}
