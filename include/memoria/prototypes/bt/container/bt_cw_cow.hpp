
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

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::CoWOpsWName)

    using typename Base::ApiProfileT;

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::TreePathT;
    using typename Base::SnapshotID;
    using typename Base::BlockID;
    using typename Base::Profile;
    using typename Base::BlockType;
    using typename Base::ContainerTypeName;

    bool ctr_is_mutable_node(const TreeNodeConstPtr& node) const {
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
                self.ctr_ref_block(new_node);

                auto parent_idx = self.ctr_get_parent_idx(path, level);
                auto old_child_id = self.ctr_set_child_id(path[level + 1].as_mutable(), parent_idx, new_node->id());

                self.ctr_unref_block(old_child_id);
            }
            else {
                self.set_root(new_node->id());
            }

            path[level] = new_node.as_immutable();
        }

        self.ctr_check_path(path, level);
    }

    void ctr_ref_block(TreeNodeConstPtr block)
    {
        auto& self = this->self();
        block.clear_orphan();
        return self.store().ref_block(block->id());
    }

    void ctr_ref_block(TreeNodePtr block)
    {
        auto& self = this->self();
        block.clear_orphan();
        return self.store().ref_block(block->id());
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
                return self.store().ref_block(block_id);
            });
        }
        else {
            self.leaf_dispatcher().dispatch(new_block, RefLeafChildren(), self);
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

        self.leaf_dispatcher().dispatch_1st_const(path[level], new_node, CopyDataToFn());

        if (MMA_UNLIKELY(root))
        {
            self.set_root(new_node->id());
        }
        else {
            auto parent_idx = self.ctr_get_parent_idx(path, level);
            auto old_child = self.ctr_set_child_id(path[level + 1].as_mutable(), parent_idx, new_node->id());
            self.ctr_ref_block(new_node);
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


    bool is_cascade_tree_removal() const {
        return self().store().is_cascade_tree_removal();
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


    void drop()
    {
        auto& self = this->self();

        if (self.store().isActive())
        {
            self.for_each_ctr_reference([&](auto prop_name, auto ctr_id) {
                    self.store().drop_ctr(ctr_id);
            });

            self.set_root(BlockID{});

            self.store().on_ctr_drop(self.name());
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
                return self.store().ref_block(child_id);
            });
        }
        else {
            self.leaf_dispatcher().dispatch(block, RefLeafChildren(), self);
        }
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::CoWOpsWName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
