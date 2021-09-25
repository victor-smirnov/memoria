
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

#include <memoria/api/allocation_map/allocation_map_api.hpp>
#include <memoria/api/store/memory_store_api.hpp>

#include <memory>

namespace memoria {

template <typename Profile>
class LiteAllocationMap {

    using CtrSizeT = ApiProfileCtrSizeT<Profile>;

    AllocSharedPtr<IMemoryStore<Profile>> store_;
    SnpSharedPtr<IMemorySnapshot<Profile>> snp_;
    CtrSharedPtr<ICtrApi<AllocationMap, Profile>> ctr_;

public:
    LiteAllocationMap(CtrSizeT map_size)
    {
        store_ = create_memory_store();
        snp_ = store_->master()->branch();
        ctr_ = create<AllocationMap>(snp_, AllocationMap());

        ctr_->expand(map_size);
    }

    void append(const AllocationMetadata<Profile>& alc) {
        ctr_->mark_allocated(alc);
    }

    auto ctr() const {
        return ctr_;
    }

    void dump(CtrSizeT num = -1) const {
        ctr_->dump_leafs(num);
    }

    void close()
    {
        snp_->commit();
        ctr_.reset();
        snp_.reset();
        store_.reset();
    }
};


}
