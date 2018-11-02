
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/api/allocator/allocator_inmem_api_common.hpp>

#include <unordered_set>
#include <unordered_map>

namespace memoria {
namespace v1 {
namespace persistent_inmem {

namespace _ {
    using PageSet = std::unordered_set<const void*>;
}

class PersistentTreeStatVisitAccumulatingConsumer {

    _::PageSet& visited_pages_;

public:
    PersistentTreeStatVisitAccumulatingConsumer(_::PageSet& visits):
        visited_pages_(visits)
    {}

    template <typename PageT>
    bool process_ptree_leaf(const PageT* page)
    {
        bool proceed_next = visited_pages_.find(page) == visited_pages_.end();
        visited_pages_.insert(page);
        return proceed_next;
    }

    template <typename PageT>
    bool process_ptree_branch(const PageT* page)
    {
        bool proceed_next = visited_pages_.find(page) == visited_pages_.end();
        visited_pages_.insert(page);
        return proceed_next;
    }

    template <typename PageT>
    void process_data_page(const PageT* page)
    {
        visited_pages_.insert(page);
    }
};


struct CtrStat {
    uint64_t total_leaf_pages_{};
    uint64_t total_branch_pages_{};

    uint64_t total_leaf_size_{};
    uint64_t total_branch_size_{};

    uint64_t total_size_{};
};


template <typename Snapshot>
class SnapshotStatsCountingConsumer {

    Snapshot* current_snapshot_{};
    _::PageSet& visited_pages_;
    bool include_containers_;

    std::unordered_map<UUID, CtrStat> ctr_stat_{};

    uint64_t total_ptree_size_{};
    uint64_t total_data_size_{};



public:
    SnapshotStatsCountingConsumer(_::PageSet& visits, Snapshot* snp, bool include_containers):
        current_snapshot_(snp),
        visited_pages_(visits),
        include_containers_(include_containers)
    {}

    template <typename PageT>
    bool process_ptree_leaf(const PageT* page)
    {
        bool proceed_next = visited_pages_.find(page) == visited_pages_.end();
        if (proceed_next)
        {
            visited_pages_.insert(page);
            total_ptree_size_ += sizeof(PageT) / 1024;
        }

        return proceed_next;
    }

    template <typename PageT>
    bool process_ptree_branch(const PageT* page)
    {
        bool proceed_next = visited_pages_.find(page) == visited_pages_.end();
        if (proceed_next)
        {
            visited_pages_.insert(page);
            total_ptree_size_ += sizeof(PageT) / 1024;
        }

        return proceed_next;
    }

    template <typename PageT>
    void process_data_page(const PageT* page)
    {
        bool proceed_next = visited_pages_.find(page) == visited_pages_.end();
        if (proceed_next)
        {
            if (include_containers_)
            {
                CtrPageDescription descr = current_snapshot_->describe_page(page->id());

                CtrStat& stat = ctr_stat_[descr.ctr_name()];

                if (descr.is_branch())
                {
                    stat.total_branch_pages_ ++;
                    stat.total_branch_size_ += descr.size() / 1024;
                }
                else {
                    stat.total_leaf_pages_ ++;
                    stat.total_leaf_size_ += descr.size() / 1024;
                }

                total_data_size_ += descr.size() / 1024;
            }
            else {
                total_data_size_ += page->page_size() / 1024;
            }

            visited_pages_.insert(page);
        }
    }


    SharedPtr<SnapshotMemoryStat> finish()
    {
        SharedPtr<SnapshotMemoryStat> snp_stat = MakeShared<SnapshotMemoryStat>(
                current_snapshot_->uuid(),
                total_ptree_size_,
                total_data_size_,
                total_ptree_size_ + total_data_size_
        );

        for (const auto& ctr_stat_item: ctr_stat_)
        {
            UUID ctr_name = ctr_stat_item.first;
            const CtrStat& stat = ctr_stat_item.second;

            SharedPtr<ContainerMemoryStat> ctr_mem_stat = MakeShared<ContainerMemoryStat>(
                    ctr_name,
                    current_snapshot_->ctr_type_name(ctr_name),
                    stat.total_leaf_pages_,
                    stat.total_branch_pages_,
                    stat.total_leaf_size_,
                    stat.total_branch_size_,
                    stat.total_leaf_size_ + stat.total_branch_size_
            );

            snp_stat->add_container_stat(ctr_mem_stat);
        }

        return snp_stat;
    }
};



}}}
