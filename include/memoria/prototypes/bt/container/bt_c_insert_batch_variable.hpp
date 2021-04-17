
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

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::BlockID;
    using typename Base::BlockUpdateMgr;
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

    MEMORIA_V1_DECLARE_NODE_FN(InsertChildFn, insert);
    Result<InsertBatchResult> ctr_insert_subtree(
            TreePathT& path,
            size_t level,
            int32_t idx,
            ILeafProvider& provider,
            std::function<Result<TreeNodePtr> ()> child_fn,
            bool update_hierarchy
    ) noexcept
    {
        using ResultT = Result<InsertBatchResult>;
        auto& self = this->self();

        TreeNodeConstPtr node = path[level];

        int32_t batch_size = 32;

        CtrSizeT provider_size0 = provider.size();

        TreeNodePtr last_child{};

        while(batch_size > 0 && provider.size() > 0)
        {
            auto checkpoint = provider.checkpoint();

            BlockUpdateMgr mgr(self);
            MEMORIA_TRY_VOID(mgr.add(node.as_mutable()));

            int32_t c;

            bool insertion_status{true};

            for (c = 0; c < batch_size && provider.size() > 0; c++)
            {
                MEMORIA_TRY(child, child_fn());

                if (!child.isSet())
                {
                    return MEMORIA_MAKE_GENERIC_ERROR("Subtree is null");
                }

                MEMORIA_TRY(sums, self.ctr_get_node_max_keys(child.as_immutable()));

                VoidResult ins_res = self.branch_dispatcher().dispatch(node.as_mutable(), InsertChildFn(), idx + c, sums, child->id());

                if(ins_res.is_error())
                {
                    if (ins_res.is_packed_error())
                    {
                        insertion_status = false;
                        break;
                    }
                    else {
                        return MEMORIA_PROPAGATE_ERROR(ins_res);
                    }
                }

                //MEMORIA_TRY_VOID(self.ctr_ref_block(child->id()));

                last_child = child;
            }

            if (!insertion_status)
            {
                if (node->level() > 1)
                {
                    MEMORIA_TRY_VOID(self.ctr_for_all_ids(node, idx, c, [&](const BlockID& id) noexcept -> VoidResult
                    {
                        return self.ctr_remove_branch_nodes(id);
                    }));
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
            MEMORIA_TRY_VOID(self.complete_tree_path(path, last_child.as_immutable()));
        }

        if (update_hierarchy)
        {
            MEMORIA_TRY_VOID(self.ctr_update_path(path, level));
        }

        return ResultT::of(idx, provider_size0 - provider.size());
    }


    Result<TreeNodePtr> ctr_build_subtree(ILeafProvider& provider, int32_t level) noexcept
    {
        using ResultT = Result<TreeNodePtr>;
        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                MEMORIA_TRY(node, self.ctr_create_node(level, false, false));

                MEMORIA_TRY_VOID(self.ctr_ref_block(node->id()));

                MEMORIA_TRY_VOID(self.ctr_layout_branch_node(node, 0xFF));

                size_t height = level + 1;
                TreePathT path = TreePathT::build(node.as_immutable(), height);

                MEMORIA_TRY_VOID(self.ctr_insert_subtree(path, level, 0, provider, [this, level, &provider]() noexcept -> Result<TreeNodePtr> {
                    auto& self = this->self();
                    return self.ctr_build_subtree(provider, level - 1);
                }, false));

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
        TreeNodePtr   head_;
        CtrSizeT    size_ = 0;

        MyType&     ctr_;

    public:
        ListLeafProvider(MyType& ctr, const TreeNodePtr& head, CtrSizeT size): head_(head),  size_(size), ctr_(ctr) {}

        virtual CtrSizeT size() const
        {
            return size_;
        }

        virtual Result<TreeNodePtr> get_leaf() noexcept
        {
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
        return self.ctr_insert_subtree(path, level, idx, provider, [&provider, &level, this]() noexcept -> Result<TreeNodePtr> {
            auto& self = this->self();
            return self.ctr_build_subtree(provider, level - 1);
        },
        update_hierarchy);
    }




MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
