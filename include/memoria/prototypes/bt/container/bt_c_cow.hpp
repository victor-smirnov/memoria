
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

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::TreePathT;
    using typename Base::SnapshotID;
    using typename Base::BlockID;
    using typename Base::Profile;
    using typename Base::BlockType;
    using typename Base::ContainerTypeName;

    bool ctr_is_mutable_node(const TreeNodeConstPtr& node) const noexcept {
        return node->header().snapshot_id() == self().snapshot_id();
    }


    void ctr_cow_clone_path(TreePathT& path, size_t level)
    {
        auto& self = this->self();

        size_t path_size = path.size();

        if (level < path_size && !self.ctr_is_mutable_node(path[level].as_immutable()))
        {
            ctr_cow_clone_path(path, level + 1);

            auto new_node = ctr_clone_block(path[level].as_immutable());

            if (level + 1 < path_size)
            {
                self.ctr_ref_block(new_node->id());

                auto parent_idx = self.ctr_get_parent_idx(path, level);
                auto old_child_id = self.ctr_set_child_id(path[level + 1].as_mutable(), parent_idx, new_node->id());

                self.ctr_unref_block(old_child_id);
            }
            else {
                self.set_root(new_node->id());
            }

            path[level] = new_node.as_immutable();
        }
    }




    MEMORIA_V1_DECLARE_NODE_FN(UnrefLeafChildren, cow_unref_children);
    void ctr_unref_block(const BlockID& block_id)
    {
        auto& self = this->self();

        auto this_ptr = this->shared_from_this();        
        return self.store().unref_block(block_id, [&]() {
            auto block = this_ptr->ctr_get_block(block_id);

            if (!block->is_leaf())
            {
                this_ptr->ctr_for_all_ids(block, [&](const BlockID& child_id) noexcept {
                    return this_ptr->ctr_unref_block(child_id);
                });
            }
            else {
                this_ptr->leaf_dispatcher().dispatch(block, UnrefLeafChildren(), this_ptr->store()).get_or_throw();
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
            self.leaf_dispatcher().dispatch(block, UnrefLeafChildren(), self.store()).get_or_throw();
        }

        return self.store().removeBlock(block->id());
    }





    void ctr_ref_block(const BlockID& block_id)
    {
        auto& self = this->self();
        return self.store().ref_block(block_id);
    }



    MEMORIA_V1_DECLARE_NODE_FN(RefLeafChildren, cow_ref_children);
    TreeNodePtr ctr_clone_block(const TreeNodeConstPtr& src)
    {
        auto& self = this->self();

        auto new_block_tmp = self.store().cloneBlock(src, self.name());
        TreeNodePtr new_block = new_block_tmp;

        if (!new_block->is_leaf())
        {
            self.ctr_for_all_ids(new_block.as_immutable(), [&](const BlockID& block_id) {
                return self.ctr_ref_block(block_id);
            });
        }
        else {
            self.leaf_dispatcher().dispatch(new_block, RefLeafChildren(), self).get_or_throw();
        }

        return new_block;
    }



    MEMORIA_V1_DECLARE_NODE_FN(CopyDataToFn, copy_node_data_to);
    void ctr_resize_block(TreePathT& path, size_t level, int32_t new_size)
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(path, level);

        bool root = path[level]->is_root();
        bool leaf = path[level]->is_leaf();

        auto new_node = self.ctr_create_node(level, root, leaf, new_size);

        self.leaf_dispatcher().dispatch_1st_const(path[level], new_node, CopyDataToFn()).get_or_throw();

        if (MMA_UNLIKELY(root))
        {
            self.set_root(new_node->id());
        }
        else {
            auto parent_idx = self.ctr_get_parent_idx(path, level);
            auto old_child = self.ctr_set_child_id(path[level + 1].as_mutable(), parent_idx, new_node->id());
            self.ctr_ref_block(new_node->id());
            self.ctr_unref_block(old_child);
        }

        path[level] = new_node.as_immutable();
    }

    void ctr_update_block_guard(const TreeNodePtr& node)
    {
        if (!ctr_is_mutable_node(node.as_immutable())) {
            MEMORIA_MAKE_GENERIC_ERROR("CoW Error: trying to update immutable node").do_throw();
        }

        return self().store().check_updates_allowed();
    }

    void ctr_update_block_guard(const TreeNodeConstPtr& node)
    {
        if (!ctr_is_mutable_node(node)) {
            MEMORIA_MAKE_GENERIC_ERROR("CoW Error: trying to update immutable node").do_throw();
        }

        return self().store().check_updates_allowed();
    }


    bool is_cascade_tree_removal() const noexcept {
        return self().store().is_cascade_tree_removal();
    }

    void traverse_ctr(
            void* node_handler_ptr
    ) const noexcept
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
        return self().leaf_dispatcher().dispatch(node, ForAllCtrRooIDsFn(), fn).get_or_throw();
    }

public:
    void ctr_remove_all_nodes(TreePathT& start_path, TreePathT& stop_path)
    {
        auto& self = this->self();

        start_path.clear();
        stop_path.clear();

        auto new_root = self.ctr_create_root_node(0, true, -1);

        self.set_root(new_root->id());

        start_path.add_root(new_root.as_immutable());

        if (stop_path.size() == 0)
        {
            stop_path.add_root(new_root.as_immutable());
        }
    }

    virtual void internal_unref_cascade(const ApiProfileBlockID<ApiProfileT>& root_block_id_api)
    {
        BlockID root_block_id;
        block_id_holder_to(root_block_id_api, root_block_id);
        return self().ctr_unref_block_cascade(root_block_id);
    }

    void drop()
    {
        auto& self = this->self();

        if (self.store().isActive())
        {
            self.for_each_ctr_reference([&](auto prop_name, auto ctr_id) {
                    self.store().drop_ctr(ctr_id);
            });

            self.set_root(BlockID{});

            if (this->do_unregister_)
            {
                this->do_unregister_on_dtr_ = false;
                return self.store().unregisterCtr(self.name(), this);
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot must be in active state to drop containers").do_throw();
        }
    }


    void cleanup()
    {
        auto& self = this->self();
        auto metadata = self.ctr_get_root_metadata();

        TreeNodePtr new_root = self.ctr_create_node(0, true, true, metadata.memory_block_size());

        self.set_root(new_root->id());
    }


    void ctr_remove_redundant_root(TreePathT& path, size_t level)
    {
        auto& self = this->self();

        if (level + 1 < path.size())
        {
            auto parent = self.ctr_get_node_parent(path, level);

            if (!parent->is_root())
            {
                ctr_remove_redundant_root(path, level + 1);
            }

            if (parent->is_root())
            {
                auto size = self.ctr_get_node_size(parent, 0);
                if (size == 1)
                {
                    TreeNodeConstPtr node = path[level];

                    // FIXME redesign it to use tryConvertToRoot(node) instead
                    if (self.ctr_can_convert_to_root(node, parent->root_metadata_size()))
                    {
                        self.ctr_cow_clone_path(path, level);

                        self.ctr_update_block_guard(node);
                        self.ctr_node_to_root(node.as_mutable());

                        self.set_root(node->id());

                        path.remove_root();
                    }
                }
            }
        }
    }

    void ctr_cow_ref_children_after_merge(const TreeNodePtr& block)
    {
        auto& self = this->self();

        if (!block->is_leaf())
        {
            self.ctr_for_all_ids(block.as_immutable(), [&](const BlockID& child_id) {
                return self.ctr_ref_block(child_id);
            });
        }
        else {
            self.leaf_dispatcher().dispatch(block, RefLeafChildren(), self).get_or_throw();
        }

    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::CoWOpsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
