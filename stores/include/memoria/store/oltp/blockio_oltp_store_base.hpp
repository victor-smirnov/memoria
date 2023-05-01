
// Copyright 2023 Victor Smirnov
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

#include <memoria/store/oltp/oltp_store_base.hpp>

#include <memoria/store/swmr/common/lite_allocation_map.hpp>

namespace memoria {

template <typename Profile>
class BlockIOOLTPStoreBase: public OLTPStoreBase<Profile> {
    using Base = OLTPStoreBase<Profile>;

protected:
    using typename Base::ApiProfileT;
    using Base::BASIC_BLOCK_SIZE;
    using Base::history_tree_;

    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;

    static constexpr size_t  HEADER_SIZE = BASIC_BLOCK_SIZE * 2;
    static constexpr int32_t ALLOCATION_MAP_LEVELS    = ICtrApi<AllocationMap, ApiProfileT>::LEVELS;
    static constexpr int32_t ALLOCATION_MAP_SIZE_STEP = ICtrApi<AllocationMap, ApiProfileT>::ALLOCATION_SIZE * BASIC_BLOCK_SIZE;
    static constexpr size_t  MB = 1024*1024;

    AllocationPool<ApiProfileT, ALLOCATION_MAP_LEVELS> allocation_pool_{128};

    using BlockIOPtr = std::shared_ptr<io::BlockIOProvider>;
    BlockIOPtr block_io_;

    std::unique_ptr<LiteAllocationMap<ApiProfileT>> allocations_;

public:
    BlockIOOLTPStoreBase(BlockIOPtr block_io):
        Base(),
        block_io_(block_io)
    {}

    const BlockIOPtr& block_io() const {return block_io_;}

    auto& allocation_pool() {
        return allocation_pool_;
    }

    void register_allocation(const AllocationMetadataT& alc)
    {
        if (allocations_) {
            allocations_->append(alc);
        }
    }

    std::unique_ptr<LiteAllocationMap<ApiProfileT>> make_lite_allocation_map()
    {
        auto ptr = do_open_readonly(history_tree_.head());
        return std::make_unique<LiteAllocationMap<ApiProfileT>>(ptr->allocation_map_size());
    }

    void do_check_allocations(const CheckResultConsumerFn& consumer)
    {
        auto ptr = do_open_readonly(history_tree_.head());
        ptr->add_system_blocks(*allocations_.get());
        ptr->check_allocation_map(*allocations_.get(), consumer);
    }


};

}
