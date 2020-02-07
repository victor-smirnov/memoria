
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
        int32_t idx() const {return idx_;}
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
            }), MMA_SRC);
        }
    };

    Result<InsertBatchResult> ctr_insert_subtree(TreePathT& path, size_t level, int32_t idx, ILeafProvider& provider, std::function<Result<NodeBaseG> ()> child_fn, bool update_hierarchy) noexcept
    {
        using ResultT = Result<InsertBatchResult>;
        auto& self = this->self();

        NodeBaseG node = path[level];

        int32_t capacity         = self.ctr_get_branch_node_capacity(node, -1ull);
        CtrSizeT provider_size0  = provider.size();
        const int32_t batch_size = 32;

        int32_t max = idx;

        for (int32_t c = 0; c < capacity; c+= batch_size)
        {
            BranchNodeEntryT subtrees[batch_size];

            int32_t i, batch_max = (c + batch_size) < capacity ? batch_size : (capacity - c);
            for (i = 0; i < batch_max && provider.size() > 0; i++)
            {
                MEMORIA_TRY(child, child_fn());

                subtrees[i].accum()     = self.ctr_get_node_max_keys(child);
                subtrees[i].child_id()  = child->id();
            }

            self.branch_dispatcher().dispatch(node, InsertChildrenFn(), idx + c, idx + c + i, subtrees);

            max = idx + c + i;

            if (i < batch_max) {
                break;
            }
        }

        if (update_hierarchy)
        {
            MEMORIA_TRY_VOID(self.ctr_update_path(path, level));
        }

        return ResultT::of(max, provider_size0 - provider.size());
    }


    Result<NodeBaseG> ctr_build_subtree(ILeafProvider& provider, size_t level) noexcept
    {
        using ResultT = Result<NodeBaseG>;
        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                MEMORIA_TRY(node, self.ctr_create_node(level, false, false));
                MEMORIA_TRY_VOID(self.ctr_layout_branch_node(node, 0xFF));

                TreePathT path;
                path.add_root(node);

                auto res = self.ctr_insert_subtree(path, 0, 0, provider, [this, level, &provider]() -> ResultT {
                    auto& self = this->self();
                    return self.ctr_build_subtree(provider, level - 1);
                }, false);
                MEMORIA_RETURN_IF_ERROR(res);

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

        const int32_t& total() const {
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
        return self.ctr_insert_subtree(path, level, idx, provider, [&provider, level, this]() -> Result<NodeBaseG> {
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

        int32_t left_size0 = self.ctr_get_branch_node_size(left_path[level]);

        MEMORIA_TRY(left_result, ctr_insert_batch_to_node(left_path, level, left_size0, provider));

        state.inserted() += left_result.subtree_size();

        if (state.shouldMoveUp())
        {
            if (left_path[level + 1] == right_path[level + 1])
            {
                MEMORIA_TRY(right_parent_idx, self.ctr_get_parent_idx(right_path, level));
                MEMORIA_TRY_VOID(self.ctr_split_path(left_path, level + 1, right_parent_idx));
                // FIXME: what about right_path here?
            }

            MEMORIA_TRY_VOID(ctr_insert_subtree(left_path, right_path, level + 1, provider, state));
        }
        else {
            MEMORIA_TRY(right_result, ctr_insert_batch_to_node(right_path, level, 0, provider));
            state.inserted() += right_result.subtree_size();
        }

        return VoidResult::of();
    }

    VoidResult ctr_insert_subtree_at_end(
            TreePathT& left_path,
            ILeafProvider& provider,
            InsertionState& state,
            size_t level
    ) noexcept
    {
        using ResultT = VoidResult;
        auto& self = this->self();

        int32_t left_size0 = self.ctr_get_branch_node_size(left_path[level]);

        MEMORIA_TRY(left_result, ctr_insert_batch_to_node(left_path, level, left_size0, provider));

        state.inserted() += left_result.subtree_size();

        if (provider.size() > 0)
        {
            if (left_path[level]->is_root())
            {
                MEMORIA_TRY_VOID(self.ctr_create_new_root_block(left_path));
            }

            MEMORIA_TRY_VOID(ctr_insert_subtree_at_end(left_path, provider, state, level + 1));
        }

        return ResultT::of();
    }


    VoidResult ctr_insert_subtree(
            TreePathT& path,
            size_t level,
            int32_t pos,
            ILeafProvider& provider
    ) noexcept
    {
        using ResultT = VoidResult;

        auto& self = this->self();

        MEMORIA_TRY(result, ctr_insert_batch_to_node(path, level, pos, provider));

        if (provider.size() == 0)
        {
            return ResultT::of();
        }
        else {
            auto node_size = self.ctr_get_branch_node_size(path[level]);

            TreePathT next;
            bool has_next_node{};

            if (result.local_pos() < node_size)
            {
                // FIXME: next path
                MEMORIA_TRY_VOID(self.ctr_split_path(path, level, result.local_pos()));

                MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(path, next, level));
                MEMORIA_TRY_VOID(self.ctr_expect_next_node(next, level));

                has_next_node = true;
            }
            else {
                MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(path, next, level));
                MEMORIA_TRY(next_res, self.ctr_get_next_node(next, level));
                has_next_node = next_res;
            }

            if (has_next_node)
            {
                MEMORIA_TRY_VOID(ctr_insert_batch_to_node(path, level, result.local_pos(), provider));

                if (provider.size() == 0)
                {
                    return ResultT::of();
                }
                else {
                    InsertionState state(provider.size());

                    auto next_size0 = self.ctr_get_branch_node_size(next[level]);

                    MEMORIA_TRY_VOID(ctr_insert_subtree(path, next, level, provider, state));

                    auto idx = self.ctr_get_branch_node_size(next[level]) - next_size0;

                    if (provider.size() == 0)
                    {
                        MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(next, path, level));
                        return ResultT::of();
                    }
                    else {
                        return ctr_insert_subtree(next, level, idx, provider);
                    }
                }
            }
            else {
                InsertionState state(provider.size());
                return ctr_insert_subtree_at_end(path, provider, state, 1);
            }
        }
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
