
// Copyright 2020-2021 Victor Smirnov
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

#include <memoria/store/swmr/common/swmr_store_commit_base.hpp>

namespace memoria {

template <typename Profile> class SWMRStoreBase;

struct InitStoreTag{};

template <typename Profile>
class SWMRStoreWritableCommitBase:
        public SWMRStoreCommitBase<Profile>,
        public ISWMRStoreWritableCommit<ApiProfile<Profile>>
{
protected:
    using Base = SWMRStoreCommitBase<Profile>;


    using typename Base::Store;
    using typename Base::CommitDescriptorT;

public:
    SWMRStoreWritableCommitBase(
            SharedPtr<Store> store,
            CommitDescriptorT* commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate
    ) noexcept :
        Base(store, commit_descriptor, refcounter_delegate)
    {}

    virtual VoidResult finish_commit_opening() noexcept = 0;
    virtual VoidResult finish_store_initialization() noexcept = 0;
};

}
