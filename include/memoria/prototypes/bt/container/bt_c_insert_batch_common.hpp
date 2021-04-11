
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

    using typename Base::BlockID;
    using typename Base::NodeBaseG;
    using typename Base::Position;
    using typename Base::CtrSizeT;

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

    class InsertBatchResult {
        int32_t idx_;
        CtrSizeT subtree_size_;
    public:
        InsertBatchResult(int32_t idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

        int32_t local_pos() const {return idx_;}
        int32_t idx() const {return idx_;}
        CtrSizeT subtree_size() const {return subtree_size_;}
    };

    VoidResult ctr_remove_branch_nodes(const BlockID& node_id) noexcept
    {
        auto& self = this->self();
        return self.ctr_unref_block(node_id);
    }




    Result<NodeBaseG> ctr_build_subtree(ILeafProvider& provider, int32_t level) noexcept
    {
        using ResultT = Result<NodeBaseG>;

        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                MEMORIA_TRY(node, self.ctr_create_node1(level, false, false));
                MEMORIA_TRY_VOID(self.ctr_ref_block(node->id()));

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

                MEMORIA_TRY(block, ctr_.store().getBlock(head_->next_leaf_id()));

                head_ = block;
                size_--;
                return ResultT::of(std::move(node));
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Leaf List is empty");
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


    class LeafList {
        CtrSizeT size_;
        NodeBaseG head_;
        NodeBaseG tail_;
    public:
        LeafList(CtrSizeT size, NodeBaseG head, NodeBaseG tail): size_(size), head_(head), tail_(tail) {}

        CtrSizeT size() const {return size_;}
        const NodeBaseG& head() const {return head_;}
        const NodeBaseG& tail() const {return tail_;}

        NodeBaseG& head() {return head_;}
        NodeBaseG& tail() {return tail_;}
    };






    template <typename Provider>
    Result<Position> ctr_insert_data_into_leaf(TreePathT& path, const Position& pos, Provider& provider)
    {
        using ResultT = Result<Position>;
        auto& self = this->self();

        MEMORIA_TRY(has_data, provider.hasData());
        if (has_data)
        {
            NodeBaseG leaf = path.leaf();
            MEMORIA_TRY(end_pos, ctr_insert_data_into_leaf(leaf, pos, provider));

            if ((end_pos - pos).sum() > 0) {
                MEMORIA_TRY_VOID(self.ctr_update_path(path, 0));
            }

            return ResultT::of(end_pos);
        }

        return ResultT::of(pos);
    }

    template <typename Provider>
    Result<Position> ctr_insert_data_into_leaf(NodeBaseG& leaf, const Position& pos, Provider& provider)
    {
        using ResultT = Result<Position>;
        auto& self = this->self();

        MEMORIA_TRY(has_data, provider.hasData());
        if (has_data)
        {
            MEMORIA_TRY_VOID(self.ctr_update_block_guard(leaf));
            MEMORIA_TRY_VOID(self.ctr_layout_leaf_node(leaf, Position(0)));

            MEMORIA_TRY(end, provider.fill(leaf, pos));

            return ResultT::of(end);
        }

        return ResultT::of(pos);
    }





    template <typename Provider>
    VoidResult ctr_insert_provided_data(TreePathT& path, Position& pos, Provider& provider) noexcept
    {
        using ResultT = VoidResult;
        auto& self = this->self();

        MEMORIA_TRY(last_pos, self.ctr_insert_data_into_leaf(path, pos, provider));

        MEMORIA_TRY(has_data, provider.hasData());
        if (has_data)
        {
            // has to be defined in subclasses
            MEMORIA_TRY(at_end, self.ctr_is_at_the_end(path.leaf(), last_pos));
            if (!at_end)
            {
                MEMORIA_TRY_VOID(self.ctr_split_leaf(path, last_pos));

                MEMORIA_TRY(last_leaf_pos, self.ctr_insert_data_into_leaf(path, last_pos, provider));
                MEMORIA_TRY(has_data2, provider.hasData());

                if (has_data2)
                {
                    return ctr_insert_data_rest(path, pos, provider);
                }
                else {
                    pos = last_leaf_pos;
                }
            }
            else {
                return ctr_insert_data_rest(path, pos, provider);
            }
        }
        else {
            pos = last_pos;
        }

        return ResultT::of();
    }



    template <typename Provider>
    Result<LeafList> ctr_create_leaf_data_list(Provider& provider) noexcept
    {
        using ResultT = Result<LeafList>;
        auto& self = this->self();

        CtrSizeT    total = 0;
        NodeBaseG   head;
        NodeBaseG   current;

        MEMORIA_TRY(meta, self.ctr_get_root_metadata());
        int32_t block_size = meta.memory_block_size();

        while (true)
        {
            MEMORIA_TRY(has_data, provider.hasData());

            if (!has_data) {
                break;
            }

            MEMORIA_TRY(node, self.ctr_create_node(0, false, true, block_size));

            MEMORIA_TRY_VOID(self.ctr_ref_block(node->id()));

            if (head.isSet())
            {
                current->next_leaf_id() = node->id();
            }
            else {
                head = node;
            }

            MEMORIA_TRY_VOID(self.ctr_insert_data_into_leaf(node, Position(), provider));
            MEMORIA_TRY_VOID(provider.iter_next_leaf(node));

            current = node;
            total++;
        }

        return ResultT::of(total, head, current);
    }




    template <typename Provider>
    VoidResult ctr_insert_data_rest(TreePathT& path, Position& end_pos, Provider& provider) noexcept
    {
        using ResultT = VoidResult;
        auto& self = this->self();

        MEMORIA_TRY(leaf_list, self.ctr_create_leaf_data_list(provider));

        if (leaf_list.size() > 0)
        {
            ListLeafProvider list_provider(self, leaf_list.head(), leaf_list.size());
            BatchInsertionState insertion_state(leaf_list.size());

            if (path.leaf()->is_root())
            {
                MEMORIA_TRY_VOID(self.ctr_create_new_root_block(path));
            }

            MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(path, 0));
            int32_t last_insertion_idx{};

            while (insertion_state.has_more())
            {

                MEMORIA_TRY(insertion_idx, self.ctr_insert_subtree_one_pass(
                                path,
                                1,
                                parent_idx + 1,
                                list_provider,
                                insertion_state
                ));

                last_insertion_idx = insertion_idx;
                parent_idx = insertion_idx - 1;

                insertion_state.next_pass();
            }

            MEMORIA_TRY(parent_size, self.ctr_get_branch_node_size(path[1]));
            if (last_insertion_idx < parent_size)
            {
                MEMORIA_TRY(child, self.ctr_get_node_child(path[1], last_insertion_idx));
                path[0] = child;

                TreePathT prev_path = path;
                MEMORIA_TRY_VOID(self.ctr_expect_prev_node(prev_path, 0));

                MEMORIA_TRY(
                            has_merge,
                            self.ctr_merge_leaf_nodes(prev_path, path);
                );

                if (has_merge) {
                    path = prev_path;
                }
            }
            else {
                MEMORIA_TRY(child, self.ctr_get_node_child(path[1], last_insertion_idx - 1));
                path[0] = child;
            }
        }

        return ResultT::of();
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS




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
            MEMORIA_TRY(node_size, self.ctr_get_branch_node_size(path[level]));
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

            MEMORIA_TRY(parent_size, self.ctr_get_branch_node_size(path[level + 1]));
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
                return self.ctr_get_branch_node_size(child);
            }
        }
        else {
            int32_t last_idx = insertion_result.idx();
            MEMORIA_TRY(node_size, self.ctr_get_branch_node_size(path[level]));

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
