
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>
#include <algorithm>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::InsertBatchFixedName)

public:
    using Types = typename Base::Types;
    using Allocator = typename Base::Allocator;

protected:
    using typename Base::BlockID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                       BlockUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    using Checkpoint    = typename Base::Checkpoint;
    using ILeafProvider = typename Base::ILeafProvider;


public:
    class InsertBatchResult {
        int32_t idx_;
        CtrSizeT subtree_size_;
    public:
        InsertBatchResult(int32_t idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

        int32_t local_pos() const {return idx_;}
        CtrSizeT subtree_size() const {return subtree_size_;}
    };

    class BranchNodeEntryT {
        BranchNodeEntry accum_;
        BlockID child_id_;
    public:
        BranchNodeEntryT(const BranchNodeEntryT& accum, const BlockID& id): accum_(accum), child_id_(id) {}
        BranchNodeEntryT() : child_id_() {}

        const BranchNodeEntry& accum() const {return accum_;}
        const BlockID& child_id() const {return child_id_;}

        BranchNodeEntry& accum() {return accum_;}
        BlockID& child_id() {return child_id_;}
    };


    struct InsertChildrenFn {
        template <typename CtrT, typename NodeT>
        void treeNode(BranchNodeSO<CtrT, NodeT>& node, int32_t from, int32_t to, const BranchNodeEntryT* entries)
        {
            int old_size = node.size();

            node.processAll(*this, from, to, entries);

            int32_t idx = 0;
            node.insertValues(old_size, from, to - from, [entries, &idx](){
                return entries[idx++].child_id();
            });
        }

        template <int32_t ListIdx, typename StreamType>
        void stream(StreamType& obj, int32_t from, int32_t to, const BranchNodeEntryT* entries)
        {
            OOM_THROW_IF_FAILED(obj.insert(from, to - from, [entries](int32_t idx) -> const auto& {
                return std::get<ListIdx>(entries[idx].accum());
            }), MMA1_SRC);
        }
    };

    InsertBatchResult insertSubtree(NodeBaseG& node, int32_t idx, ILeafProvider& provider, std::function<NodeBaseG ()> child_fn, bool update_hierarchy)
    {
        auto& self = this->self();

        int32_t capacity         = self.getBranchNodeCapacity(node, -1ull);
        CtrSizeT provider_size0  = provider.size();
        const int32_t batch_size = 32;

        int32_t max = idx;

        for (int32_t c = 0; c < capacity; c+= batch_size)
        {
            BranchNodeEntryT subtrees[batch_size];

            int32_t i, batch_max = (c + batch_size) < capacity ? batch_size : (capacity - c);
            for (i = 0; i < batch_max && provider.size() > 0; i++)
            {
                NodeBaseG child = child_fn();

                subtrees[i].accum()     = self.max(child);
                subtrees[i].child_id()  = child->id();

                child->parent_id() = node->id();
                child->parent_idx() = idx + c + i;
            }

            self.branch_dispatcher().dispatch(node, InsertChildrenFn(), idx + c, idx + c + i, subtrees);

            max = idx + c + i;

            if (i < batch_max)
            {
                break;
            }
        }

        if (update_hierarchy)
        {
            self.update_path(node);
            self.updateChildIndexes(node, max);
        }

        return InsertBatchResult(max, provider_size0 - provider.size());
    }


    NodeBaseG BuildSubtree(ILeafProvider& provider, int32_t level)
    {
        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                NodeBaseG node = self.createNode(level, false, false);

                self.layoutBranchNode(node, 0xFF);

                self.insertSubtree(node, 0, provider, [this, level, &provider]() -> NodeBaseG {
                    auto& self = this->self();
                    return self.BuildSubtree(provider, level - 1);
                }, false);

                return node;
            }
            else {
                return provider.get_leaf();
            }
        }
        else {
            return NodeBaseG();
        }
    }





    class ListLeafProvider: public ILeafProvider {
        NodeBaseG   head_;
        CtrSizeT    size_ = 0;

        MyType&     ctr_;

    public:
        ListLeafProvider(MyType& ctr, NodeBaseG head, CtrSizeT size): head_(head),  size_(size), ctr_(ctr) {}

        virtual CtrSizeT size() const
        {
            return size_;
        }

        virtual NodeBaseG get_leaf()
        {
            if (head_.isSet())
            {
                auto node = head_;
                head_ = ctr_.store().getBlock(head_->next_leaf_id(), ctr_.master_name());
                size_--;
                return node;
            }
            else {
                MMA1_THROW(BoundsException()) << WhatInfo("Leaf List is empty");
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




    class InsertionState {
        int32_t inserted_ = 0;
        int32_t total_;
    public:
        InsertionState(int32_t total): total_(total) {}

        int32_t& total() {
            return total_;
        }

        int32_t& inserted() {
            return inserted_;
        }

        bool shouldMoveUp() const {
            return inserted_ <= total_ / 3;
        }
    };


    InsertBatchResult insertBatchToNode(NodeBaseG& node, int32_t idx, ILeafProvider& provider, int32_t level = 1, bool update_hierarchy = true)
    {
        auto& self = this->self();
        return self.insertSubtree(node, idx, provider, [&provider, &node, this]() -> NodeBaseG {
            auto& self = this->self();
            return self.BuildSubtree(provider, node->level() - 1);
        },
        update_hierarchy);
    }

    void insert_subtree(NodeBaseG& left, NodeBaseG& right, ILeafProvider& provider, InsertionState& state, int32_t level = 1)
    {
        auto& self = this->self();

        int32_t left_size0 = self.getBranchNodeSize(left);

        auto left_result = insertBatchToNode(left, left_size0, provider, level);

        state.inserted() += left_result.subtree_size();

        if (state.shouldMoveUp())
        {
            auto left_parent    = self.getNodeParentForUpdate(left);
            auto right_parent   = self.getNodeParentForUpdate(right);

            if (left_parent == right_parent)
            {
                right_parent = self.splitPathP(left_parent, right->parent_idx());
            }

            insert_subtree(left_parent, right_parent, provider, state, level + 1);
        }
        else {
            auto right_result = insertBatchToNode(right, 0, provider, level);
            state.inserted() += right_result.subtree_size();
        }
    }

    NodeBaseG insert_subtree_at_end(NodeBaseG& left, ILeafProvider& provider, InsertionState& state, int32_t level = 1)
    {
        auto& self = this->self();

        int32_t left_size0 = self.getBranchNodeSize(left);

        auto left_result = insertBatchToNode(left, left_size0, provider, level);

        state.inserted() += left_result.subtree_size();

        if (provider.size() > 0)
        {
            if (left->is_root())
            {
                self.newRootP(left);
            }

            auto left_parent = self.getNodeParentForUpdate(left);

            auto right = insert_subtree_at_end(left_parent, provider, state, level + 1);

            int32_t right_size = self.getBranchNodeSize(right);

            return self.getChild(right, right_size - 1);
        }
        else {
            return left;
        }
    }


    int32_t insert_subtree(NodeBaseG& node, int32_t pos, ILeafProvider& provider)
    {
        auto& self = this->self();

        auto result = insertBatchToNode(node, pos, provider);

        if (provider.size() == 0)
        {
            return result.local_pos();
        }
        else {
            auto node_size = self.getBranchNodeSize(node);

            NodeBaseG next;

            if (result.local_pos() < node_size)
            {
                next = self.splitPathP(node, result.local_pos());
            }
            else {
                next = self.getNextNodeP(node);
            }

            if (next.isSet())
            {
                auto left_result = insertBatchToNode(node, result.local_pos(), provider);

                if (provider.size() == 0)
                {
                    return left_result.local_pos();
                }
                else {
                    InsertionState state(provider.size());

                    auto next_size0 = self.getBranchNodeSize(next);

                    insert_subtree(node, next, provider, state);

                    auto idx = self.getBranchNodeSize(next) - next_size0;

                    if (provider.size() == 0)
                    {
                        node = next;
                        return idx;
                    }
                    else {
                        return insert_subtree(next, idx, provider);
                    }
                }
            }
            else {
                InsertionState state(provider.size());
                node = insert_subtree_at_end(node, provider, state, 1);

                return self.getBranchNodeSize(node);
            }
        }
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}}
