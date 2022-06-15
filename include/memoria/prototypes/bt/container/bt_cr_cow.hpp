
// Copyright 2020 Victor Smirnov
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
#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/container/macros.hpp>


#include <vector>
#include <utility>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::CoWOpsRName)

    using typename Base::ApiProfileT;

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::TreePathT;
    using typename Base::SnapshotID;
    using typename Base::BlockID;
    using typename Base::Profile;
    using typename Base::BlockType;
    using typename Base::ContainerTypeName;

    MEMORIA_V1_DECLARE_NODE_FN(UnrefLeafChildren, cow_unref_children);

    void ctr_unref_block(const BlockID& block_id)
    {
        auto& self = this->self();

        auto this_ptr = memoria_static_pointer_cast<MyType>(this->shared_from_this());
        return self.store().unref_block(block_id, [&]() {
            auto block = this_ptr->ctr_get_block(block_id);

            if (!block->is_leaf())
            {
                this_ptr->ctr_for_all_ids(block, [&](const BlockID& child_id) {
                    return this_ptr->ctr_unref_block(child_id);
                });
            }
            else {
                this_ptr->leaf_dispatcher().dispatch(block, UnrefLeafChildren(), this_ptr->store());
            }

            return this_ptr->store().removeBlock(block->id());
        });
    }

    void ctr_unref_block_cascade(const BlockID& block_id)
    {
        auto& self = this->self();

        auto block = self.ctr_get_block(block_id);
        if (!block->is_leaf())
        {
            self.ctr_for_all_ids(block, [&](const BlockID& block_id) {
                return self.ctr_unref_block(block_id);
            });
        }
        else {
            self.leaf_dispatcher().dispatch(block, UnrefLeafChildren(), self.store());
        }

        return self.store().removeBlock(block->id());
    }

    virtual void internal_unref_cascade(const AnyID& root_block_id_api)
    {
        BlockID root_block_id = cast_to<BlockID>(root_block_id_api);
        return self().ctr_unref_block_cascade(root_block_id);
    }


    void traverse_ctr(
            void* node_handler_ptr
    ) const
    {
        auto& self = this->self();

        BTreeTraverseNodeHandler<Profile>* node_handler = ptr_cast<BTreeTraverseNodeHandler<Profile>>(node_handler_ptr);

        auto root = self.ctr_get_root_node();
        return ctr_do_cow_traverse_tree(*node_handler, root);
    }

private:

    void ctr_do_cow_traverse_tree(BTreeTraverseNodeHandler<Profile>& node_handler, const TreeNodeConstPtr& node) const
    {
        auto& self = this->self();

        constexpr bool is_ctr_directory = bt::CtrDirectoryHelper<ContainerTypeName>::Value;

        if (node->is_leaf())
        {
            if (!is_ctr_directory) {
                return node_handler.process_leaf_node(&node.block()->header());
            }
            else {
                node_handler.process_directory_leaf_node(&node.block()->header());
                return self.ctr_for_all_leaf_ctr_refs(node, [&](const BlockID& id)
                {
                    auto proceed_with = node_handler.proceed_with(id);
                    if (proceed_with){
                        return self.store().traverse_ctr(id, node_handler);
                    }
                });
            }
        }
        else {
            node_handler.process_branch_node(&node.block()->header());
            return self.ctr_for_all_ids(node, [&](const BlockID& id)
            {
                auto proceed_with = node_handler.proceed_with(id);
                if (proceed_with)
                {
                    auto child = self.ctr_get_block(id);
                    return self.ctr_do_cow_traverse_tree(node_handler, child);
                }
            });
        }
    }


    MEMORIA_V1_DECLARE_NODE_FN(ForAllCtrRooIDsFn, for_all_ctr_root_ids);
    void ctr_for_all_leaf_ctr_refs(const TreeNodeConstPtr& node, const std::function<void (const BlockID&)>& fn) const
    {
        return self().leaf_dispatcher().dispatch(node, ForAllCtrRooIDsFn(), fn);
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::CoWOpsRName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
