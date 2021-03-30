
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

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::CoWOpsName)

    using typename Base::ApiProfileT;

    using typename Base::NodeBaseG;
    using typename Base::TreePathT;
    using typename Base::SnapshotID;
    using typename Base::BlockID;
    using typename Base::Profile;
    using typename Base::BlockType;
    using typename Base::ContainerTypeName;

    bool ctr_is_mutable_node(const NodeBaseG& node) const noexcept {
        return node->header().snapshot_id() == self().snapshot_id();
    }


    VoidResult ctr_cow_clone_path(TreePathT& path, size_t level) noexcept
    {
        auto& self = this->self();

        size_t path_size = path.size();

        if (level < path_size && !self.ctr_is_mutable_node(path[level]))
        {
            MEMORIA_TRY_VOID(ctr_cow_clone_path(path, level + 1));

            MEMORIA_TRY(new_node, ctr_clone_block(path[level]));

            if (level + 1 < path_size)
            {
                MEMORIA_TRY_VOID(self.ctr_ref_block(new_node->id()));

                MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(path, level));
                MEMORIA_TRY(old_child_id, self.ctr_set_child_id(path[level + 1], parent_idx, new_node->id()));

                MEMORIA_TRY_VOID(self.ctr_unref_block(old_child_id));
            }
            else {
                MEMORIA_TRY_VOID(self.set_root(new_node->id()));
            }

            path[level] = new_node;
        }

        return VoidResult::of();
    }




    MEMORIA_V1_DECLARE_NODE_FN(UnrefLeafChildren, cow_unref_children);
    VoidResult ctr_unref_block(const BlockID& block_id) noexcept
    {
        auto& self = this->self();

        auto this_ptr = this->shared_from_this();        
        return self.store().unref_block(block_id, [=]() -> VoidResult {

            MEMORIA_TRY(block, this_ptr->ctr_get_block(block_id));

            if (!block->is_leaf())
            {
                MEMORIA_TRY_VOID(this_ptr->ctr_for_all_ids(block, [&](const BlockID& child_id) noexcept {
                    return this_ptr->ctr_unref_block(child_id);
                }));
            }
            else {
                MEMORIA_TRY_VOID(this_ptr->leaf_dispatcher().dispatch(block, UnrefLeafChildren(), this_ptr->store()));
            }

            return this_ptr->store().removeBlock(block->id());
        });
    }


    VoidResult ctr_unref_block_cascade(const BlockID& block_id) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(block, self.ctr_get_block(block_id));

        if (!block->is_leaf())
        {
            MEMORIA_TRY_VOID(self.ctr_for_all_ids(block, [&](const BlockID& block_id) noexcept {
                return self.ctr_unref_block(block_id);
            }));
        }
        else {
            MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch(block, UnrefLeafChildren(), self.store()));
        }

        return self.store().removeBlock(block->id());
    }





    VoidResult ctr_ref_block(const BlockID& block_id) noexcept
    {
        auto& self = this->self();
        return  self.store().ref_block(block_id);
    }



    MEMORIA_V1_DECLARE_NODE_FN(RefLeafChildren, cow_ref_children);
    Result<NodeBaseG> ctr_clone_block(const NodeBaseG& src) noexcept
    {
        using ResultT = Result<NodeBaseG>;
        auto& self = this->self();

        MEMORIA_TRY(new_block_tmp, self.store().cloneBlock(src));
        NodeBaseG new_block = new_block_tmp;

        if (!new_block->is_leaf())
        {
            MEMORIA_TRY_VOID(self.ctr_for_all_ids(new_block, [&](const BlockID& block_id) noexcept {
                return self.ctr_ref_block(block_id);
            }));
        }
        else {
            MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch(new_block, RefLeafChildren(), self));
        }

        return ResultT::of(new_block);
    }



    MEMORIA_V1_DECLARE_NODE_FN(CopyDataToFn, copy_node_data_to);
    VoidResult ctr_resize_block(TreePathT& path, size_t level, int32_t new_size) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, level));

        bool root = path[level]->is_root();
        bool leaf = path[level]->is_leaf();

        MEMORIA_TRY(new_node, self.ctr_create_node(level, root, leaf, new_size));

        MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch_1st_const(path[level], new_node, CopyDataToFn()));

        if (MMA_UNLIKELY(root))
        {
            MEMORIA_TRY_VOID(self.set_root(new_node->id()));
        }
        else {
            MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(path, level));
            MEMORIA_TRY(old_child, self.ctr_set_child_id(path[level + 1], parent_idx, new_node->id()));
            MEMORIA_TRY_VOID(self.ctr_ref_block(new_node->id()));
            MEMORIA_TRY_VOID(self.ctr_unref_block(old_child));
        }

        path[level] = new_node;

        return VoidResult::of();
    }

    VoidResult ctr_update_block_guard(NodeBaseG& node) noexcept
    {
        if (!ctr_is_mutable_node(node)) {
            return MEMORIA_MAKE_GENERIC_ERROR("CoW Error: trying to update immutable node");
        }

        return self().store().check_updates_allowed();
    }

    bool is_cascade_tree_removal() const noexcept {
        return self().store().is_cascade_tree_removal();
    }

    VoidResult traverse_ctr(
            void* node_handler_ptr
    ) const noexcept
    {
        auto& self = this->self();

        BTreeTraverseNodeHandler<Profile>* node_handler = ptr_cast<BTreeTraverseNodeHandler<Profile>>(node_handler_ptr);

        MEMORIA_TRY(root, self.ctr_get_root_node());
        return ctr_do_cow_traverse_tree(*node_handler, root);
    }

private:

    VoidResult ctr_do_cow_traverse_tree(BTreeTraverseNodeHandler<Profile>& node_handler, NodeBaseG node) const noexcept
    {
        auto& self = this->self();

        constexpr bool is_ctr_directory = bt::CtrDirectoryHelper<ContainerTypeName>::Value;

        if (node->is_leaf())
        {
            if (!is_ctr_directory) {
                return node_handler.process_leaf_node(&node.block()->header());
            }
            else {
                MEMORIA_TRY_VOID(node_handler.process_directory_leaf_node(&node.block()->header()));
                return self.ctr_for_all_leaf_ctr_refs(node, [&](const BlockID& id) noexcept -> VoidResult
                {
                    MEMORIA_TRY(proceed_with, node_handler.proceed_with(id));
                    if (proceed_with){
                        return self.store().traverse_ctr(id, node_handler);
                    }
                    else {
                        return VoidResult::of();
                    }
                });
            }
        }
        else {
            MEMORIA_TRY_VOID(node_handler.process_branch_node(&node.block()->header()));
            return self.ctr_for_all_ids(node, [&](const BlockID& id) noexcept -> VoidResult
            {
                MEMORIA_TRY(proceed_with, node_handler.proceed_with(id));
                if (proceed_with)
                {
                    MEMORIA_TRY(child, self.ctr_get_block(id));
                    return self.ctr_do_cow_traverse_tree(node_handler, child);
                }
                else {
                    return VoidResult::of();
                }
            });
        }

        return VoidResult::of();
    }


    MEMORIA_V1_DECLARE_NODE_FN(ForAllCtrRooIDsFn, for_all_ctr_root_ids);
    VoidResult ctr_for_all_leaf_ctr_refs(const NodeBaseG& node, const std::function<VoidResult (const BlockID&)>& fn) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, ForAllCtrRooIDsFn(), fn);
    }

public:
    VoidResult ctr_remove_all_nodes(TreePathT& start_path, TreePathT& stop_path) noexcept
    {
        auto& self = this->self();

        start_path.clear();
        stop_path.clear();

        MEMORIA_TRY(new_root, self.ctr_create_root_node(0, true, -1));

        MEMORIA_TRY_VOID(self.set_root(new_root->id()));

        start_path.add_root(new_root);

        if (stop_path.size() == 0)
        {
            stop_path.add_root(new_root);
        }

        return VoidResult::of();
    }

    virtual VoidResult internal_unref_cascade(const ApiProfileBlockID<ApiProfileT>& root_block_id_api) noexcept
    {
        BlockID root_block_id;
        block_id_holder_to(root_block_id_api, root_block_id);
        return self().ctr_unref_block_cascade(root_block_id);
    }

    VoidResult drop() noexcept
    {
        auto& self = this->self();

        if (self.store().isActive())
        {
            auto res0 = self.for_each_ctr_reference([&](auto prop_name, auto ctr_id) noexcept -> VoidResult {
                    MEMORIA_TRY_VOID(self.store().drop_ctr(ctr_id));
                    return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res0);

            MEMORIA_TRY_VOID(self.set_root(BlockID{}));

            if (this->do_unregister_)
            {
                this->do_unregister_on_dtr_ = false;
                return self.store().unregisterCtr(self.name(), this);
            }
            else {
                return VoidResult::of();
            }
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot must be in active state to drop containers");
        }

        return VoidResult::of();
    }


    VoidResult cleanup() noexcept
    {
        auto& self = this->self();
        auto metadata = self.ctr_get_root_metadata();

        NodeBaseG new_root = self.ctr_create_node(0, true, true, metadata.memory_block_size());

        return self.set_root(new_root->id());
    }


    VoidResult ctr_remove_redundant_root(TreePathT& path, size_t level) noexcept
    {
        auto& self = this->self();

        if (level + 1 < path.size())
        {
            MEMORIA_TRY(parent, self.ctr_get_node_parent(path, level));

            if (!parent->is_root())
            {
                MEMORIA_TRY_VOID(ctr_remove_redundant_root(path, level + 1));
            }

            if (parent->is_root())
            {
                MEMORIA_TRY(size, self.ctr_get_node_size(parent, 0));
                if (size == 1)
                {
                    NodeBaseG node = path[level];

                    // FIXME redesign it to use tryConvertToRoot(node) instead
                    if (self.ctr_can_convert_to_root(node, parent->root_metadata_size()))
                    {
                        MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, level));

                        MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));
                        MEMORIA_TRY_VOID(self.ctr_node_to_root(node));

                        MEMORIA_TRY_VOID(self.set_root(node->id()));

                        path.remove_root();
                    }
                }
            }
        }

        return VoidResult::of();
    }

    VoidResult ctr_cow_ref_children_after_merge(NodeBaseG block) noexcept
    {
        auto& self = this->self();

        if (!block->is_leaf())
        {
            MEMORIA_TRY_VOID(self.ctr_for_all_ids(block, [&](const BlockID& child_id) noexcept {
                return self.ctr_ref_block(child_id);
            }));
        }
        else {
            MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch(block, RefLeafChildren(), self));
        }

        return VoidResult::of();
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::CoWOpsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
