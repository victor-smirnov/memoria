
// Copyright 2017 Victor Smirnov
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
#include <memoria/v1/core/strings/strings.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/api/common/ctr_api.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/logs.hpp>

#include <memoria/v1/allocators/inmem/common/container_collection_cfg.hpp>

#include <memoria/v1/filesystem/path.hpp>



namespace memoria {
namespace v1 {
    

enum class SnapshotStatus {ACTIVE, COMMITTED, DROPPED, DATA_LOCKED};

template <typename TxnId>
class SnapshotMetadata {
	TxnId parent_id_;
	TxnId snapshot_id_;
	std::vector<TxnId> children_;
    U16String description_;
	SnapshotStatus status_;
public:
    SnapshotMetadata(const TxnId& parent_id, const TxnId& snapshot_id, const std::vector<TxnId>& children, U16StringRef description, SnapshotStatus status):
		parent_id_(parent_id),
		snapshot_id_(snapshot_id),
		children_(children),
		description_(description),
		status_(status)
	{}

    const TxnId& parent_id() const              {return parent_id_;}
    const TxnId& snapshot_id() const            {return snapshot_id_;}
    const std::vector<TxnId>& children() const 	{return children_;}
    const U16String& description() const        {return description_;}
    SnapshotStatus status() const               {return status_;}
};


class ContainerMemoryStat {
    UUID ctr_name_;
    U16String ctr_type_name_;

    uint64_t total_leaf_pages_;
    uint64_t total_branch_pages_;

    uint64_t total_leaf_size_;
    uint64_t total_branch_size_;

    uint64_t total_size_;

public:
    ContainerMemoryStat(
            const UUID& ctr_name, U16String ctr_type_name, uint64_t total_leaf_pages, uint64_t total_branch_pages,
            uint64_t total_leaf_size, uint64_t total_branch_size, uint64_t total_size
    ):
        ctr_name_(ctr_name),
        ctr_type_name_(ctr_type_name),
        total_leaf_pages_(total_leaf_pages), total_branch_pages_(total_branch_pages),
        total_leaf_size_(total_leaf_size), total_branch_size_(total_branch_size),
        total_size_(total_size)
    {}

    const UUID& ctr_name() const {return ctr_name_;}
    const U16String& ctr_type_name() const {return ctr_type_name_;}

    uint64_t total_leaf_pages() const {return total_leaf_pages_;}
    uint64_t total_branch_pages() const {return total_branch_pages_;}

    uint64_t total_leaf_size() const {return total_leaf_size_;}
    uint64_t total_branch_size() const {return total_branch_size_;}

    uint64_t total_size() const {return total_size_;}
};



class SnapshotMemoryStat {
    UUID snapshot_id_;

    uint64_t total_ptree_size_;
    uint64_t total_data_size_;
    uint64_t total_size_;

    using CtrMap = std::unordered_map<UUID, SharedPtr<ContainerMemoryStat>>;

    std::unordered_map<UUID, SharedPtr<ContainerMemoryStat>> containers_;
public:
    SnapshotMemoryStat(
            const UUID& snapshot_id, uint64_t total_ptree_size, uint64_t total_data_size, uint64_t total_size
    ):
        snapshot_id_(snapshot_id),
        total_ptree_size_(total_ptree_size),
        total_data_size_(total_data_size),
        total_size_(total_size)
    {}

    const UUID& snapshot_id() const {return snapshot_id_;}
    uint64_t total_size() const {return total_size_;}

    uint64_t total_ptree_size() const {return total_ptree_size_;}
    uint64_t total_data_size() const {return total_data_size_;}

    const CtrMap& containers() const {return containers_;}

    template <typename... Args>
    void add_container_stat(SharedPtr<ContainerMemoryStat> container_stat)
    {
        containers_[container_stat->ctr_name()] = container_stat;
    }
};



class AllocatorMemoryStat {
    uint64_t total_size_;

    using SnapshotMap = std::unordered_map<UUID, SharedPtr<SnapshotMemoryStat>>;
    SnapshotMap snapshots_;

public:
    AllocatorMemoryStat(): total_size_(0) {}

    AllocatorMemoryStat(uint64_t total_size): total_size_(total_size) {}
    const SnapshotMap& snapshots() const {return snapshots_;}

    uint64_t total_size() const {return total_size_;}

    template <typename... Args>
    void add_snapshot_stat(SharedPtr<SnapshotMemoryStat> snapshot_stat)
    {
        snapshots_[snapshot_stat->snapshot_id()] = snapshot_stat;
    }

    void compute_total_size()
    {
        total_size_ = 0;
        for (const auto& snp: snapshots_)
        {
            total_size_ += snp.second->total_size();
        }
    }
};

void print(std::ostream& out, const ContainerMemoryStat& stat);
void print(std::ostream& out, const SnapshotMemoryStat& stat, int ntabs = 0);
void print(std::ostream& out, const AllocatorMemoryStat& stat);


void print_json(std::ostream& out, const ContainerMemoryStat& stat);
void print_json(std::ostream& out, const SnapshotMemoryStat& stat);
void print_json(std::ostream& out, const AllocatorMemoryStat& stat);


}
}
