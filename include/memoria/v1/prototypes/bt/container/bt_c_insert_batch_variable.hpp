
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

using namespace v1::bt;
using namespace v1::core;

using namespace std;

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bt::InsertBatchVariableName)
public:
    using typename Base::Types;
    using typename Base::Iterator;

protected:
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;


    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;


    using Checkpoint    = typename Base::Checkpoint;
    using ILeafProvider = typename Base::ILeafProvider;

    class InsertBatchResult {
        Int idx_;
        CtrSizeT subtree_size_;
    public:
        InsertBatchResult(Int idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

        Int idx() const {return idx_;}
        CtrSizeT subtree_size() const {return subtree_size_;}
    };

    MEMORIA_V1_DECLARE_NODE_FN(InsertChildFn, insert);
    InsertBatchResult insertSubtree(NodeBaseG& node, Int idx, ILeafProvider& provider, std::function<NodeBaseG ()> child_fn, bool update_hierarchy)
    {
        auto& self = this->self();

        Int batch_size = 32;

        CtrSizeT provider_size0 = provider.size();

        while(batch_size > 0 && provider.size() > 0)
        {
            auto checkpoint = provider.checkpoint();

            PageUpdateMgr mgr(self);
            mgr.add(node);

            Int c;

            try {
                for (c = 0; c < batch_size && provider.size() > 0; c++)
                {
                    auto child = child_fn();

                    if (!child.isSet())
                    {
                        throw NullPointerException(MA_SRC, "Subtree is null");
                    }

                    child->parent_id()  = node->id();
                    child->parent_idx() = idx + c;

                    BranchNodeEntry sums = self.max(child);
                    BranchDispatcher::dispatch(node, InsertChildFn(), idx + c, sums, child->id());
                }

                idx += c;
            }
            catch (PackedOOMException& ex)
            {
                if (node->level() > 1)
                {
                    self.forAllIDs(node, idx, c, [&, this](const ID& id, Int parent_idx)
                    {
                        auto& self = this->self();
                        self.remove_branch_nodes(id);
                    });
                }

                provider.rollback(checkpoint);
                mgr.rollback();
                batch_size /= 2;
            }
        }

        if (update_hierarchy)
        {
            BranchNodeEntry sums = self.max(node);
            self.update_parent(node, sums);
            self.updateChildIndexes(node, idx);
        }

        return InsertBatchResult(idx, provider_size0 - provider.size());
    }


    NodeBaseG BuildSubtree(ILeafProvider& provider, Int level)
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
                head_ = ctr_.allocator().getPage(head_->next_leaf_id(), ctr_.master_name());
                size_--;
                return node;
            }
            else {
                throw v1::BoundsException(MA_SRC, "Leaf List is empty");
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
        Int inserted_ = 0;
        Int total_;
    public:
        InsertionState(Int total): total_(total) {}

        Int& total() {
            return total_;
        }

        Int& inserted() {
            return inserted_;
        }

        bool shouldMoveUp() const {
            return inserted_ <= total_ / 3;
        }
    };


    InsertBatchResult insertBatchToNode(NodeBaseG& node, Int idx, ILeafProvider& provider, Int level = 1, bool update_hierarchy = true)
    {
        auto& self = this->self();
        return self.insertSubtree(node, idx, provider, [&provider, &node, this]() -> NodeBaseG {
            auto& self = this->self();
            return self.BuildSubtree(provider, node->level() - 1);
        },
        update_hierarchy);
    }

    void insert_subtree(NodeBaseG& left, NodeBaseG& right, ILeafProvider& provider, InsertionState& state, Int level = 1)
    {
        auto& self = this->self();

        Int left_size0 = self.getBranchNodeSize(left);

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

    NodeBaseG insert_subtree_at_end(NodeBaseG& left, ILeafProvider& provider, InsertionState& state, Int level = 1)
    {
        auto& self = this->self();

        Int left_size0 = self.getBranchNodeSize(left);

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

            Int right_size = self.getBranchNodeSize(right);

            return self.getChild(right, right_size - 1);
        }
        else {
            return left;
        }
    }


    Int insert_subtree(NodeBaseG& node, Int pos, ILeafProvider& provider)
    {
        auto& self = this->self();

        auto result = insertBatchToNode(node, pos, provider);

        if (provider.size() == 0)
        {
            return result.idx();
        }
        else {
            auto node_size = self.getBranchNodeSize(node);

            NodeBaseG next;

            if (result.idx() < node_size)
            {
                next = self.splitPathP(node, result.idx());
            }
            else {
                next = self.getNextNodeP(node);
            }

            if (next.isSet())
            {
                auto left_result = insertBatchToNode(node, result.idx(), provider);

                if (provider.size() == 0)
                {
                    return left_result.idx();
                }
                else {
                    PageUpdateMgr mgr(self);
                    mgr.add(next);

                    auto checkpoint = provider.checkpoint();

                    auto next_result = insertBatchToNode(next, 0, provider, 1, false);

                    if (provider.size() == 0)
                    {
                        auto sums = self.max(next);
                        self.update_parent(next, sums);

                        self.updateChildIndexes(next, next_result.idx());

                        node = next;

                        return next_result.idx();
                    }
                    else {
                        mgr.rollback();

                        provider.rollback(checkpoint);

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
            }
            else {
                InsertionState state(provider.size());
                node = insert_subtree_at_end(node, provider, state, 1);

                return self.getBranchNodeSize(node);
            }
        }
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bt::InsertBatchVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}}