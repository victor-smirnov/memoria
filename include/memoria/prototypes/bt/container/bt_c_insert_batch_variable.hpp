
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

#include <vector>
#include <algorithm>

namespace memoria {

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

    using typename Base::TreePathT;


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
    Result<InsertBatchResult> ctr_insert_subtree(
            TreePathT& path,
            size_t level,
            int32_t idx,
            ILeafProvider& provider,
            std::function<Result<NodeBaseG> ()> child_fn,
            bool update_hierarchy
    ) noexcept
    {
        using ResultT = Result<InsertBatchResult>;
        auto& self = this->self();

        NodeBaseG node = path[level];

        int32_t batch_size = 32;

        CtrSizeT provider_size0 = provider.size();

        NodeBaseG last_child{};

        while(batch_size > 0 && provider.size() > 0)
        {
            auto checkpoint = provider.checkpoint();

            BlockUpdateMgr mgr(self);
            MEMORIA_TRY_VOID(mgr.add(node));

            int32_t c;

            OpStatus status{OpStatus::OK};

            for (c = 0; c < batch_size && provider.size() > 0; c++)
            {
                MEMORIA_TRY(child, child_fn());

                if (!child.isSet())
                {
                    return ResultT::make_error("Subtree is null");
                }

                BranchNodeEntry sums = self.ctr_get_node_max_keys(child);
                if(isFail(self.branch_dispatcher().dispatch(node, InsertChildFn(), idx + c, sums, child->id()))) {
                    status = OpStatus::FAIL;
                    break;
                }

                last_child = child;
            }

            if (isFail(status))
            {
                if (node->level() > 1)
                {
                    auto res = self.ctr_for_all_ids(node, idx, c, [&, this](const BlockID& id) noexcept -> VoidResult
                    {
                        auto& self = this->self();
                        MEMORIA_TRY_VOID(self.ctr_remove_branch_nodes(id));
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

        if (last_child) {
            MEMORIA_TRY_VOID(self.complete_tree_path(path, last_child));
        }

        if (update_hierarchy)
        {
            MEMORIA_TRY_VOID(self.ctr_update_path(path, level));
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
                MEMORIA_TRY(node, self.ctr_create_node(level, false, false));

                auto res0 = self.ctr_layout_branch_node(node, 0xFF);
                MEMORIA_RETURN_IF_ERROR(res0);

                size_t height = level + 1;
                TreePathT path = TreePathT::build(node, height);

                auto res1 = self.ctr_insert_subtree(path, level, 0, provider, [this, level, &provider]() noexcept -> Result<NodeBaseG> {
                    auto& self = this->self();
                    return self.ctr_build_subtree(provider, level - 1);
                }, false);
                MEMORIA_RETURN_IF_ERROR(res1);

                return ResultT::of(node);
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


    Result<InsertBatchResult> ctr_insert_batch_to_node(
            TreePathT& path,
            size_t level,
            int32_t idx,
            ILeafProvider& provider,
            bool update_hierarchy = true
    ) noexcept
    {
        auto& self = this->self();
        return self.ctr_insert_subtree(path, level, idx, provider, [&provider, &level, this]() noexcept -> Result<NodeBaseG> {
            auto& self = this->self();
            return self.ctr_build_subtree(provider, level - 1);
        },
        update_hierarchy);
    }

    VoidResult ctr_insert_subtree(
            TreePathT& left_path,
            TreePathT& right_path,
            size_t level,
            ILeafProvider& provider,
            InsertionState& state
    ) noexcept
    {
        auto& self = this->self();

        NodeBaseG left = left_path[level];
        NodeBaseG right = right_path[level];

        int32_t left_size0 = self.ctr_get_branch_node_size(left);

        auto left_result = ctr_insert_batch_to_node(left_path, level, left_size0, provider);
        MEMORIA_RETURN_IF_ERROR(left_result);

        state.inserted() += left_result.get().subtree_size();

        if (state.shouldMoveUp())
        {
            auto left_parent = self.ctr_get_node_parent_for_update(left_path, level);
            MEMORIA_RETURN_IF_ERROR(left_parent);

            auto right_parent_res = self.ctr_get_node_parent_for_update(right_path, level);
            MEMORIA_RETURN_IF_ERROR(right_parent_res);

            NodeBaseG right_parent = right_parent_res.get();

            if (left_parent.get() == right_parent)
            {
                MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(right_parent, right->id()));

                auto res = self.ctr_split_path(left_path, level + 1, parent_idx);
                MEMORIA_RETURN_IF_ERROR(res);
            }

            auto res = ctr_insert_subtree(left_path, right_path, level + 1, provider, state);
            MEMORIA_RETURN_IF_ERROR(res);
        }
        else {
            auto right_result = ctr_insert_batch_to_node(right_path, level, 0, provider);
            MEMORIA_RETURN_IF_ERROR(right_result);

            state.inserted() += right_result.get().subtree_size();
        }

        return VoidResult::of();
    }

    Result<NodeBaseG> ctr_insert_subtree_at_end(
            TreePathT& left_path,
            size_t level,
            ILeafProvider&
            provider,
            InsertionState& state
    ) noexcept
    {
        using ResultT = Result<NodeBaseG>;
        auto& self = this->self();

        NodeBaseG left = left_path[level];

        int32_t left_size0 = self.ctr_get_branch_node_size(left);

        auto left_result = ctr_insert_batch_to_node(left_path, level, left_size0, provider);
        MEMORIA_RETURN_IF_ERROR(left_result);

        state.inserted() += left_result.get().subtree_size();

        if (provider.size() > 0)
        {
            if (left->is_root())
            {
                MEMORIA_TRY_VOID(self.ctr_create_new_root_block(left_path));
            }

            auto left_parent = self.ctr_get_node_parent_for_update(left_path, level);
            MEMORIA_RETURN_IF_ERROR(left_parent);

            auto right = ctr_insert_subtree_at_end(left_path, level + 1, provider, state);
            MEMORIA_RETURN_IF_ERROR(right);

            int32_t right_size = self.ctr_get_branch_node_size(right.get());

            return self.ctr_get_node_child(right.get(), right_size - 1);
        }
        else {
            return ResultT::of(left);
        }
    }


    Result<int32_t> ctr_insert_subtree(TreePathT& path, size_t level, int32_t pos, ILeafProvider& provider) noexcept
    {
        using ResultT = Result<int32_t>;
        auto& self = this->self();

        NodeBaseG node = path[level];

        MEMORIA_TRY(result, ctr_insert_batch_to_node(path, level, pos, provider));

        if (provider.size() == 0)
        {
            return result.local_pos();
        }
        else {
            auto node_size = self.ctr_get_branch_node_size(node);

            TreePathT next = path;
            bool use_next_leaf{};

            if (result.local_pos() < node_size)
            {
                MEMORIA_TRY_VOID(self.ctr_split_path(path, level, result.local_pos()));
                use_next_leaf = true;

                MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(path, next, level));
                MEMORIA_TRY(has_next, self.ctr_get_next_node(next, level));

                if (!has_next)
                {
                    return ResultT::make_error("No next leaf found in ctr_insert_subtree()");
                }
            }
            else {
                MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(path, next, level));
                MEMORIA_TRY(has_next, self.ctr_get_next_node(next, level));
                use_next_leaf = has_next;
            }

            if (use_next_leaf)
            {
                MEMORIA_TRY(left_result, ctr_insert_batch_to_node(path, level, result.local_pos(), provider));

                if (provider.size() == 0)
                {
                    return left_result.local_pos();
                }
                else {
                    BlockUpdateMgr mgr(self);
                    MEMORIA_TRY_VOID(mgr.add(next[level]));

                    auto checkpoint = provider.checkpoint();

                    MEMORIA_TRY(next_result, ctr_insert_batch_to_node(next, level, 0, provider, false));

                    if (provider.size() == 0)
                    {
                        MEMORIA_TRY_VOID(self.ctr_update_path(next, level));

                        MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(next, path, level));

                        return next_result.local_pos();
                    }
                    else {
                        mgr.rollback();

                        provider.rollback(checkpoint);

                        InsertionState state(provider.size());

                        auto next_size0 = self.ctr_get_branch_node_size(next[level]);

                        MEMORIA_TRY_VOID(ctr_insert_subtree(path, next, level, provider, state));

                        auto idx = self.ctr_get_branch_node_size(next[level]) - next_size0;

                        if (provider.size() == 0)
                        {
                            MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(next, path, level));
                            return ResultT::of(idx);
                        }
                        else {
                            return ctr_insert_subtree(next, level, idx, provider);
                        }
                    }
                }
            }
            else {
                InsertionState state(provider.size());
                MEMORIA_TRY(end_res, ctr_insert_subtree_at_end(path, level, provider, state));
                node = end_res;

                return self.ctr_get_branch_node_size(node);
            }
        }
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
