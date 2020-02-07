
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

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::InsertBatchCommonName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    using typename Base::BlockID;
    
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    using typename Base::TreePathT;

    class Checkpoint {
        NodeBaseG head_;
        int32_t size_;
    public:
        Checkpoint(NodeBaseG head, int32_t size): head_(head), size_(size) {}

        NodeBaseG head() const {return head_;}
        int32_t size() const {return size_;}
    };


    struct ILeafProvider {
        virtual Result<NodeBaseG> get_leaf() noexcept = 0;

        virtual Checkpoint checkpoint() = 0;

        virtual void rollback(const Checkpoint& chekpoint) = 0;

        virtual CtrSizeT size() const = 0;
    };

    VoidResult ctr_remove_branch_nodes(const BlockID& node_id) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(node, self.ctr_get_block(node_id));
        if (node->level() > 0)
        {
            auto res = self.ctr_for_all_ids(node, [&, this](const BlockID& id)
            {
                auto& self = this->self();
                return self.ctr_remove_branch_nodes(id);
            });
            MEMORIA_RETURN_IF_ERROR(res);

            return self.store().removeBlock(node->id());
        }

        return VoidResult::of();
    }

    class InsertBatchResult {
        int32_t idx_;
        CtrSizeT subtree_size_;
    public:
        InsertBatchResult(int32_t idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

        int32_t local_pos() const {return idx_;}
        int32_t idx() const {return idx_;}
        CtrSizeT subtree_size() const {return subtree_size_;}
    };


    Result<NodeBaseG> ctr_build_subtree(ILeafProvider& provider, int32_t level) noexcept
    {
        using ResultT = Result<NodeBaseG>;

        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                MEMORIA_TRY(node, self.ctr_create_node1(level, false, false));

                self.layoutNonLeafNode(node, 0xFF);

                MEMORIA_TRY_VOID(self.ctr_insert_subtree(node, 0, provider, [this, level, &provider]() -> ResultT {
                    auto& self = this->self();
                    return self.ctr_build_subtree(provider, level - 1);
                }, false));

                return node_result;
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

                auto res = ctr_.store().getBlock(head_->next_leaf_id());
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

    MEMORIA_V1_DECLARE_NODE_FN_RTN(IsEmptyFn, ctr_is_empty, bool);
    bool ctr_is_empty(const NodeBaseG& node) noexcept {
        return self().node_dispatcher().dispatch(node, IsEmptyFn());
    }


    class BatchInsertionState {
        int64_t inserted_ = 0;
        int64_t total_;
    public:
        BatchInsertionState(int64_t total): total_(total) {}

        int64_t total() const {
            return total_;
        }

        int64_t& inserted() {
            return inserted_;
        }

        bool shouldMoveUp() const {
            return inserted_ <= total_ / 3;
        }

        bool has_more() const {
            return inserted_ < total_;
        }

        void next_pass()
        {
            total_ -= inserted_;
            inserted_ = 0;
        }
    };



    Int32Result ctr_insert_subtree_one_pass(
            TreePathT& path,
            size_t level,
            int32_t idx,
            ILeafProvider& leaf_provider,
            BatchInsertionState& state
    ) noexcept;

    Result<NodeBaseG> createNextLeaf(NodeBaseG& left_node) noexcept;

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
Result<typename M_TYPE::NodeBaseG> M_TYPE::createNextLeaf(NodeBaseG& left_node) noexcept
{
    using ResultT = Result<NodeBaseG>;

    auto& self = this->self();

    if (left_node->is_root())
    {
        self.ctr_create_new_root_block(left_node);
    }
    else {
        self.ctr_update_block_guard(left_node);
    }

    NodeBaseG left_parent = self.ctr_get_node_parent_for_update(left_node);

    NodeBaseG other  = self.ctr_create_node1(left_node->level(), false, left_node->is_leaf(), left_node->memory_block_size());

    other->next_leaf_id().clear();

    ListLeafProvider provider(self, other, 1);

    self.ctr_insert_subtree(left_parent, left_node->parent_idx() + 1, provider);

    return ResultT::of(other);
}

M_PARAMS
Int32Result M_TYPE::ctr_insert_subtree_one_pass(
        TreePathT& path,
        size_t level,
        int32_t idx,
        ILeafProvider& leaf_provider,
        BatchInsertionState& state
) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY(insertion_result, self.ctr_insert_batch_to_node(path, level, idx, leaf_provider));
    state.inserted() += insertion_result.subtree_size();

    if (state.has_more())
    {
        if (state.shouldMoveUp())
        {
            int32_t node_size = self.ctr_get_branch_node_size(path[level]);
            if (insertion_result.idx() < node_size)
            {
                MEMORIA_TRY_VOID(self.ctr_split_path(path, level, insertion_result.idx()));
                MEMORIA_TRY(insertion_result5, self.ctr_insert_batch_to_node(path, level, insertion_result.idx(), leaf_provider));
                state.inserted() += insertion_result5.subtree_size();

                if (!state.has_more()) {
                    return Int32Result::of(insertion_result5.idx());
                }
            }
            else if (path[level]->is_root())
            {
                 MEMORIA_TRY_VOID(self.ctr_create_new_root_block(path));
            }

            MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(path, level));
            MEMORIA_TRY(
                        parent_insertion_result,
                        self.ctr_insert_subtree_one_pass(
                            path,
                            level + 1,
                            parent_idx + 1,
                            leaf_provider,
                            state
                        )
            );

            int32_t parent_size = self.ctr_get_branch_node_size(path[level + 1]);
            if (parent_insertion_result < parent_size)
            {
                MEMORIA_TRY(child, self.ctr_get_node_child(path[level + 1], parent_insertion_result));
                path[level] = child;

                if (state.has_more())
                {
                    MEMORIA_TRY(insertion_result4, self.ctr_insert_batch_to_node(path, level, 0, leaf_provider));
                    state.inserted() += insertion_result4.subtree_size();

                    return Int32Result::of(insertion_result4.idx());
                }
                else {
                    return Int32Result::of(0);
                }
            }
            else {
                MEMORIA_TRY(child, self.ctr_get_node_child(path[level + 1], parent_insertion_result - 1));
                path[level] = child;
                return Int32Result::of(self.ctr_get_branch_node_size(child));
            }
        }
        else {
            int32_t last_idx = insertion_result.idx();
            int32_t node_size = self.ctr_get_branch_node_size(path[level]);

            if (last_idx < node_size)
            {
                MEMORIA_TRY_VOID(self.ctr_split_path(path, level, last_idx));

                MEMORIA_TRY(insertion_result2, self.ctr_insert_batch_to_node(path, level, last_idx, leaf_provider));
                state.inserted() += insertion_result2.subtree_size();

                if (state.has_more())
                {
                    MEMORIA_TRY_VOID(self.ctr_expect_next_node(path, level));
                    MEMORIA_TRY(insertion_result3, self.ctr_insert_batch_to_node(path, level, 0, leaf_provider));
                    state.inserted() += insertion_result3.subtree_size();

                    return Int32Result::of(insertion_result3.idx());
                }
                else {
                    return Int32Result::of(insertion_result2.idx());
                }
            }
            else {
                MEMORIA_TRY_VOID(self.ctr_split_path(path, level, last_idx));
                MEMORIA_TRY_VOID(self.ctr_expect_next_node(path, level));

                MEMORIA_TRY(insertion_result6, self.ctr_insert_batch_to_node(path, level, 0, leaf_provider));
                state.inserted() += insertion_result6.subtree_size();

                return Int32Result::of(insertion_result6.idx());
            }
        }
    }
    else {
        return Int32Result::of(insertion_result.idx());
    }
}

#undef M_TYPE
#undef M_PARAMS

}
