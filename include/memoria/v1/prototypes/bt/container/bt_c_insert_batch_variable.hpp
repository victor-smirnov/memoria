
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

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::InsertBatchVariableName)
public:
    using Types = typename Base::Types;
    using typename Base::Iterator;

protected:
    typedef typename Base::Allocator                                            Allocator;    
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    using typename Base::BlockID;

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

    MEMORIA_V1_DECLARE_NODE_FN_RTN(InsertChildFn, insert, OpStatus);
    Result<InsertBatchResult> ctr_insert_subtree(NodeBaseG& node, int32_t idx, ILeafProvider& provider, std::function<Result<NodeBaseG> ()> child_fn, bool update_hierarchy) noexcept
    {
        using ResultT = Result<InsertBatchResult>;

        auto& self = this->self();

        int32_t batch_size = 32;

        CtrSizeT provider_size0 = provider.size();

        while(batch_size > 0 && provider.size() > 0)
        {
            auto checkpoint = provider.checkpoint();

            BlockUpdateMgr mgr(self);
            MEMORIA_RETURN_IF_ERROR_FN(mgr.add(node));

            int32_t c;

            OpStatus status{OpStatus::OK};

            for (c = 0; c < batch_size && provider.size() > 0; c++)
            {
                Result<NodeBaseG> child_res = child_fn();
                MEMORIA_RETURN_IF_ERROR(child_res);

                NodeBaseG child = child_res.get();

                if (!child.isSet())
                {
                    return ResultT::make_error("Subtree is null");
                }

                child->parent_id()  = node->id();
                child->parent_idx() = idx + c;

                BranchNodeEntry sums = self.ctr_get_node_max_keys(child);
                if(isFail(self.branch_dispatcher().dispatch(node, InsertChildFn(), idx + c, sums, child->id()))) {
                    status = OpStatus::FAIL;
                    break;
                }
            }

            if (isFail(status))
            {
                if (node->level() > 1)
                {
                    auto res = self.ctr_for_all_ids(node, idx, c, [&, this](const BlockID& id, int32_t parent_idx) noexcept -> VoidResult
                    {
                        auto& self = this->self();
                        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_branch_nodes(id));
                        return VoidResult::of();
                    });
                    MEMORIA_RETURN_IF_ERROR(res);
                }

                provider.rollback(checkpoint);
                mgr.rollback();
                batch_size /= 2;
            }
            else {
                idx += c;
            }
        }

        if (update_hierarchy)
        {
            MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_path(node));
            MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_child_indexes(node, idx));
        }

        return ResultT::of(idx, provider_size0 - provider.size());
    }


    Result<NodeBaseG> ctr_build_subtree(ILeafProvider& provider, int32_t level) noexcept
    {
        using ResultT = Result<NodeBaseG>;
        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                Result<NodeBaseG> node = self.ctr_create_node(level, false, false);
                MEMORIA_RETURN_IF_ERROR(node);

                auto res0 = self.ctr_layout_branch_node(node.get(), 0xFF);
                MEMORIA_RETURN_IF_ERROR(res0);

                auto res1 = self.ctr_insert_subtree(node.get(), 0, provider, [this, level, &provider]() noexcept -> Result<NodeBaseG> {
                    auto& self = this->self();
                    return self.ctr_build_subtree(provider, level - 1);
                }, false);
                MEMORIA_RETURN_IF_ERROR(res1);

                return node;
            }
            else {
                return provider.get_leaf();
            }
        }
        else {
            return ResultT::of();
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

        virtual Result<NodeBaseG> get_leaf() noexcept
        {
            using ResultT = Result<NodeBaseG>;

            if (head_.isSet())
            {
                auto node = head_;

                auto res = ctr_.store().getBlock(head_->next_leaf_id(), ctr_.master_name());
                MEMORIA_RETURN_IF_ERROR(res);

                head_ = res.get();
                size_--;
                return node;
            }
            else {
                return ResultT::make_error("Leaf List is empty");
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


    Result<InsertBatchResult> ctr_insert_batch_to_node(NodeBaseG& node, int32_t idx, ILeafProvider& provider, int32_t level = 1, bool update_hierarchy = true) noexcept
    {
        auto& self = this->self();
        return self.ctr_insert_subtree(node, idx, provider, [&provider, &node, this]() noexcept -> Result<NodeBaseG> {
            auto& self = this->self();
            return self.ctr_build_subtree(provider, node->level() - 1);
        },
        update_hierarchy);
    }

    VoidResult ctr_insert_subtree(NodeBaseG& left, NodeBaseG& right, ILeafProvider& provider, InsertionState& state, int32_t level = 1) noexcept
    {
        auto& self = this->self();

        int32_t left_size0 = self.ctr_get_branch_node_size(left);

        auto left_result = ctr_insert_batch_to_node(left, left_size0, provider, level);
        MEMORIA_RETURN_IF_ERROR(left_result);

        state.inserted() += left_result.get().subtree_size();

        if (state.shouldMoveUp())
        {
            auto left_parent = self.ctr_get_node_parent_for_update(left);
            MEMORIA_RETURN_IF_ERROR(left_parent);

            auto right_parent_res = self.ctr_get_node_parent_for_update(right);
            MEMORIA_RETURN_IF_ERROR(right_parent_res);

            NodeBaseG right_parent = right_parent_res.get();

            if (left_parent.get() == right_parent)
            {
                auto res = self.ctr_split_path(left_parent.get(), right->parent_idx());
                MEMORIA_RETURN_IF_ERROR(res);

                right_parent = res.get();
            }

            auto res = ctr_insert_subtree(left_parent.get(), right_parent, provider, state, level + 1);
            MEMORIA_RETURN_IF_ERROR(res);
        }
        else {
            auto right_result = ctr_insert_batch_to_node(right, 0, provider, level);
            MEMORIA_RETURN_IF_ERROR(right_result);

            state.inserted() += right_result.get().subtree_size();
        }

        return VoidResult::of();
    }

    Result<NodeBaseG> ctr_insert_subtree_at_end(NodeBaseG& left, ILeafProvider& provider, InsertionState& state, int32_t level = 1) noexcept
    {
        using ResultT = Result<NodeBaseG>;

        auto& self = this->self();

        int32_t left_size0 = self.ctr_get_branch_node_size(left);

        auto left_result = ctr_insert_batch_to_node(left, left_size0, provider, level);
        MEMORIA_RETURN_IF_ERROR(left_result);

        state.inserted() += left_result.get().subtree_size();

        if (provider.size() > 0)
        {
            if (left->is_root())
            {
                MEMORIA_RETURN_IF_ERROR_FN(self.ctr_create_new_root_block(left));
            }

            auto left_parent = self.ctr_get_node_parent_for_update(left);
            MEMORIA_RETURN_IF_ERROR(left_parent);

            auto right = ctr_insert_subtree_at_end(left_parent.get(), provider, state, level + 1);
            MEMORIA_RETURN_IF_ERROR(right);

            int32_t right_size = self.ctr_get_branch_node_size(right.get());

            return self.ctr_get_node_child(right.get(), right_size - 1);
        }
        else {
            return ResultT::of(left);
        }
    }


    Result<int32_t> ctr_insert_subtree(NodeBaseG& node, int32_t pos, ILeafProvider& provider) noexcept
    {
        auto& self = this->self();

        auto result = ctr_insert_batch_to_node(node, pos, provider);
        MEMORIA_RETURN_IF_ERROR(result);

        if (provider.size() == 0)
        {
            return result.get().local_pos();
        }
        else {
            auto node_size = self.ctr_get_branch_node_size(node);

            NodeBaseG next;

            if (result.get().local_pos() < node_size)
            {
                auto next_res = self.ctr_split_path(node, result.get().local_pos());
                MEMORIA_RETURN_IF_ERROR(next_res);

                next = next_res.get();
            }
            else {
                auto next_res = self.ctr_get_next_node(node);
                MEMORIA_RETURN_IF_ERROR(next_res);

                next = next_res.get();
            }

            if (next.isSet())
            {
                auto left_result = ctr_insert_batch_to_node(node, result.get().local_pos(), provider);
                MEMORIA_RETURN_IF_ERROR(left_result);

                if (provider.size() == 0)
                {
                    return left_result.get().local_pos();
                }
                else {
                    BlockUpdateMgr mgr(self);
                    MEMORIA_RETURN_IF_ERROR_FN(mgr.add(next));

                    auto checkpoint = provider.checkpoint();

                    auto next_result = ctr_insert_batch_to_node(next, 0, provider, 1, false);
                    MEMORIA_RETURN_IF_ERROR(next_result);

                    if (provider.size() == 0)
                    {
                        MEMORIA_RETURN_IF_ERROR(self.ctr_update_path(next));

                        MEMORIA_RETURN_IF_ERROR(self.ctr_update_child_indexes(next, next_result.get().local_pos()));

                        node = next;

                        return next_result.get().local_pos();
                    }
                    else {
                        mgr.rollback();

                        provider.rollback(checkpoint);

                        InsertionState state(provider.size());

                        auto next_size0 = self.ctr_get_branch_node_size(next);

                        auto res0 = ctr_insert_subtree(node, next, provider, state);
                        MEMORIA_RETURN_IF_ERROR(res0);

                        auto idx = self.ctr_get_branch_node_size(next) - next_size0;

                        if (provider.size() == 0)
                        {
                            node = next;
                            return idx;
                        }
                        else {
                            return ctr_insert_subtree(next, idx, provider);
                        }
                    }
                }
            }
            else {
                InsertionState state(provider.size());
                auto end_res = ctr_insert_subtree_at_end(node, provider, state, 1);
                MEMORIA_RETURN_IF_ERROR(end_res);

                node = end_res.get();

                return self.ctr_get_branch_node_size(node);
            }
        }
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}}
