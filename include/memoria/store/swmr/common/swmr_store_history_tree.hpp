
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/arena_buffer.hpp>

#include <memoria/profiles/common/common.hpp>
#include <memoria/store/swmr/common/swmr_store_commit_descriptor.hpp>
#include <memoria/store/swmr/common/swmr_store_datatypes.hpp>
#include <memoria/store/swmr/common/superblock.hpp>

#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <stack>

#include <absl/container/btree_map.h>
#include <absl/container/btree_set.h>

namespace memoria {

template <typename Profile>
class SWMRStoreBase;

template <typename Profile>
class HistoryTree {

    using CommitDescriptorT = CommitDescriptor<Profile>;
    using CommitID          = ProfileSnapshotID<Profile>;
    using SequeceID         = uint64_t;
    using CommitMetadataT   = CommitMetadata<ApiProfile<Profile>>;
    using SuperblockT       = SWMRSuperblock<Profile>;

public:
    struct UpdateOp {
        bool reparent;
        CommitID commit_id;
        CommitID new_parent_id;
    };

private:

    static constexpr uint64_t BASIC_BLOCK_SIZE = SWMRStoreBase<Profile>::BASIC_BLOCK_SIZE;

    using EvictionFn        = std::function<void (const UpdateOp&)>;
    using SuperblockFn      = std::function<SharedSBPtr<SuperblockT>(uint64_t)>;

    std::unordered_map<CommitID, CommitDescriptorT*> commits_;
    std::unordered_map<U8String, CommitDescriptorT*> branch_heads_;

    CommitDescriptorT* root_;
    CommitDescriptorT* consistency_point1_;
    CommitDescriptorT* consistency_point2_;

    CommitDescriptorsList<Profile> eviction_queue_;

    ArenaBuffer<CommitDescriptorT*> volatile_commits_;

    SuperblockFn superblock_fn_;

public:
    HistoryTree() noexcept :
        root_(),
        consistency_point1_(),
        consistency_point2_()
    {}

    ~HistoryTree() noexcept {
        for (auto commit: commits_) {
            if (commit.second->is_linked()) {
                eviction_queue_.erase(eviction_queue_.iterator_to(*commit.second));
            }

            delete commit.second;
        }
    }



    void do_remove_rollback_span(std::vector<CommitDescriptorT*>& span) noexcept
    {
        remove_tree_nodes(span);

        consistency_point1_ = consistency_point2_;
        consistency_point2_ = nullptr;

        volatile_commits_.clear();
    }

    std::vector<CommitDescriptorT*> get_rollback_span() const
    {
        std::vector<CommitDescriptorT*> buffer;

        if (consistency_point1_ && consistency_point2_)
        {
            if (consistency_point1_->sequence_id() - consistency_point2_->sequence_id() > 1) {
                auto cp2_seq_id = consistency_point2_->sequence_id();
                for (const auto& pair: commits_) {
                    if (pair.second->sequence_id() > cp2_seq_id) {
                        buffer.push_back(pair.second);
                    }
                }
            }
            else {
                buffer.push_back(consistency_point1_);
            }
        }

        return buffer;
    }

    std::vector<CommitDescriptorT*> get_volatile_commits() const {
        std::vector<CommitDescriptorT*> buffer(volatile_commits_.span().begin(), volatile_commits_.span().end());
        return buffer;
    }

    void remove_volatile_commits(std::vector<CommitDescriptorT*>& nodes)
    {
        remove_tree_nodes(nodes);
        volatile_commits_.clear();
    }

    void cleanup_eviction_queue()
    {
        eviction_queue_.erase_and_dispose(
            eviction_queue_.begin(),
            eviction_queue_.end(),
            [](CommitDescriptorT* commit_descr) noexcept {
                delete commit_descr;
            }
        );
    }

    int64_t count_volatile_commits() const noexcept {
        return volatile_commits_.size();
    }

    bool can_rollback_last_consistency_point() const noexcept {
        return volatile_commits_.size() == 0 && consistency_point2_ != nullptr;
    }

    void set_superblock_fn(SuperblockFn fn) noexcept {
        superblock_fn_ = fn;
    }

    std::vector<U8String> branch_names() const noexcept
    {
        std::vector<U8String> names;

        for (const auto& entries: branch_heads_) {
            names.emplace_back(entries.first);
        }

        return names;
    }

    size_t branches_size() const noexcept {
        return branch_heads_.size();
    }

    Optional<CommitDescriptorT*> get_branch_head(U8StringView name) const noexcept
    {
        auto ii = branch_heads_.find(name);
        if (ii != branch_heads_.end()) {
            return Optional<CommitDescriptorT*>{ii->second};
        }
        return Optional<CommitDescriptorT*>{};
    }

    CommitDescriptorT* consistency_point1() const noexcept {
        return consistency_point1_;
    }

    CommitDescriptorT* consistency_point2() const noexcept {
        return consistency_point2_;
    }

    const CommitDescriptorsList<Profile>& eviction_queue() const noexcept {
        return eviction_queue_;
    }

    CommitDescriptorsList<Profile>& eviction_queue() noexcept {
        return eviction_queue_;
    }

    U8String mark_branch_not_persistent(CommitDescriptorT* descr, U8StringView branch)
    {        
        CommitDescriptorT* dd = descr;

        while (true) {
            if (dd->parent() && dd->children().size() <= 1) {
                if (dd->is_persistent()) {
                    dd->set_persistent(false);

                    if (!is_in_last_2_queue(dd)) {
                        eviction_queue_.push_back(*dd);
                    }
                }
            }
            else {
                break;
            }

            dd = dd->parent();
        }

        branch_heads_.erase(branch_heads_.find(U8String(branch)));

        if (!dd->parent() && branch_heads_.empty()) {
            branch_heads_["main"] = descr;
            return "main";
        }

        return branch;
    }

    bool mark_commit_not_persistent(const CommitID& commit_id)
    {
        auto commit = get(commit_id);
        if (commit)
        {
            CommitDescriptorT* descr = commit.get();
            if (descr->is_persistent())
            {
                // TODO: Need to check the following code for more
                // cases of tree consistency violation.
                if (descr->parent()) {
                    // OK
                }
                else if (descr->children().size() > 1) {
                    MEMORIA_MAKE_GENERIC_ERROR("Can't remove root commit {} with multiple children.", descr->commit_id()).do_throw();
                }
                else if (descr->children().size() == 0) {
                    MEMORIA_MAKE_GENERIC_ERROR("Can't remove root commit {} without children.", descr->commit_id()).do_throw();
                }

                descr->set_persistent(false);

                eviction_queue_.push_back(*descr);
            }

            return true;
        }

        return false;
    }

    void prepare_eviction(const EvictionFn& eviction_fn)
    {
        for (CommitDescriptorT& descr: eviction_queue_)
        {
            CommitID parent_id{};

            if (descr.parent())
            {
                CommitDescriptorT* parent = descr.parent();

                parent->children().erase(&descr);
                parent_id = parent->commit_id();

                for (auto* chl: descr.children())
                {
                    chl->set_parent(parent);
                    eviction_fn(UpdateOp{true, chl->commit_id(), parent->commit_id()});
                }
            }
            else {
                for (auto* chl: descr.children()) {
                    chl->set_parent(nullptr);
                    root_ = chl;
                    eviction_fn(UpdateOp{true, chl->commit_id(), CommitID{}});
                }
            }

            commits_.erase(descr.commit_id());
            eviction_fn(UpdateOp{false, descr.commit_id()});
        }
    }

    Optional<CommitDescriptorT*> get(const CommitID& id) noexcept
    {
        auto ii = commits_.find(id);
        if (ii != commits_.end()) {
            return Optional<CommitDescriptorT*>{ii->second};
        }
        return Optional<CommitDescriptorT*>{};
    }


    void attach_commit(CommitDescriptorT* descr, bool consistency_point) noexcept
    {
        if (descr->parent()) {
            descr->parent()->children().insert(descr);
        }
        else {
            root_ = descr;
        }

        U8String branch_name = get_branch_name(descr);
        CommitDescriptorT* last_head = branch_heads_[branch_name];

        commits_[descr->commit_id()] = descr;
        branch_heads_[branch_name] = descr;

        if (consistency_point)
        {
            CommitDescriptorT* tmp = enqueue_last_2(descr);

            if (tmp) {
                if (!is_branch_head(tmp)) {
                    enqueue_for_eviction(tmp);
                }

                if (last_head &&  tmp != last_head && !is_in_last_2_queue(last_head)) {
                    enqueue_for_eviction(last_head);
                }
            }
            else if (last_head && !is_in_last_2_queue(last_head)) {
                enqueue_for_eviction(last_head);
            }

            volatile_commits_.clear();
        }
        else {
            volatile_commits_.append_value(descr);
        }
    }

    void load(
            Span<const std::pair<CommitID, CommitMetadataT>> metas,
            const CommitDescriptorT* consistency_point1,
            const CommitDescriptorT* consistency_point2
    )
    {
        // FIXME: fill eviction_queue here!
        for (const auto& pair: metas)
        {
            const CommitMetadataT& meta = std::get<1>(pair);
            const CommitID& commit_id   = std::get<0>(pair);

            CommitDescriptorT* descr = new CommitDescriptorT();

            auto superblock = superblock_fn_(meta.superblock_file_pos());

            descr->set_superblock(meta.superblock_file_pos() * BASIC_BLOCK_SIZE, superblock.get());
            descr->set_persistent(meta.is_persistent());

            commits_[commit_id] = descr;
        }

        for (const auto& pair: metas)
        {
            const CommitMetadataT& meta = std::get<1>(pair);
            const CommitID& commit_id   = std::get<0>(pair);

            CommitDescriptorT* current = commits_[commit_id];

            if (meta.parent_commit_id())
            {
                auto ii = commits_.find(meta.parent_commit_id());
                if (ii != commits_.end())
                {
                    CommitDescriptorT* parent = ii->second;
                    current->set_parent(parent);
                    parent->children().insert(current);
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Cannot find parent commit: {}", meta.parent_commit_id()).do_throw();
                }
            }
            else if (!root_) {
                root_ = current;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Multiple root commits in the store: {} and {}", root_->commit_id(), commit_id).do_throw();
            }
        }

        if (consistency_point1)
        {
            auto cp = get(consistency_point1->commit_id());
            if (cp) {
                consistency_point1_ = cp.get();
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Last commit ID is not found {}", consistency_point1->commit_id()).do_throw();
            }
        }

        if (consistency_point2)
        {
            auto cp = get(consistency_point2->commit_id());
            if (cp) {
                consistency_point2_ = cp.get();
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Previous last commit ID is not found {}", consistency_point2->commit_id()).do_throw();
            }
        }

        parse_commit_tree();
    }

private:
    class TraverseState {
        CommitDescriptorT* descr_;
        typename CommitDescriptorT::ChildIterator ii_;
        bool done_{false};

    public:
        TraverseState(CommitDescriptorT* descr) noexcept :
            descr_(descr),
            ii_(descr->children().begin())
        {}

        bool is_done() const noexcept {
            return done_;
        }

        void done() noexcept {
            done_ = true;
        }

        CommitDescriptorT* descr() const noexcept {
            return descr_;
        }

        CommitDescriptorT* next_child() noexcept {
            if (ii_ != descr_->children().end()) {
                CommitDescriptorT* tmp = *ii_;
                ++ii_;
                return tmp;
            }
            else {
                return nullptr;
            }
        }
    };
public:
    void traverse_tree_preorder(const std::function<bool (CommitDescriptorT*)>& fn) {
        traverse_tree_preorder(root_, fn);
    }

    void traverse_tree_preorder(const std::function<void (CommitDescriptorT*)>& fn) {
        traverse_tree_preorder(root_, [&](CommitDescriptorT* dd){
            fn(dd);
            return true;
        });
    }

    void traverse_tree_preorder(CommitDescriptorT* start, const std::function<bool (CommitDescriptorT*)>& fn)
    {
        if (!start) {
            return;
        }

        std::stack<TraverseState> stack;
        stack.push(TraverseState{start});

        while (!stack.empty())
        {
            TraverseState& state = stack.top();
            if (!state.is_done()) {
                if (!fn(state.descr()))
                {
                    break;
                }

                state.done();
            }

            CommitDescriptorT* child = state.next_child();
            if (child) {
                stack.push(TraverseState{child});
            }
            else {
                stack.pop();
            }
        }
    }


private:
    const U8String& get_branch_name(CommitDescriptorT* descr) noexcept {
        return descr->branch();
    }

    void parse_commit_tree()
    {
        uint64_t counter{};
        traverse_tree_preorder([&](CommitDescriptorT* descr){
            ++counter;

            if (descr->children().empty()) {
                branch_heads_[get_branch_name(descr)] = descr;
            }
            else if (descr == consistency_point2_ || descr == consistency_point1_) {
                // do nothing
            }
            else if (!descr->is_persistent()) {
                eviction_queue_.push_back(*descr);
            }
        });

        if (counter != commits_.size()) {
            println("Inconsistent history tree: {} :: {}", counter, commits_.size());
        }
    }

    bool is_branch_head(CommitDescriptorT* descr)
    {
        for (auto pair: branch_heads_) {
            if (pair.second == descr) {
                return true;
            }
        }

        return false;
    }

    void enqueue_for_eviction(CommitDescriptorT* descr) noexcept
    {
        bool persistent = descr->is_persistent();
        if (!persistent) {
            eviction_queue_.push_back(*descr);
        }
    }

    CommitDescriptorT* enqueue_last_2(CommitDescriptorT* descr) noexcept
    {
        CommitDescriptorT* tmp = consistency_point2_;
        consistency_point2_ = consistency_point1_;
        consistency_point1_ = descr;

        return tmp;
    }

    bool is_in_last_2_queue(CommitDescriptorT* descr) noexcept {
        return descr == consistency_point1_ || descr == consistency_point2_;
    }

    void remove_tree_nodes(std::vector<CommitDescriptorT*>& nodes) noexcept
    {
        std::sort(nodes.begin(), nodes.end(), [](const auto* one, const auto* two){
            return one->sequence_id() > two->sequence_id();
        });

        for (CommitDescriptorT* descr: nodes) {
            if (descr->is_linked()) {
                eviction_queue_.erase(eviction_queue_.iterator_to(*descr));
            }

            commits_.erase(descr->commit_id());

            descr->detach_from_tree();
            delete descr;
        }
    }
};


}
