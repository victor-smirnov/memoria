
// Copyright 2018-2020 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/api/store/memory_store_common.hpp>

#include <unordered_set>
#include <unordered_map>

namespace memoria {
namespace store {
namespace memory_cow {

namespace _ {
    using BlockSet = std::unordered_set<const void*>;
}

class PersistentTreeStatVisitAccumulatingConsumer {

    _::BlockSet& visited_blocks_;

public:
    PersistentTreeStatVisitAccumulatingConsumer(_::BlockSet& visits):
        visited_blocks_(visits)
    {}

    template <typename BlockT>
    bool process_ptree_leaf(const BlockT* block)
    {
        bool proceed_next = visited_blocks_.find(block) == visited_blocks_.end();
        visited_blocks_.insert(block);
        return proceed_next;
    }

    template <typename BlockT>
    bool process_ptree_branch(const BlockT* block)
    {
        bool proceed_next = visited_blocks_.find(block) == visited_blocks_.end();
        visited_blocks_.insert(block);
        return proceed_next;
    }

    template <typename BlockT>
    void process_data_block(const BlockT* block)
    {
        visited_blocks_.insert(block);
    }
};


struct CtrStat {
    uint64_t total_leaf_blocks_{};
    uint64_t total_branch_blocks_{};

    uint64_t total_leaf_size_{};
    uint64_t total_branch_size_{};

    uint64_t total_size_{};
};


template <typename Snapshot>
class SnapshotStatsCountingConsumer {

    Snapshot* current_snapshot_{};
    _::BlockSet& visited_blocks_;

    using CtrID = typename Snapshot::CtrID;
    using Profile = typename Snapshot::ProfileT;
    using ApiProfileT = ApiProfile<Profile>;

    uint64_t total_ptree_size_{};
    uint64_t total_data_size_{};



public:
    SnapshotStatsCountingConsumer(_::BlockSet& visits, Snapshot* snp):
        current_snapshot_(snp),
        visited_blocks_(visits)
    {}

    template <typename BlockT>
    bool process_ptree_leaf(const BlockT* block)
    {
        bool proceed_next = visited_blocks_.find(block) == visited_blocks_.end();
        if (proceed_next)
        {
            visited_blocks_.insert(block);
            total_ptree_size_ += sizeof(BlockT) / 1024;
        }

        return proceed_next;
    }

    template <typename BlockT>
    bool process_ptree_branch(const BlockT* block)
    {
        bool proceed_next = visited_blocks_.find(block) == visited_blocks_.end();
        if (proceed_next)
        {
            visited_blocks_.insert(block);
            total_ptree_size_ += sizeof(BlockT) / 1024;
        }

        return proceed_next;
    }

    template <typename BlockT>
    void process_data_block(const BlockT* block)
    {
        bool proceed_next = visited_blocks_.find(block) == visited_blocks_.end();
        if (proceed_next)
        {            
            total_data_size_ += block->memory_block_size() / 1024;
            visited_blocks_.insert(block);
        }
    }


    SharedPtr<SnapshotMemoryStat<ApiProfileT>> finish()
    {
        SharedPtr<SnapshotMemoryStat<ApiProfileT>> snp_stat = MakeShared<SnapshotMemoryStat<ApiProfileT>>(
                current_snapshot_->uuid(),
                total_ptree_size_,
                total_data_size_,
                total_ptree_size_ + total_data_size_
        );

        return snp_stat;
    }
};



}}}
