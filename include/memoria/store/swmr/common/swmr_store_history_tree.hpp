
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
    using CDescrPtr         = typename CommitDescriptorT::SharedPtrT;

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

    std::unordered_map<CommitID, CDescrPtr> commits_;
    std::unordered_map<U8String, CDescrPtr> branch_heads_;

    CDescrPtr root_;
    CDescrPtr consistency_point1_;
    CDescrPtr consistency_point2_;
    CDescrPtr head_;

    CommitDescriptorsList<Profile> eviction_queue_;

    SuperblockFn superblock_fn_;

public:
    HistoryTree() noexcept :
        root_(),
        consistency_point1_(),
        consistency_point2_(),
        head_()
    {}

    ~HistoryTree() noexcept
    {
        for (const auto& entry: commits_) {
            entry.second->set_new();
        }
    }

    template <typename... Args>
    CDescrPtr new_commit_descriptor(Args&&... args) {
        return new CommitDescriptorT(this, std::forward<Args>(args)...);
    }



    bool is_clean() const noexcept {
        return head_ == consistency_point1_;
    }

    int64_t count_volatile_commits() const noexcept {
        // FIXME: implement volatile commits counter
        return 0;
    }

    bool can_rollback_last_consistency_point() const noexcept {
        return head_ == consistency_point1_ && consistency_point2_ != nullptr;
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

    CDescrPtr get_branch_head(U8StringView name) const noexcept
    {
        auto ii = branch_heads_.find(name);
        if (ii != branch_heads_.end()) {
            return ii->second;
        }
        return CDescrPtr{};
    }

    CDescrPtr head() const noexcept {
        return head_;
    }

    CDescrPtr consistency_point1() const noexcept {
        return consistency_point1_;
    }

    CDescrPtr consistency_point2() const noexcept {
        return consistency_point2_;
    }

    const CommitDescriptorsList<Profile>& eviction_queue() const noexcept {
        return eviction_queue_;
    }

    CommitDescriptorsList<Profile>& eviction_queue() noexcept {
        return eviction_queue_;
    }

    U8String mark_branch_transient(CommitDescriptorT* descr, U8StringView branch)
    {        
        CommitDescriptorT* dd = descr;

        while (true)
        {
            if (dd->parent() && dd->children().size() <= 1) {
                if (!dd->is_transient())
                {
                    dd->set_transient(true);

                    if (!is_pinned(dd)) {
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

    bool mark_commit_transient(const CommitID& commit_id)
    {
        auto descr = get(commit_id);
        if (descr) {
            if (!descr->is_transient())
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

                descr->set_transient(true);

                return true;
            }
        }

        return false;
    }

    void prepare_eviction(const EvictionFn& eviction_fn)
    {
        for (CommitDescriptorT& descr: eviction_queue_)
        {
            if (descr.parent())
            {
                CommitDescriptorT* parent = descr.parent();
                CommitID parent_id = parent->commit_id();

                for (auto chl: descr.children()) {
                    eviction_fn(UpdateOp{true, chl->commit_id(), parent_id});
                }
            }
            else {
                for (auto chl: descr.children()) {
                    eviction_fn(UpdateOp{true, chl->commit_id(), CommitID{}});
                }
            }

            eviction_fn(UpdateOp{false, descr.commit_id()});
        }
    }

    void cleanup_eviction_queue() noexcept
    {
        for (CommitDescriptorT& descr: eviction_queue_)
        {
            if (descr.parent())
            {
                CommitDescriptorT* parent = descr.parent();
                parent->children().erase(&descr);

                for (auto chl: descr.children()) {
                    chl->set_parent(parent);
                    parent->children().insert(chl);
                }
            }
            else {
                for (auto chl: descr.children()) {
                    chl->set_parent(nullptr);
                    root_ = chl;
                }
            }
        }

        eviction_queue_.erase_and_dispose(
            eviction_queue_.begin(),
            eviction_queue_.end(),
            [&](CommitDescriptorT* commit_descr) noexcept {
                commit_descr->set_new();
                commits_.erase(commit_descr->commit_id());
            }
        );
    }

    CDescrPtr get(const CommitID& id) noexcept
    {
        auto ii = commits_.find(id);
        if (ii != commits_.end()) {
            return ii->second;
        }
        return CDescrPtr{};
    }


    void attach_commit(CDescrPtr descr, bool consistency_point) noexcept
    {
        descr->set_attached();

        if (descr->parent()) {
            descr->parent()->children().insert(descr);
        }
        else {
            root_ = descr;
        }

        U8String branch_name = get_branch_name(descr);
        CDescrPtr last_head = branch_heads_[branch_name];

        commits_[descr->commit_id()] = descr;
        branch_heads_[branch_name] = descr;

        if (consistency_point)
        {
            consistency_point2_ = consistency_point1_;
            consistency_point1_ = descr;
        }

        head_ = descr;

//        println("Attaching commit: {} {} || {} {} {}",
//                descr->commit_id(),
//                descr->references_,
//                consistency_point1_ ? consistency_point1_->commit_id() : UUID{},
//                consistency_point2_ ? consistency_point2_->commit_id() : UUID{},
//                head_->commit_id()
//                );
    }

    void load(
            Span<const std::pair<CommitID, CommitMetadataT>> metas,
            CDescrPtr consistency_point1,
            CDescrPtr consistency_point2
    )
    {
        for (const auto& pair: metas)
        {
            const CommitMetadataT& meta = std::get<1>(pair);
            const CommitID& commit_id   = std::get<0>(pair);

            auto superblock = superblock_fn_(meta.superblock_file_pos());

            LDDocumentView doc = superblock->metadata_doc();
            LDDMapView map = doc.value().as_map();

            U8String branch_name;
            auto bname_opt = map.get("branch_name");
            if (bname_opt) {
                branch_name = bname_opt.get().as_varchar().view();
            }
            else {
                branch_name = "";
            }

            CDescrPtr descr = new_commit_descriptor(branch_name);

            descr->set_superblock(meta.superblock_file_pos() * BASIC_BLOCK_SIZE, superblock.get());
            descr->set_transient(meta.is_transient());
            descr->set_system_commit(meta.is_system_commit());

            commits_[commit_id] = descr;
        }

        for (const auto& pair: metas)
        {
            const CommitMetadataT& meta = std::get<1>(pair);
            const CommitID& commit_id   = std::get<0>(pair);

            CDescrPtr current = commits_[commit_id];

            if (meta.parent_commit_id())
            {
                auto ii = commits_.find(meta.parent_commit_id());
                if (ii != commits_.end())
                {
                    CDescrPtr parent = ii->second;
                    current->set_parent(parent.get());
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
                head_ = consistency_point1_ = cp.get();
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
        CDescrPtr descr_;
        typename CommitDescriptorT::ChildIterator ii_;
        bool done_{false};

    public:
        TraverseState(CDescrPtr descr) noexcept :
            descr_(descr),
            ii_(descr->children().begin())
        {}

        bool is_done() const noexcept {
            return done_;
        }

        void done() noexcept {
            done_ = true;
        }

        CDescrPtr descr() const noexcept {
            return descr_;
        }

        CDescrPtr next_child() noexcept {
            if (ii_ != descr_->children().end()) {
                CDescrPtr tmp = *ii_;
                ++ii_;
                return tmp;
            }
            else {
                return nullptr;
            }
        }
    };
public:
    void traverse_tree_preorder(const std::function<bool (CDescrPtr)>& fn) {
        traverse_tree_preorder(root_, fn);
    }

    void traverse_tree_preorder(const std::function<void (CDescrPtr)>& fn) {
        traverse_tree_preorder(root_, [&](CDescrPtr dd){
            fn(dd);
            return true;
        });
    }

    void traverse_tree_preorder(CDescrPtr start, const std::function<bool (CDescrPtr)>& fn)
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

            CDescrPtr child = state.next_child();
            if (child) {
                stack.push(TraverseState{child});
            }
            else {
                stack.pop();
            }
        }
    }

private:
    const U8String& get_branch_name(CDescrPtr descr) noexcept {
        return descr->branch();
    }

    void parse_commit_tree()
    {
        uint64_t counter{};
        traverse_tree_preorder([&](CDescrPtr descr){
            ++counter;

            if (descr->children().empty()) {
                branch_heads_[get_branch_name(descr)] = descr;
            }
            else if (descr == consistency_point2_ || descr == consistency_point1_) {
                // do nothing
            }
            else if (descr->is_transient()) {
                eviction_queue_.push_back(*descr.get());
            }
        });

        if (counter != commits_.size()) {
            println("Inconsistent history tree: {} :: {}", counter, commits_.size());
        }
    }
};

template <typename Profile>
inline void CommitDescriptor<Profile>::enque_for_eviction() {
    history_tree_->eviction_queue().push_back(*this);
}

template <typename Profile>
inline void CommitDescriptor<Profile>::unlink_from_eviction_queue() {
    auto& queue = history_tree_->eviction_queue();
    queue.erase(
        queue.iterator_to(*this)
    );
}

}
