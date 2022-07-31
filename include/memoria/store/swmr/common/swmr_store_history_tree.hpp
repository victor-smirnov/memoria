
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
#include <memoria/store/swmr/common/swmr_store_snapshot_descriptor.hpp>
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

    using SnapshotDescriptorT = SnapshotDescriptor<Profile>;
    using CDescrPtr         = typename SnapshotDescriptorT::SharedPtrT;

    using SnapshotID        = ProfileSnapshotID<Profile>;
    using SequeceID         = uint64_t;
    using SnapshotMetadataT = SWMRSnapshotMetadata<ApiProfile<Profile>>;
    using SuperblockT       = SWMRSuperblock<Profile>;

public:
    struct UpdateOp {
        bool reparent;
        SnapshotID snapshot_id;
        SnapshotID new_parent_id;
    };

private:

    static constexpr uint64_t BASIC_BLOCK_SIZE = SWMRStoreBase<Profile>::BASIC_BLOCK_SIZE;

    using EvictionFn        = std::function<void (const UpdateOp&)>;
    using SuperblockFn      = std::function<SharedSBPtr<SuperblockT>(uint64_t)>;

    std::unordered_map<SnapshotID, CDescrPtr> snapshots_;
    std::unordered_map<U8String, CDescrPtr> branch_heads_;

    CDescrPtr root_;
    CDescrPtr consistency_point1_;
    CDescrPtr consistency_point2_;
    CDescrPtr head_;

    SnapshotDescriptorsList<Profile> eviction_queue_;

    SuperblockFn superblock_fn_;

public:
    HistoryTree()  :
        root_(),
        consistency_point1_(),
        consistency_point2_(),
        head_()
    {}

    ~HistoryTree()
    {
        for (const auto& entry: snapshots_) {
            entry.second->set_new();
        }
    }

    template <typename... Args>
    CDescrPtr new_snapshot_descriptor(Args&&... args) {
        return new SnapshotDescriptorT(this, std::forward<Args>(args)...);
    }



    bool is_clean() const  {
        return head_ == consistency_point1_;
    }

    int64_t count_volatile_snapshots() const  {
        // FIXME: implement volatile snapshots counter
        return 0;
    }

    bool can_rollback_last_consistency_point() const  {
        return head_ == consistency_point1_ && consistency_point2_ != nullptr;
    }

    void set_superblock_fn(SuperblockFn fn)  {
        superblock_fn_ = fn;
    }

    std::vector<U8String> branch_names() const
    {
        std::vector<U8String> names;

        for (const auto& entries: branch_heads_) {
            names.emplace_back(entries.first);
        }

        return names;
    }

    size_t branches_size() const  {
        return branch_heads_.size();
    }

    CDescrPtr get_branch_head(U8StringView name) const
    {
        auto ii = branch_heads_.find(name);
        if (ii != branch_heads_.end()) {
            return ii->second;
        }
        return CDescrPtr{};
    }

    void remove_branch(U8StringView name)  {
        branch_heads_.erase(name);
    }

    CDescrPtr head() const  {
        return head_;
    }

    CDescrPtr consistency_point1() const  {
        return consistency_point1_;
    }

    CDescrPtr consistency_point2() const  {
        return consistency_point2_;
    }

    const SnapshotDescriptorsList<Profile>& eviction_queue() const  {
        return eviction_queue_;
    }

    SnapshotDescriptorsList<Profile>& eviction_queue()  {
        return eviction_queue_;
    }

    U8String mark_branch_transient(SnapshotDescriptorT* descr, U8StringView branch)
    {        
        SnapshotDescriptorT* dd = descr;

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

    bool mark_snapshot_transient(const SnapshotID& snapshot_id)
    {
        auto descr = get(snapshot_id);
        if (descr) {
            if (!descr->is_transient())
            {
                // TODO: Need to check the following code for more
                // cases of tree consistency violation.
                if (descr->parent()) {
                    // OK
                }
                else if (descr->children().size() > 1) {
                    MEMORIA_MAKE_GENERIC_ERROR("Can't remove root snapshot {} with multiple children.", descr->snapshot_id()).do_throw();
                }
                else if (descr->children().size() == 0) {
                    MEMORIA_MAKE_GENERIC_ERROR("Can't remove root snapshot {} without children.", descr->snapshot_id()).do_throw();
                }

                descr->set_transient(true);

                return true;
            }
        }

        return false;
    }

    void prepare_eviction(const EvictionFn& eviction_fn)
    {
        std::vector<SnapshotDescriptorT*> reparenting_set;

        for (SnapshotDescriptorT& descr: eviction_queue_)
        {
            eviction_fn(UpdateOp{false, descr.snapshot_id()});

            for (auto& chl: descr.children()) {
                if (!chl->is_linked()) {
                    reparenting_set.push_back(chl.get());
                }
            }
        }

        for (SnapshotDescriptorT* descr: reparenting_set)
        {
            auto parent = descr->parent();
            while (parent && parent->is_linked()) {
                parent = parent->parent();
            }

            if (parent) {
                eviction_fn(UpdateOp{true, descr->snapshot_id(), parent->snapshot_id()});
            }
            else {
                eviction_fn(UpdateOp{true, descr->snapshot_id(), SnapshotID{}});
            }
        }



//        for (SnapshotDescriptorT& descr: eviction_queue_)
//        {
//            if (descr.parent())
//            {
//                SnapshotDescriptorT* parent = descr.parent();
//                SnapshotID parent_id = parent->snapshot_id();

//                for (auto chl: descr.children()) {
//                    eviction_fn(UpdateOp{true, chl->snapshot_id(), parent_id});
//                }
//            }
//            else {
//                for (auto chl: descr.children()) {
//                    eviction_fn(UpdateOp{true, chl->snapshot_id(), SnapshotID{}});
//                }
//            }

//            eviction_fn(UpdateOp{false, descr.snapshot_id()});
//        }
    }

    void cleanup_eviction_queue()
    {
        for (SnapshotDescriptorT& descr: eviction_queue_)
        {
            if (descr.parent())
            {
                SnapshotDescriptorT* parent = descr.parent();
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
            [&](SnapshotDescriptorT* snapshot_descr)  {
                snapshot_descr->set_new();
                snapshots_.erase(snapshot_descr->snapshot_id());
            }
        );
    }

    CDescrPtr get(const SnapshotID& id)
    {
        auto ii = snapshots_.find(id);
        if (ii != snapshots_.end()) {
            return ii->second;
        }
        return CDescrPtr{};
    }


    void attach_snapshot(CDescrPtr descr, bool consistency_point)
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

        snapshots_[descr->snapshot_id()] = descr;
        branch_heads_[branch_name] = descr;

        if (consistency_point)
        {
            consistency_point2_ = consistency_point1_;
            consistency_point1_ = descr;
        }

        head_ = descr;
    }

    void load(
            Span<const std::pair<SnapshotID, SnapshotMetadataT>> metas,
            CDescrPtr consistency_point1,
            CDescrPtr consistency_point2
    )
    {
        for (const auto& pair: metas)
        {
            const SnapshotMetadataT& meta = std::get<1>(pair);
            const SnapshotID& snapshot_id   = std::get<0>(pair);

            auto superblock = superblock_fn_(meta.superblock_file_pos());

            auto doc = superblock->metadata_doc();
            auto map = doc->value()->as_map();

            U8String branch_name;
            auto bname_opt = map->get("branch_name");
            if (bname_opt) {
                branch_name = bname_opt->as_varchar()->view();
            }
            else {
                branch_name = "";
            }

            CDescrPtr descr = new_snapshot_descriptor(branch_name);

            descr->set_superblock(meta.superblock_file_pos() * BASIC_BLOCK_SIZE, superblock.get());
            descr->set_transient(meta.is_transient());
            descr->set_system_snapshot(meta.is_system_snapshot());

            snapshots_[snapshot_id] = descr;
        }

        for (const auto& pair: metas)
        {
            const SnapshotMetadataT& meta = std::get<1>(pair);
            const SnapshotID& snapshot_id   = std::get<0>(pair);

            CDescrPtr current = snapshots_[snapshot_id];

            if (meta.parent_snapshot_id())
            {
                auto ii = snapshots_.find(meta.parent_snapshot_id());
                if (ii != snapshots_.end())
                {
                    CDescrPtr parent = ii->second;
                    current->set_parent(parent.get());
                    parent->children().insert(current);
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Cannot find parent snapshot: {}", meta.parent_snapshot_id()).do_throw();
                }
            }
            else if (!root_) {
                root_ = current;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Multiple root snapshots in the store: {} and {}", root_->snapshot_id(), snapshot_id).do_throw();
            }
        }

        if (consistency_point1)
        {
            auto cp = get(consistency_point1->snapshot_id());
            if (cp) {
                head_ = consistency_point1_ = cp.get();
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Last snapshot ID is not found {}", consistency_point1->snapshot_id()).do_throw();
            }
        }

        if (consistency_point2)
        {
            auto cp = get(consistency_point2->snapshot_id());
            if (cp) {
                consistency_point2_ = cp.get();
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Previous last snapshot ID is not found {}", consistency_point2->snapshot_id()).do_throw();
            }
        }

        parse_snapshot_tree();
    }



private:
    class TraverseState {
        CDescrPtr descr_;
        typename SnapshotDescriptorT::ChildIterator ii_;
        bool done_{false};

    public:
        TraverseState(CDescrPtr descr)  :
            descr_(descr),
            ii_(descr->children().begin())
        {}

        bool is_done() const  {
            return done_;
        }

        void done()  {
            done_ = true;
        }

        CDescrPtr descr() const  {
            return descr_;
        }

        CDescrPtr next_child()  {
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
    const U8String& get_branch_name(CDescrPtr descr)  {
        return descr->branch();
    }

    void parse_snapshot_tree()
    {
        uint64_t counter{};
        traverse_tree_preorder([&](CDescrPtr descr){
            ++counter;

//            println("Loading snapshot: {}", descr->snapshot_id());

            if (descr->children().empty()) {
                branch_heads_[get_branch_name(descr)] = descr;
            }
            else if (descr == consistency_point2_ || descr == consistency_point1_ || !descr->parent()) {
                // do nothing
            }
            else if (descr->is_transient()) {
//                println("*** Parse Tree: enqueue for eviction: {}", descr->snapshot_id());
                eviction_queue_.push_back(*descr.get());
            }
        });

        if (counter != snapshots_.size()) {
            println("Inconsistent history tree: {} :: {}", counter, snapshots_.size());
        }
    }
};

template <typename Profile>
inline void SnapshotDescriptor<Profile>::enqueue_for_eviction()
{
    //println("*Enqueue snapshot for eviction: {}", snapshot_id());
    history_tree_->eviction_queue().push_back(*this);
}

template <typename Profile>
inline void SnapshotDescriptor<Profile>::unlink_from_eviction_queue() {
    auto& queue = history_tree_->eviction_queue();
    queue.erase(
        queue.iterator_to(*this)
    );
}

}
