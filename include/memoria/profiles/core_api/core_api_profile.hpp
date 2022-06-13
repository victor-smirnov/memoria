
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
#include <memoria/core/tools/uid_256.hpp>

#include <unordered_set>

namespace memoria {

template <typename TT>
struct ApiProfileTraits<CoreApiProfileT<TT>> {
    using SnapshotID    = UID256;
    using CtrID         = UID256;
    using CtrSizeT      = uint64_t;
    using ApiProfileT   = CoreApiProfileT<TT>;

    static CtrID make_random_ctr_id() {
        return CtrID::make_random();
    }

    static SnapshotID make_random_snapshot_id() {
        return SnapshotID::make_random();
    }
};

template <typename TT>
struct CtrReferenceable<CoreApiProfileT<TT>>: CtrReferenceableBase<CoreApiProfileT<TT>> {
    using ApiProfile = CoreApiProfileT<TT>;

    virtual CtrSharedPtr<CtrReferenceable<ApiProfile>> shared_self() noexcept = 0;

    virtual void traverse_ctr(void* node_handler) const {
        MEMORIA_MAKE_GENERIC_ERROR("Read-only container").do_throw();
    }
};

}
