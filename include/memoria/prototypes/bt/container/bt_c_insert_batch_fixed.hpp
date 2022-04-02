
// Copyright 2015 Victor Smirnov
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node.hpp>

#include <vector>
#include <algorithm>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::InsertBatchFixedName)


    using typename Base::BlockID;
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::TreePathT;

    using typename Base::Checkpoint;
    using typename Base::ILeafProvider;


public:
    class InsertBatchResult {
        int32_t idx_;
        CtrSizeT subtree_size_;
    public:
        InsertBatchResult(int32_t idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

        int32_t local_pos() const {return idx_;}
        int32_t idx() const {return idx_;}
        CtrSizeT subtree_size() const {return subtree_size_;}
    };

    class BranchNodeEntryT {
        BranchNodeEntry accum_;
        BlockID child_id_;
        TreeNodePtr node_;
    public:
        BranchNodeEntryT(const BranchNodeEntryT& accum, const BlockID& id, TreeNodePtr node):
            accum_(accum), child_id_(id)
        {}

        BranchNodeEntryT(): accum_(), child_id_(), node_()
        {}

        const BranchNodeEntry& accum() const {return accum_;}
        const BlockID& child_id() const {return child_id_;}

        BranchNodeEntry& accum() {return accum_;}
        BlockID& child_id() {return child_id_;}
        TreeNodePtr& child_node() {return node_;}
    };


    struct CommitInsertChildrenFn {
        template <typename CtrT, typename NodeT, typename UpdateState>
        void treeNode(BranchNodeSO<CtrT, NodeT>& node, int32_t from, int32_t to, const BranchNodeEntryT* entries, UpdateState& update_state)
        {
            auto old_size = node.size();

            node.processAll(*this, from, to, entries, update_state);

            int32_t idx = 0;
            PackedAllocatorUpdateState& state = bt::get_allocator_update_state(update_state);
            return node.commit_insert_values(old_size, from, to - from, state, [entries, &idx](){
                return entries[idx++].child_id();
            });
        }

        template <int32_t ListIdx, typename StreamType, typename UpdateState>
        void stream(StreamType& obj, int32_t from, int32_t to, const BranchNodeEntryT* entries, UpdateState& update_state)
        {
            return obj.commit_insert(from, to - from, std::get<ListIdx>(update_state), [entries](int32_t column, int32_t idx) -> const auto& {
                return std::get<ListIdx>(entries[idx].accum())[column];
            });
        }
    };

    InsertBatchResult ctr_insert_subtree(TreePathT& path, size_t level, int32_t idx, ILeafProvider& provider, std::function<TreeNodePtr ()> child_fn, bool update_hierarchy)
    {
        auto& self = this->self();

        TreeNodeConstPtr node = path[level];

        auto capacity = self.ctr_get_branch_node_capacity(node);
        CtrSizeT provider_size0  = provider.size();
        const int32_t batch_size = 32;

        int32_t max = idx;

        for (int32_t c = 0; c < capacity; c+= batch_size)
        {
            BranchNodeEntryT subtrees[batch_size];

            int32_t i, batch_max = (c + batch_size) < capacity ? batch_size : (capacity - c);
            for (i = 0; i < batch_max && provider.size() > 0; i++)
            {
                auto child = child_fn();

                auto max = self.ctr_get_node_max_keys(child.as_immutable());
                subtrees[i].accum()         = max;
                subtrees[i].child_id()      = child->id();
                subtrees[i].child_node()    = child;
            }

            auto update_state = self.ctr_make_branch_update_state();
            self.branch_dispatcher().dispatch(node.as_mutable(), CommitInsertChildrenFn(), idx + c, idx + c + i, subtrees, update_state);

            max = idx + c + i;

            if (i < batch_max) {
                break;
            }
        }

        if (update_hierarchy){
            self.ctr_update_path(path, level);
        }

        return InsertBatchResult(max, provider_size0 - provider.size());
    }


    TreeNodePtr ctr_build_subtree(ILeafProvider& provider, size_t level)
    {
        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                auto node = self.ctr_create_node(level, false, false);
                self.ctr_ref_block(node->id());

                self.ctr_layout_branch_node(node);

                TreePathT path;
                path.add_root(node.as_immutable());

                self.ctr_insert_subtree(path, 0, 0, provider, [this, level, &provider]() {
                    auto& self = this->self();
                    return self.ctr_build_subtree(provider, level - 1);
                }, false);


                return node;
            }
            else {
                return provider.get_leaf();
            }
        }
        else {
            return TreeNodePtr{};
        }
    }



    class ListLeafProvider: public ILeafProvider {
        TreeNodePtr   head_;
        CtrSizeT    size_ = 0;

        MyType&     ctr_;

    public:
        ListLeafProvider(MyType& ctr, TreeNodePtr head, CtrSizeT size): head_(head),  size_(size), ctr_(ctr) {}

        virtual CtrSizeT size() const
        {
            return size_;
        }

        virtual TreeNodePtr get_leaf()
        {
            if (head_.isSet())
            {
                auto node = head_;

                auto block = ctr_.store().getBlock(head_->next_leaf_id(), ctr_.master_name());

                head_ = block;
                size_--;
                return node;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Leaf List is empty").do_throw();
            }
        }


        virtual Checkpoint checkpoint() {
            return Checkpoint(head_, size_);
        }


        virtual void rollback(const Checkpoint& checkpoint)
        {
            size_   = checkpoint.size();
            head_   = checkpoint.head();
        }
    };







    InsertBatchResult ctr_insert_batch_to_node(
            TreePathT& path,
            size_t level,
            int32_t idx,
            ILeafProvider& provider,
            bool update_hierarchy = true
    )
    {
        auto& self = this->self();
        return self.ctr_insert_subtree(path, level, idx, provider, [&provider, level, this]() {
            auto& self = this->self();
            return self.ctr_build_subtree(provider, level - 1);
        },
        update_hierarchy);
    }






MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
