
// Copyright 2020-2025 Victor Smirnov
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
#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/time.hpp>

#include <memoria/profiles/common/common.hpp>

#include <memoria/store/oltp/oltp_superblock.hpp>

#include <boost/intrusive/list.hpp>
#include <boost/shared_ptr.hpp>

#include <atomic>
#include <unordered_set>

namespace memoria {

template <typename Profile>
class OLTPHistory;

template <typename Profile>
class OLTPSnapshotDescriptor {

    using Superblock = OLTPSuperblock<Profile>;
    using BlockID    = ProfileBlockID<Profile>;
    using SnapshotID   = typename Superblock::SnapshotID;
    using SequenceID   = typename Superblock::SequenceID;

public:
    using SharedPtrT = boost::local_shared_ptr<OLTPSnapshotDescriptor>;

private:

    SharedPtrT prev_;
    SharedPtrT next_;

    BlockID superblock_id_;

    bool transient_{true};
    bool system_snapshot_{false};

    bool full_txn_{true};


    SequenceID sequence_id_{};
    SequenceID consistency_point_sequence_id_{};
    SnapshotID snapshot_id_{};

    SnapshotID allocation_map_snapshot_id_{};
    BlockID allocation_map_root_id_{};

    uint64_t allocated_basic_blocks_{};
    uint64_t allocated_basic_blocks_threshold_{};
    int64_t  t_start_{};

    uint64_t snapshots_since_{};
    uint64_t snapshots_threshold_{};
    int64_t  cp_timeout_{};

    std::atomic<size_t> uses_{};

public:
    OLTPSnapshotDescriptor() :
        prev_(), next_(),
        superblock_id_()
    {}

    OLTPSnapshotDescriptor(
            SharedPtrT parent,
            BlockID superblock_id,
            const Superblock* superblock
    ) :        
        prev_(std::move(parent)), next_(),
        superblock_id_(superblock_id)
    {
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        snapshot_id_   = superblock->snapshot_id();
    }

    OLTPSnapshotDescriptor(
            const Superblock* superblock
    ) :
        superblock_id_(superblock->id())
    {
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        snapshot_id_   = superblock->snapshot_id();
    }

public:

    template <typename... Args>
    static auto make(Args&&... args) {
        return boost::make_local_shared<OLTPSnapshotDescriptor>(std::forward<Args>(args)...);
    }

    const SharedPtrT& prev() const {return prev_;}
    const SharedPtrT& next() const {return next_;}

    void set_next(SharedPtrT next) {
        next_ = std::move(next);
    }

    void set_prev(SharedPtrT prev) {
        prev_ = std::move(prev);
    }

    bool is_full_txn() const {
        return full_txn_;
    }

    void set_full_txn(bool val) {
        full_txn_ = val;
    }

    void remove_prev() {
        prev_.reset();
    }

    SharedPtrT self() {
        return this->shared_from_this();
    }

    uint64_t allocated_basic_blocks() const  {
        return allocated_basic_blocks_;
    }

    void set_allocated_basic_blocks_threshold(uint64_t val)  {
        allocated_basic_blocks_threshold_ = val;
    }

    void add_allocated(uint64_t value)  {
        allocated_basic_blocks_ += value;
    }

    void inc_snapshots()  {
        snapshots_since_++;
    }

    void set_snapshots_threshold(uint64_t val)  {
        snapshots_threshold_ = val;
    }

    void set_cp_timeout(int64_t val)  {
        cp_timeout_ = val;
    }

    void set_start_time(int64_t val)  {
        t_start_ = val;
    }

    bool should_make_consistency_point(const SharedPtrT& head)
    {
        return allocated_basic_blocks_ >= head->allocated_basic_blocks_threshold_ ||
                snapshots_since_ >= head->snapshots_threshold_ ||
                head->cp_timeout_ >= 0 ? getTimeInMillis() - t_start_ >= head->cp_timeout_ : false;
    }


    const BlockID& allocation_map_root_id() const  {
        return allocation_map_root_id_;
    }

    BlockID* allocation_map_root_id_ptr()  {
        return &allocation_map_root_id_;
    }

    void set_allocation_map_root_id(const BlockID& id){
        allocation_map_root_id_ = id;
    }

    const SnapshotID& allocation_map_snapshot_id() const  {
        return allocation_map_snapshot_id_;
    }

    void set_allocation_map_snapshot_id(const SnapshotID& id){
        allocation_map_snapshot_id_ = id;
    }


    BlockID superblock_id() const  {
        return superblock_id_;
    }


    void set_superblock(Superblock* superblock)
    {
        superblock_id_ = superblock->id();
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        snapshot_id_ = superblock->snapshot_id();
    }



    void refresh_descriptor(Superblock* superblock)
    {
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        snapshot_id_ = superblock->snapshot_id();
    }

    bool is_transient() const  {
        return transient_;
    }

    bool is_system_snapshot() const  {
        return system_snapshot_;
    }

    void set_system_snapshot(bool value)  {
        system_snapshot_ = value;
    };

    const SequenceID& sequence_id() const  {
        return sequence_id_;
    }

    const SequenceID& consistency_point_sequence_id() const  {
        return consistency_point_sequence_id_;
    }

    void ref() noexcept {
        uses_.fetch_add(1);
    }

    void unref() noexcept {
        uses_.fetch_sub(1);
    }

    bool is_in_use() {
        return uses_.load() > 0;
    }

    const SnapshotID& snapshot_id() const  {
        return snapshot_id_;
    }
};



}
