
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
class CommitDescriptor: public boost::intrusive::list_base_hook<> {
    using Superblock = SWMRSuperblock<Profile>;
    using BlockID    = ProfileBlockID<Profile>;
    using CommitID   = typename Superblock::CommitID;
    using SequenceID   = typename Superblock::SequenceID;

public:
    using SharedPtrT = boost::intrusive_ptr<CommitDescriptor>;

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
    friend void intrusive_ptr_add_ref(CommitDescriptor<TT>*) noexcept;

    template <typename TT>
    friend void intrusive_ptr_release(CommitDescriptor<TT>*) noexcept;

    template <typename> friend class HistoryTree;

    // FIXME: Do we need the refcounter to be thread safe?
public:
    int32_t references_{};
private:

    HistoryTreeT* history_tree_;

    std::atomic<int32_t> uses_{};
    uint64_t superblock_ptr_;
    bool transient_{true};
    bool system_commit_{false};
    SequenceID sequence_id_{};
    SequenceID consistency_point_sequence_id_{};
    CommitID commit_id_{};

    CommitID allocation_map_commit_id_{};
    BlockID allocation_map_root_id_{};

    CommitDescriptor* parent_;
    Children children_;
    U8String branch_;

    Allocations postponed_deallocations_;
    size_t saved_deallocations_size_{};

    uint64_t allocated_basic_blocks_{};
    uint64_t allocated_basic_blocks_threshold_{};
    int64_t t_start_{};

    uint64_t commits_since_{};
    uint64_t commits_threshold_{};
    int64_t cp_timeout_{};

    Status status_{Status::NEW};

private:
    CommitDescriptor(HistoryTreeT* history_tree, U8StringView branch) noexcept:
        history_tree_(history_tree),
        superblock_ptr_(),
        parent_(),
        branch_(branch)
    {}

    CommitDescriptor(
            HistoryTreeT* history_tree,
            uint64_t superblock_ptr,
            Superblock* superblock,
            U8StringView branch
    ) noexcept:
        history_tree_(history_tree),
        superblock_ptr_(superblock_ptr),
        parent_(),
        branch_(branch)
    {
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        commit_id_   = superblock->commit_id();
    }

public:

    bool is_new() const noexcept {
        return status_ == Status::NEW;
    }

    bool is_attached() const noexcept {
        return status_ == Status::ATTACHED;
    }

    void set_attached() noexcept {
        status_ = Status::ATTACHED;
    }

    void set_new() noexcept {
        status_ = Status::NEW;
    }

    void enque_for_eviction();

    uint64_t allocated_basic_blocks() const noexcept {
        return allocated_basic_blocks_;
    }

    void set_allocated_basic_blocks_threshold(uint64_t val) noexcept {
        allocated_basic_blocks_threshold_ = val;
    }

    void add_allocated(uint64_t value) noexcept {
        allocated_basic_blocks_ += value;
    }

    void inc_commits() noexcept {
        commits_since_++;
    }

    void set_commits_threshold(uint64_t val) noexcept {
        commits_threshold_ = val;
    }

    void set_cp_timeout(int64_t val) noexcept {
        cp_timeout_ = val;
    }

    void set_start_time(int64_t val) noexcept {
        t_start_ = val;
    }

    bool should_make_consistency_point(const SharedPtrT& head) noexcept
    {
        return allocated_basic_blocks_ >= head->allocated_basic_blocks_threshold_ ||
                commits_since_ >= head->commits_threshold_ ||
                head->cp_timeout_ >= 0 ? getTimeInMillis() - t_start_ >= head->cp_timeout_ : false;
    }

    const Allocations& postponed_deallocations() const noexcept {
        return postponed_deallocations_;
    }

    Allocations& postponed_deallocations() noexcept {
        return postponed_deallocations_;
    }

    void transfer_postponed_deallocations_to(CommitDescriptor* other)
    {
        if (other->postponed_deallocations_.size() == 0)
        {
            other->postponed_deallocations_ = std::move(postponed_deallocations_);
            // FIXME: is it necessary?
            other->saved_deallocations_size_ = saved_deallocations_size_;
        }
    }

    void clear_postponed_deallocations() noexcept
    {
        postponed_deallocations_.reset();
        saved_deallocations_size_ = 0;
    }

    void save_deallocations_size() noexcept {
        saved_deallocations_size_ = postponed_deallocations_.size();
    }

    size_t saved_deallocations_size() const noexcept {
        return saved_deallocations_size_;
    }

    const BlockID& allocation_map_root_id() const noexcept {
        return allocation_map_root_id_;
    }

    BlockID* allocation_map_root_id_ptr() noexcept {
        return &allocation_map_root_id_;
    }

    void set_allocation_map_root_id(const BlockID& id){
        allocation_map_root_id_ = id;
    }

    const CommitID& allocation_map_commit_id() const noexcept {
        return allocation_map_commit_id_;
    }

    void set_allocation_map_commit_id(const CommitID& id){
        allocation_map_commit_id_ = id;
    }

    void detach_from_tree() noexcept
    {
        if (parent_) {
            parent_->children_.erase(this);
        }

        for (CommitDescriptor* chl: children_) {
            chl->parent_ = nullptr;
        }

        children_.clear();
    }

    const U8String& branch() const noexcept {return branch_;}

    uint64_t superblock_ptr() const noexcept {
        return superblock_ptr_;
    }

    CommitDescriptor* parent() const noexcept {
        return parent_;
    }

    void set_parent(CommitDescriptor* parent) noexcept {
        parent_ = parent;
    }

    void set_superblock(uint64_t ptr, Superblock* superblock) noexcept
    {
        superblock_ptr_ = ptr;
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        commit_id_   = superblock->commit_id();
    }

    void refresh_descriptor(Superblock* superblock) noexcept
    {
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        commit_id_   = superblock->commit_id();
    }

    bool is_transient() const noexcept {
        return transient_;
    }

    void set_transient(bool transient) noexcept {
        transient_ = transient;
    };

    bool is_system_commit() const noexcept {
        return system_commit_;
    }

    void set_system_commit(bool value) noexcept {
        system_commit_ = value;
    };

    const SequenceID& sequence_id() const noexcept {
        return sequence_id_;
    }

    const SequenceID& consistency_point_sequence_id() const noexcept {
        return sequence_id_;
    }


    void ref() noexcept {
        uses_.fetch_add(1);
    }

    void unref() noexcept {
        uses_.fetch_sub(1);
    }

    bool is_in_use() noexcept {
        return uses_.load() > 0;
    }

    const CommitID& commit_id() const noexcept {
        return commit_id_;
    }

    auto& children() noexcept {
        return children_;
    }

    const auto& children() const noexcept {
        return children_;
    }

    bool try_enqueue_for_eviction() noexcept
    {
        if (!is_linked())
        {
            if (is_attached() && references_ == 1 && parent_) {
                enque_for_eviction();
                return true;
            }

            return false;
        }
        else {
            return true;
        }
    }

    void unlink_from_eviction_queue();
};

template <typename Profile>
using CommitDescriptorsList = boost::intrusive::list<CommitDescriptor<Profile>>;

template <typename Profile>
inline void intrusive_ptr_add_ref(CommitDescriptor<Profile>* x) noexcept {
//    auto val =
    ++x->references_;

    //println("CD ref: {} {} {} {}", x->commit_id(), (int)x->is_new(), (int)x->is_transient(), val);
}

template <typename Profile>
inline void intrusive_ptr_release(CommitDescriptor<Profile>* x) noexcept {
    auto val = --x->references_;

//    if (val > 10) {
//        int a = 0;
//        a++;
//    }

//    println("CD unref: {} {} {} {}", x->commit_id(), (int)x->is_new(), (int)x->is_transient(), val);

    if(x->is_attached())
    {
        if (val == 2) {
            if (x->is_transient() && x->parent() && !x->is_linked()) {
                x->enque_for_eviction();
            }
        }
        else if (val == 0) {
            println("Zero refcounter for attached commit descriptor! {}", x->commit_id());
        }
    }
    else if(val == 0) { // is_new() == true
        if (x->is_linked()) {
            x->unlink_from_eviction_queue();
        }
        delete x;
    }
}

}
