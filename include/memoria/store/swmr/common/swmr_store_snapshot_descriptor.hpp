
// Copyright 2020-2021 Victor Smirnov
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

#include <memoria/store/swmr/common/superblock.hpp>


#include <boost/intrusive/list.hpp>
#include <boost/intrusive_ptr.hpp>

#include <atomic>
#include <unordered_set>

namespace memoria {

template <typename Profile>
class HistoryTree;

template <typename Profile>
class SnapshotDescriptor: public boost::intrusive::list_base_hook<> {
    using Superblock = SWMRSuperblock<Profile>;
    using BlockID    = ProfileBlockID<Profile>;
    using SnapshotID   = typename Superblock::SnapshotID;
    using SequenceID   = typename Superblock::SequenceID;

public:
    using SharedPtrT = boost::intrusive_ptr<SnapshotDescriptor>;

private:
    using Children = std::unordered_set<SharedPtrT>;
    using HistoryTreeT = HistoryTree<Profile>;

public:
    using ChildIterator = typename Children::iterator;
    using ConstChildIterator = typename Children::const_iterator;

    using Allocations = ArenaBuffer<AllocationMetadata<ApiProfile<Profile>>>;

private:
    enum class Status {
        NEW, ATTACHED
    };

    template <typename TT>
    friend void intrusive_ptr_add_ref(SnapshotDescriptor<TT>*) ;

    template <typename TT>
    friend void intrusive_ptr_release(SnapshotDescriptor<TT>*) noexcept ;

    template <typename> friend class HistoryTree;

    // FIXME: Do we need the refcounter to be thread safe?
public:
    int32_t references_{};
private:

    HistoryTreeT* history_tree_;

    std::atomic<int32_t> uses_{};
    uint64_t superblock_ptr_;
    bool transient_{true};
    bool system_snapshot_{false};
    SequenceID sequence_id_{};
    SequenceID consistency_point_sequence_id_{};
    SnapshotID snapshot_id_{};

    SnapshotID allocation_map_snapshot_id_{};
    BlockID allocation_map_root_id_{};

    SnapshotDescriptor* parent_;
    Children children_;
    U8String branch_;

    Allocations postponed_deallocations_;
    size_t saved_deallocations_size_{};

    uint64_t allocated_basic_blocks_{};
    uint64_t allocated_basic_blocks_threshold_{};
    int64_t t_start_{};

    uint64_t snapshots_since_{};
    uint64_t snapshots_threshold_{};
    int64_t  cp_timeout_{};

    Status status_{Status::NEW};

private:
    SnapshotDescriptor(HistoryTreeT* history_tree, U8StringView branch) :
        history_tree_(history_tree),
        superblock_ptr_(),
        parent_(),
        branch_(branch)
    {}

    SnapshotDescriptor(
            HistoryTreeT* history_tree,
            uint64_t superblock_ptr,
            Superblock* superblock,
            U8StringView branch
    ) :
        history_tree_(history_tree),
        superblock_ptr_(superblock_ptr),
        parent_(),
        branch_(branch)
    {
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        snapshot_id_   = superblock->snapshot_id();
    }

public:

    bool is_new() const  {
        return status_ == Status::NEW;
    }

    bool is_attached() const  {
        return status_ == Status::ATTACHED;
    }

    void set_attached()  {
        status_ = Status::ATTACHED;
    }

    void set_new()  {
        status_ = Status::NEW;
    }

    void enqueue_for_eviction();

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

    const Allocations& postponed_deallocations() const  {
        return postponed_deallocations_;
    }

    Allocations& postponed_deallocations()  {
        return postponed_deallocations_;
    }

    void transfer_postponed_deallocations_to(SnapshotDescriptor* other)
    {
        if (other->postponed_deallocations_.size() == 0)
        {
            other->postponed_deallocations_ = std::move(postponed_deallocations_);
            // FIXME: is it necessary?
            other->saved_deallocations_size_ = saved_deallocations_size_;
        }
    }

    void clear_postponed_deallocations()
    {
        postponed_deallocations_.reset();
        saved_deallocations_size_ = 0;
    }

    void save_deallocations_size()  {
        saved_deallocations_size_ = postponed_deallocations_.size();
    }

    size_t saved_deallocations_size() const  {
        return saved_deallocations_size_;
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

    void detach_from_tree()
    {
        if (parent_) {
            parent_->children_.erase(this);
        }

        for (SnapshotDescriptor* chl: children_) {
            chl->parent_ = nullptr;
        }

        children_.clear();
    }

    const U8String& branch() const  {return branch_;}

    uint64_t superblock_ptr() const  {
        return superblock_ptr_;
    }

    SnapshotDescriptor* parent() const  {
        return parent_;
    }

    void set_parent(SnapshotDescriptor* parent)  {
        parent_ = parent;
    }

    void set_superblock(uint64_t ptr, Superblock* superblock)
    {
        superblock_ptr_ = ptr;
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        snapshot_id_   = superblock->snapshot_id();
    }

    void refresh_descriptor(Superblock* superblock)
    {
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        snapshot_id_   = superblock->snapshot_id();
    }

    bool is_transient() const  {
        return transient_;
    }

    void set_transient(bool transient)  {
        transient_ = transient;
        handle_descriptior_state();
    };

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
        return sequence_id_;
    }


    void ref()  {
        uses_.fetch_add(1);
    }

    void unref() noexcept {
        uses_.fetch_sub(1);
    }

    bool is_in_use()  {
        return uses_.load() > 0;
    }

    const SnapshotID& snapshot_id() const  {
        return snapshot_id_;
    }

    auto& children()  {
        return children_;
    }

    const auto& children() const  {
        return children_;
    }

    bool is_read_only_openable() const  {
        return (!transient_) || references_ > 2;
    }

    void unlink_from_eviction_queue();

private:

    void handle_descriptior_state()
    {
        auto val = references_;

        if(is_attached())
        {
            if (val == 2) {
                if (is_transient() && parent_ && !is_linked()) {
                    enqueue_for_eviction();
                }
            }
            else if (val == 0) {
                println("Zero refcounter for attached snapshot descriptor! {}", snapshot_id());
            }
        }
        else if(val == 0) { // is_new() == true
            if (is_linked()) {
                unlink_from_eviction_queue();
            }

            delete this;
        }
    }

};

template <typename Profile>
using SnapshotDescriptorsList = boost::intrusive::list<SnapshotDescriptor<Profile>>;

template <typename Profile>
inline void intrusive_ptr_add_ref(SnapshotDescriptor<Profile>* x) {
    ++x->references_;
}

template <typename Profile>
inline void intrusive_ptr_release(SnapshotDescriptor<Profile>* x) noexcept {
    --x->references_;
    x->handle_descriptior_state();
}

}
