
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
#include <memoria/core/memory/memory.hpp>

#include <memoria/api/common/ctr_api.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/store.hpp>
#include <memoria/core/container/logs.hpp>

#include <memoria/profiles/common/common.hpp>

#include <memoria/filesystem/path.hpp>

#include <memoria/profiles/common/common.hpp>


namespace memoria {
    

enum class SnapshotStatus {ACTIVE, COMMITTED, DROPPED, DATA_LOCKED};

template <typename Profile>
class SnapshotMetadata {
    using SnapshotID = ApiProfileSnapshotID<Profile>;
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





template <typename Profile>
class SnapshotMemoryStat {
    using SnpID = ApiProfileSnapshotID<Profile>;

    SnpID snapshot_id_;

    uint64_t total_ptree_size_;
    uint64_t total_data_size_;
    uint64_t total_size_;

public:
    SnapshotMemoryStat(
            const SnpID& snapshot_id, uint64_t total_ptree_size, uint64_t total_data_size, uint64_t total_size
    ) noexcept :
        snapshot_id_(snapshot_id),
        total_ptree_size_(total_ptree_size),
        total_data_size_(total_data_size),
        total_size_(total_size)
    {}

    const SnpID& snapshot_id() const noexcept {return snapshot_id_;}
    uint64_t total_size() const noexcept {return total_size_;}

    uint64_t total_ptree_size() const noexcept {return total_ptree_size_;}
    uint64_t total_data_size() const noexcept {return total_data_size_;}
};


template <typename Profile>
class StoreMemoryStat {
    using SnpID = ApiProfileSnapshotID<Profile>;
    uint64_t total_size_;

    using SnapshotMap = std::unordered_map<SnpID, SharedPtr<SnapshotMemoryStat<Profile>>>;
    SnapshotMap snapshots_;

public:
    StoreMemoryStat() noexcept : total_size_(0) {}

    StoreMemoryStat(uint64_t total_size) noexcept: total_size_(total_size) {}
    const SnapshotMap& snapshots() const noexcept {return snapshots_;}

    uint64_t total_size() const noexcept {return total_size_;}

    template <typename... Args>
    void add_snapshot_stat(SharedPtr<SnapshotMemoryStat<Profile>> snapshot_stat) noexcept
    {
        snapshots_[snapshot_stat->snapshot_id()] = snapshot_stat;
    }

    void compute_total_size() noexcept
    {
        total_size_ = 0;
        for (const auto& snp: snapshots_)
        {
            total_size_ += snp.second->total_size();
        }
    }
};

template <typename Profile>
void print(std::ostream& out, const SnapshotMemoryStat<Profile>& stat, int ntabs = 0);

template <typename Profile>
void print(std::ostream& out, const StoreMemoryStat<Profile>& stat);

template <typename Profile>
void print_json(std::ostream& out, const SnapshotMemoryStat<Profile>& stat);

template <typename Profile>
void print_json(std::ostream& out, const StoreMemoryStat<Profile>& stat);

}
