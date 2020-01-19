
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

#include <memoria/core/types.hpp>
#include <memoria/core/strings/strings.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/memory.hpp>

#include <memoria/api/common/ctr_api.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/logs.hpp>

#include <memoria/profiles/common/common.hpp>

#include <memoria/filesystem/path.hpp>



namespace memoria {
    

enum class SnapshotStatus {ACTIVE, COMMITTED, DROPPED, DATA_LOCKED};

template <typename SnapshotID>
class SnapshotMetadata {
    SnapshotID parent_id_;
    SnapshotID snapshot_id_;
    std::vector<SnapshotID> children_;
    U8String description_;
	SnapshotStatus status_;
public:
    SnapshotMetadata(const SnapshotID& parent_id, const SnapshotID& snapshot_id, const std::vector<SnapshotID>& children, U8StringRef description, SnapshotStatus status):
		parent_id_(parent_id),
		snapshot_id_(snapshot_id),
		children_(children),
		description_(description),
		status_(status)
	{}

    const SnapshotID& parent_id() const             {return parent_id_;}
    const SnapshotID& snapshot_id() const           {return snapshot_id_;}
    const std::vector<SnapshotID>& children() const {return children_;}
    const U8String& description() const            {return description_;}
    SnapshotStatus status() const                   {return status_;}
};


class ContainerMemoryStat {
    UUID ctr_name_;
    U8String ctr_type_name_;

    uint64_t total_leaf_blocks_;
    uint64_t total_branch_blocks_;

    uint64_t total_leaf_size_;
    uint64_t total_branch_size_;

    uint64_t total_size_;

public:
    ContainerMemoryStat(
            const UUID& ctr_name, U8String ctr_type_name, uint64_t total_leaf_blocks, uint64_t total_branch_blocks,
            uint64_t total_leaf_size, uint64_t total_branch_size, uint64_t total_size
    ):
        ctr_name_(ctr_name),
        ctr_type_name_(ctr_type_name),
        total_leaf_blocks_(total_leaf_blocks), total_branch_blocks_(total_branch_blocks),
        total_leaf_size_(total_leaf_size), total_branch_size_(total_branch_size),
        total_size_(total_size)
    {}

    const UUID& ctr_name() const {return ctr_name_;}
    const U8String& ctr_type_name() const {return ctr_type_name_;}

    uint64_t total_leaf_blocks() const {return total_leaf_blocks_;}
    uint64_t total_branch_blocks() const {return total_branch_blocks_;}

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
