
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
    using typename Base::NodeBaseG;
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
        NodeBaseG node_;
    public:
        BranchNodeEntryT(const BranchNodeEntryT& accum, const BlockID& id, NodeBaseG node) noexcept:
            accum_(accum), child_id_(id)
        {}

        BranchNodeEntryT() noexcept: accum_(), child_id_(), node_()
        {}

        const BranchNodeEntry& accum() const noexcept {return accum_;}
        const BlockID& child_id() const noexcept {return child_id_;}

        BranchNodeEntry& accum() noexcept {return accum_;}
        BlockID& child_id() noexcept {return child_id_;}
        NodeBaseG& child_node() noexcept {return node_;}
    };


    struct InsertChildrenFn {
        template <typename CtrT, typename NodeT>
        VoidResult treeNode(BranchNodeSO<CtrT, NodeT>& node, int32_t from, int32_t to, const BranchNodeEntryT* entries) noexcept
        {
            MEMORIA_TRY(old_size, node.size());

            MEMORIA_TRY_VOID(node.processAll(*this, from, to, entries));

            int32_t idx = 0;
            return node.insertValues(old_size, from, to - from, [entries, &idx](){
                return entries[idx++].child_id();
            });
        }

        template <int32_t ListIdx, typename StreamType>
        VoidResult stream(StreamType& obj, int32_t from, int32_t to, const BranchNodeEntryT* entries) noexcept
        {
            return obj.insert_entries(from, to - from, [entries](int32_t column, int32_t idx) noexcept -> const auto& {
                return std::get<ListIdx>(entries[idx].accum())[column];
            });
        }
    };

    Result<InsertBatchResult> ctr_insert_subtree(TreePathT& path, size_t level, int32_t idx, ILeafProvider& provider, std::function<Result<NodeBaseG> ()> child_fn, bool update_hierarchy) noexcept
    {
        using ResultT = Result<InsertBatchResult>;
        auto& self = this->self();

        NodeBaseG node = path[level];

        MEMORIA_TRY(capacity, self.ctr_get_branch_node_capacity(node, -1ull));
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

                MEMORIA_TRY(max, self.ctr_get_node_max_keys(child));
                subtrees[i].accum()         = max;
                subtrees[i].child_id()      = child->id();
                subtrees[i].child_node()    = child;
            }

            MEMORIA_TRY_VOID(self.branch_dispatcher().dispatch(node, InsertChildrenFn(), idx + c, idx + c + i, subtrees));

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
            if (head_.isSet())
            {
                auto node = head_;

                MEMORIA_TRY(block, ctr_.store().getBlock(head_->next_leaf_id(), ctr_.master_name()));

                head_ = block;
                size_--;
                return node;
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






MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
