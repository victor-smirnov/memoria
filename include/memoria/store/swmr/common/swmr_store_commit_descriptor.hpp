
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

#include <memoria/profiles/common/common.hpp>

#include <memoria/store/swmr/common/superblock.hpp>


#include <boost/intrusive/list.hpp>

#include <atomic>
#include <unordered_set>

namespace memoria {

template <typename Profile>
class CommitDescriptor: public boost::intrusive::list_base_hook<> {
    using Superblock = SWMRSuperblock<Profile>;
    using BlockID    = ProfileBlockID<Profile>;
    using CommitID   = typename Superblock::CommitID;
    using SequenceID   = typename Superblock::SequenceID;

    using Children = std::unordered_set<CommitDescriptor*>;

public:

    using ChildIterator = typename Children::iterator;
    using ConstChildIterator = typename Children::const_iterator;

    using Allocations = ArenaBuffer<AllocationMetadata<ApiProfile<Profile>>>;

private:

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

public:
    CommitDescriptor(U8StringView branch = "main") noexcept:
        superblock_ptr_(),
        parent_(),
        branch_(branch)
    {}

    CommitDescriptor(uint64_t superblock_ptr, Superblock* superblock, U8StringView branch = "main") noexcept:
        superblock_ptr_(superblock_ptr),
        parent_(),
        branch_(branch)
    {
        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        commit_id_   = superblock->commit_id();
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

    void set_superblock(uint64_t ptr, Superblock* superblock) noexcept {
        superblock_ptr_ = ptr;

        sequence_id_ = superblock->sequence_id();
        consistency_point_sequence_id_ = superblock->consistency_point_sequence_id();
        commit_id_   = superblock->commit_id();
    }

    void refresh_descriptor(Superblock* superblock) noexcept {
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

    std::unordered_set<CommitDescriptor*>& children() noexcept {
        return children_;
    }

    const std::unordered_set<CommitDescriptor*>& children() const noexcept {
        return children_;
    }
};

template <typename Profile>
using CommitDescriptorsList = boost::intrusive::list<CommitDescriptor<Profile>>;

}
