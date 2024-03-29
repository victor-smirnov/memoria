
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
    using typename Base::CtrSizeT;
    using typename Base::TreePathT;
    using typename Base::Checkpoint;
    using typename Base::ILeafProvider;
    using typename Base::BranchNodeEntry;

public:
    class InsertBatchResult {
        size_t idx_;
        CtrSizeT subtree_size_;
    public:
        InsertBatchResult(size_t idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

        size_t local_pos() const {return idx_;}
        size_t idx() const {return idx_;}

        CtrSizeT subtree_size() const {return subtree_size_;}
    };

    MEMORIA_V1_DECLARE_NODE_FN(InsertChildFn, insert);
    InsertBatchResult ctr_insert_subtree(
            TreePathT& path,
            size_t level,
            size_t idx,
            ILeafProvider& provider,
            std::function<TreeNodePtr ()> child_fn,
            bool update_hierarchy
    )
    {
        auto& self = this->self();

        TreeNodeConstPtr node = path[level];

        CtrSizeT provider_size0 = provider.size();
        TreeNodePtr last_child{};

        size_t c = 0;
        while(provider.size() > 0)
        {
            auto checkpoint = provider.checkpoint();

            auto child = child_fn();
            if (!child.isSet()) {
                MEMORIA_MAKE_GENERIC_ERROR("Subtree is null").do_throw();
            }

            auto sums = self.ctr_get_node_max_keys(child.as_immutable());
            PkdUpdateStatus ins_res = self.branch_dispatcher().dispatch(node.as_mutable(), InsertChildFn(), idx + c, sums, child->id());
            if (is_success(ins_res)) {
                c += 1;
                last_child = child;

                if (child->is_leaf()) {
                    self.ctr_ref_block(child);
                }
            }
            else {
                provider.rollback(checkpoint);
                if (!child->is_leaf()) {
                    self.ctr_remove_branch_nodes(child->id());
                }
                idx += c;
                break;
            }
        }

        if (last_child) {
            self.complete_tree_path(path, last_child.as_immutable());
        }

        if (update_hierarchy) {
            self.ctr_update_path(path, level);
        }

        return InsertBatchResult(idx, provider_size0 - provider.size());
    }


    TreeNodePtr ctr_build_subtree(ILeafProvider& provider, size_t level)
    {
        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                auto node = self.ctr_create_node(level, false, false);
                self.ctr_ref_block(node);

                size_t height = level + 1;
                TreePathT path = TreePathT::build(node.as_immutable(), height);

                self.ctr_insert_subtree(path, level, 0, provider, [this, level, &provider]() {
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
        ListLeafProvider(MyType& ctr, const TreeNodePtr& head, CtrSizeT size): head_(head),  size_(size), ctr_(ctr) {}

        virtual CtrSizeT size() const
        {
            return size_;
        }

        virtual TreeNodePtr get_leaf()
        {
            if (head_.isSet())
            {
                auto node = head_;

                auto res = ctr_.store().getBlock(head_->next_leaf_id(), ctr_.master_name());

                head_ = res;
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
            size_t idx,
            ILeafProvider& provider,
            bool update_hierarchy = true
    )
    {
        auto& self = this->self();
        return self.ctr_insert_subtree(path, level, idx, provider, [&provider, &level, this]() {
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
