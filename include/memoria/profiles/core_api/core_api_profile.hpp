
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

#include <memoria/profiles/common/common.hpp>
#include <memoria/profiles/common/block.hpp>

#include <memoria/core/container/store.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <unordered_set>

namespace memoria {

template <typename TT>
struct ApiProfileTraits<CoreApiProfile<TT>> {
    using SnapshotID    = UUID;
    using CtrID         = UUID;
    using CtrSizeT      = int64_t;
    using ApiProfileT   = CoreApiProfile<TT>;
    using ApiBlockID    = ApiBlockIDHolder<2>;

    static UUID make_random_ctr_id() {
        return UUID::make_random();
    }

    static UUID make_random_snapshot_id() {
        return UUID::make_random();
    }
};

template <typename TT>
struct CtrReferenceable<CoreApiProfile<TT>>: CtrReferenceableBase<CoreApiProfile<TT>> {
    using ApiProfile = CoreApiProfile<TT>;

    virtual CtrSharedPtr<CtrReferenceable<ApiProfile>> shared_self() noexcept = 0;

    virtual void traverse_ctr(void* node_handler) const = 0;
};

}
